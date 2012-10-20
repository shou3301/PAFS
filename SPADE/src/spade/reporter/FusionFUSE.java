/*
 --------------------------------------------------------------------------------
 SPADE - Support for Provenance Auditing in Distributed Environments.
 Copyright (C) 2011 SRI International

 This program is free software: you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
 --------------------------------------------------------------------------------
 */
package spade.reporter;

import java.io.BufferedReader;
import java.io.FileReader;
import java.lang.management.ManagementFactory;
import java.net.InetAddress;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.logging.Level;
import java.util.logging.Logger;
import spade.core.AbstractReporter;
import spade.core.AbstractVertex;
import spade.edge.opm.*;
import spade.vertex.custom.File;
import spade.vertex.custom.Network;
import spade.vertex.custom.Program;
import spade.vertex.opm.Agent;

public class FusionFUSE extends AbstractReporter {

	// CSHOU debug
	private org.apache.log4j.Logger fsLogger = org.apache.log4j.Logger.getLogger(FusionFUSE.class);
	
    private long boottime;
    private Map<String, AbstractVertex> localCache;
    private Map<String, String> links;
    private String mountPoint;
    private String mountPath;
    private String myPID;
    private String localHostAddress;
    private String localHostName;
    private boolean refreshHost = false;
    private final String simpleDatePattern = "EEE MMM d H:mm:ss yyyy";
    
    private Thread fusionFUSEServer;

    private native int launchFUSEServer(String argument);

    @Override
    public boolean launch(String arguments) {
        
    	if (arguments == null) {
            return false;
        }

        try {
            localHostAddress = InetAddress.getLocalHost().getHostAddress();
            localHostName = InetAddress.getLocalHost().getHostName();
        } catch (Exception ex) {
            localHostAddress = null;
            localHostName = null;
            Logger.getLogger(FusionFUSE.class.getName()).log(Level.WARNING, null, ex);
        }

        try {
            mountPoint = arguments;
            localCache = Collections.synchronizedMap(new HashMap<String, AbstractVertex>());
            links = Collections.synchronizedMap(new HashMap<String, String>());

            // No need to create mount dir

            mountPath = (new java.io.File(mountPoint)).getAbsolutePath();
            myPID = ManagementFactory.getRuntimeMXBean().getName().split("@")[0].trim();

            // Load the native library.
            System.loadLibrary("FusionFUSE");

            boottime = 0;
            try {
                BufferedReader boottimeReader = new BufferedReader(new FileReader("/proc/stat"));
                String line;
                while ((line = boottimeReader.readLine()) != null) {
                    StringTokenizer st = new StringTokenizer(line);
                    if (st.nextToken().equals("btime")) {
                        boottime = Long.parseLong(st.nextToken()) * 1000;
                        break;
                    } else {
                        continue;
                    }
                }
                boottimeReader.close();
            } catch (Exception exception) {
                Logger.getLogger(FusionFUSE.class.getName()).log(Level.SEVERE, null, exception);
            }

            // Create an initial root vertex which will be used as the root of the
            // process tree.
            Program rootVertex = new Program();
            rootVertex.addAnnotation("pidname", "System");
            rootVertex.addAnnotation("pid", "0");
            rootVertex.addAnnotation("ppid", "0");
            String stime_readable = new java.text.SimpleDateFormat(simpleDatePattern).format(new java.util.Date(boottime));
            String stime = Long.toString(boottime);
            rootVertex.addAnnotation("boottime_unix", stime);
            rootVertex.addAnnotation("boottime_simple", stime_readable);
            localCache.put("0", rootVertex);
            putVertex(rootVertex);

            String path = "/proc";
            String currentProgram;
            java.io.File folder = new java.io.File(path);
            java.io.File[] listOfFiles = folder.listFiles();

            // Build the process tree using the directories under /proc/. Directories
            // which have a numeric name represent processes.
            for (int i = 0; i < listOfFiles.length; i++) {
                if (listOfFiles[i].isDirectory()) {

                    currentProgram = listOfFiles[i].getName();
                    try {
                        Integer.parseInt(currentProgram);
                        Program processVertex = createProgramVertex(currentProgram);
                        String ppid = (String) processVertex.getAnnotation("ppid");
                        localCache.put(currentProgram, processVertex);
                        putVertex(processVertex);
                        if (Integer.parseInt(ppid) >= 0) {
                            if (((Program) localCache.get(ppid) != null) && (processVertex != null)) {
                                WasTriggeredBy triggerEdge = new WasTriggeredBy(processVertex, (Program) localCache.get(ppid));
                                putEdge(triggerEdge);
                            }
                        }
                    } catch (Exception exception) {
                        continue;
                    }
                }
            }

            Runnable fusionServerThread = new Runnable() {

                public void run() {
                    try {
                        // Launch FUSE from the native library.
                        launchFUSEServer(mountPoint);
                    } catch (Exception exception) {
                        Logger.getLogger(FusionFUSE.class.getName()).log(Level.SEVERE, null, exception);
                    }
                }
            };
            
            fusionFUSEServer = new Thread(fusionServerThread, "FusionFUSEServer-Thread");
            fusionFUSEServer.start();

        } catch (Exception exception) {
            Logger.getLogger(FusionFUSE.class.getName()).log(Level.SEVERE, null, exception);
            return false;
        }

        return true;
    }

    private Program createProgramVertex(String pid) {
        // The process vertex is created using the proc filesystem.
        Program resultVertex = new Program();
        try {
            BufferedReader procReader = new BufferedReader(new FileReader("/proc/" + pid + "/status"));
            String nameline = procReader.readLine();
            procReader.readLine();
            String tgidline = procReader.readLine();
            procReader.readLine();
            String ppidline = procReader.readLine();
            String tracerpidline = procReader.readLine();
            String uidline = procReader.readLine();
            String gidline = procReader.readLine();
            procReader.close();

            BufferedReader statReader = new BufferedReader(new FileReader("/proc/" + pid + "/stat"));
            String statline = statReader.readLine();
            statReader.close();

            BufferedReader cmdlineReader = new BufferedReader(new FileReader("/proc/" + pid + "/cmdline"));
            String cmdline = cmdlineReader.readLine();
            cmdlineReader.close();
            if (cmdline == null) {
                cmdline = "";
            } else {
                cmdline = cmdline.replace("\0", " ");
                cmdline = cmdline.replace("\"", "'");
            }

            String stats[] = statline.split("\\s+");
            long elapsedtime = Long.parseLong(stats[21]) * 10;
            long starttime = boottime + elapsedtime;
            String stime_readable = new java.text.SimpleDateFormat(simpleDatePattern).format(new java.util.Date(starttime));
            String stime = Long.toString(starttime);

            StringTokenizer st1 = new StringTokenizer(nameline);
            st1.nextToken();
            String name = st1.nextToken();

            StringTokenizer st3 = new StringTokenizer(ppidline);
            st3.nextToken();
            String ppid = st3.nextToken("").trim();

            if (ppid.equals(myPID)) {
                // Return null if this was our own child process.
                return null;
            }

            StringTokenizer st5 = new StringTokenizer(uidline);
            st5.nextToken();
            String uid = st5.nextToken().trim();

            StringTokenizer st6 = new StringTokenizer(gidline);
            st6.nextToken();
            String gid = st6.nextToken().trim();

            if (refreshHost) {
                try {
                    localHostAddress = InetAddress.getLocalHost().getHostAddress();
                    localHostName = InetAddress.getLocalHost().getHostName();
                } catch (Exception ex) {
                    localHostAddress = null;
                    localHostName = null;
                    Logger.getLogger(FusionFUSE.class.getName()).log(Level.WARNING, null, ex);
                }
            }

            resultVertex.addAnnotation("pidname", name);
            resultVertex.addAnnotation("pid", pid);
            resultVertex.addAnnotation("ppid", ppid);
            resultVertex.addAnnotation("uid", uid);
            resultVertex.addAnnotation("gid", gid);
            resultVertex.addAnnotation("starttime_unix", stime);
            resultVertex.addAnnotation("starttime_simple", stime_readable);
            resultVertex.addAnnotation("group", stats[4]);
            resultVertex.addAnnotation("sessionid", stats[5]);
            resultVertex.addAnnotation("commandline", cmdline);
            resultVertex.addAnnotation("hostname", localHostName);
            resultVertex.addAnnotation("hostaddress", localHostAddress);
        } catch (Exception exception) {
            Logger.getLogger(FusionFUSE.class.getName()).log(Level.SEVERE, null, exception);
            return null;
        }

        try {
            BufferedReader environReader = new BufferedReader(new FileReader("/proc/" + pid + "/environ"));
            String environ = environReader.readLine();
            environReader.close();
            if (environ != null) {
                environ = environ.replace("\0", ", ");
                environ = environ.replace("\"", "'");
                resultVertex.addAnnotation("environment", environ);
            }
        } catch (Exception exception) {
            // Unable to access the environment variables
        }
        return resultVertex;
    }

    private void checkProgramTree(String pid) {
        // Check the process tree to ensure that the given PID exists in it. If not,
        // then add it and recursively check its parents so that this process
        // eventually joins the main process tree.
        try {
            if (localCache.containsKey(pid)) {
                return;
            }
            Program processVertex = createProgramVertex(pid);
            if (processVertex == null) {
                return;
            }
            Agent tempAgent = new Agent();
            tempAgent.addAnnotation("uid", processVertex.removeAnnotation("uid"));
            tempAgent.addAnnotation("gid", processVertex.removeAnnotation("gid"));
            putVertex(processVertex);
            putVertex(tempAgent);
            putEdge(new WasControlledBy(processVertex, tempAgent));
            localCache.put(pid, processVertex);
            String ppid = processVertex.getAnnotation("ppid");
            if (Integer.parseInt(ppid) >= 0) {
                checkProgramTree(ppid);
                WasTriggeredBy triggerEdge = new WasTriggeredBy((Program) localCache.get(pid), (Program) localCache.get(ppid));
                putEdge(triggerEdge);
            }
            
            // cshou debug
            fsLogger.info("Putting program vertext:\n" + processVertex);
            fsLogger.info("Putting temp agent vertex:\n" + tempAgent);
            fsLogger.info("Putting WasControlledBy edge: between program vertex and temp agent");
            
        } catch (Exception exception) {
            Logger.getLogger(FusionFUSE.class.getName()).log(Level.SEVERE, null, exception);
        }
    }

    /**
     * Read event triggered by FUSE.
     *
     * @param pid PID of the triggering process.
     * @param iotime IO time of the operation.
     * @param path Path indicating target file.
     * @param link An integer used to indicate whether the target was a link or
     * not.
     */
    public void read(int pid, int iotime, String path, int link, String size, String mtime) {
    	
    	// cshou debug 
    	fsLogger.info("Enter READ ================================");
    	fsLogger.info("Trying to reach process with PID=" + pid);
    	
        checkProgramTree(Integer.toString(pid));
        path = sanitizePath(path);
        long now = System.currentTimeMillis();
        // Create file artifact depending on whether this is a link or not.
        // Link artifacts are created differently to avoid recursion that may
        // cause FUSE to crash.

        File fileVertex = (link == 1) ? createLinkVertex(path) : createFileVertex(path);
        if (fileVertex == null) {
            fileVertex = new File();
            java.io.File file = new java.io.File(path);
            fileVertex.addAnnotation("filename", file.getName());
            fileVertex.addAnnotation("path", path);
            fileVertex.addAnnotation("size", size);
            fileVertex.addAnnotation("lastmodified", mtime);
            fileVertex.addAnnotation("current hostaddress", localHostAddress);
            fileVertex.addAnnotation("temp", "true");
        }
        putVertex(fileVertex);
        
        // cshou debug
        fsLogger.info("Putting File vertext: \n" + fileVertex);
        
        Used edge = new Used((Program) localCache.get(Integer.toString(pid)), fileVertex);
        edge.addAnnotation("iotime", Integer.toString(iotime));
        edge.addAnnotation("endtime", Long.toString(now));
        putEdge(edge);
        
        // cshou debug
        fsLogger.info("Putting Used edge: \n" + edge);
        
        // If the given path represents a link, then perform the same operation on the
        // artifact to which the link points.
        if (link == 1 && links.containsKey(path)) {
            read(pid, iotime, links.get(path), 0, size, mtime);
        }
    }

    /**
     * Write event triggered by FUSE.
     *
     * @param pid PID of the triggering process.
     * @param iotime IO time of the operation.
     * @param path Path indicating target file.
     * @param link An integer used to indicate whether the target was a link or
     * not.
     */
    public void write(int pid, int iotime, String path, int link) {
    	
    	// cshou debug 
    	fsLogger.info("Enter WRITE ================================");
    	fsLogger.info("Trying to reach process with PID=" + pid);
    	
        checkProgramTree(Integer.toString(pid));
        path = sanitizePath(path);
        long now = System.currentTimeMillis();
        // Create file artifact depending on whether this is a link or not.
        // Link artifacts are created differently to avoid recursion that may
        // cause FUSE to crash.
        File fileVertex = (link == 1) ? createLinkVertex(path) : createFileVertex(path);
        putVertex(fileVertex);
        
        // cshou debug
        fsLogger.info("Putting File vertex:\n" + fileVertex);
        
        WasGeneratedBy edge = new WasGeneratedBy(fileVertex, (Program) localCache.get(Integer.toString(pid)));
        edge.addAnnotation("iotime", Integer.toString(iotime));
        edge.addAnnotation("endtime", Long.toString(now));
        putEdge(edge);
        
        // cshou debug
        fsLogger.info("Putting WasGeneratedBy edge:\n" + edge);
        
        // If the given path represents a link, then perform the same operation on the
        // artifact to which the link points.
        if (link == 1 && links.containsKey(path)) {
            write(pid, iotime, links.get(path), 0);
        }
    }

    /**
     * ReadLink event triggered by FUSE.
     *
     * @param pid PID of the triggering process.
     * @param iotime IO time of the operation.
     * @param path Path indicating target file.
     */
    public void readlink(int pid, int iotime, String path) {
        checkProgramTree(Integer.toString(pid));
        path = sanitizePath(path);
        // Create the file artifact and populate the annotations with file information.
        long now = System.currentTimeMillis();
        File linkFile = createLinkVertex(path);
        putVertex(linkFile);
        Used edge = new Used((Program) localCache.get(Integer.toString(pid)), linkFile);
        edge.addAnnotation("endtime", Long.toString(now));
        edge.addAnnotation("operation", "readlink");
        putEdge(edge);
        // If the given path represents a link, then perform the same operation on the
        // artifact to which the link points.
        if (links.containsKey(path)) {
            read(pid, iotime, links.get(path), 0, "0", "0");
        }
    }

    /**
     * Rename event triggered by FUSE.
     *
     * @param pid PID of the triggering process.
     * @param iotime IO time of the operation.
     * @param pathfrom The source path.
     * @param pathto The destination path.
     * @param link An integer used to indicate whether the target was a link or
     * not.
     * @param done An intiger used to indicate whether this event was triggered
     * before or after the rename operation.
     */
    public void rename(int pid, int iotime, String pathfrom, String pathto, int link, int done) {
        checkProgramTree(Integer.toString(pid));
        pathfrom = sanitizePath(pathfrom);
        pathto = sanitizePath(pathto);
        long now = System.currentTimeMillis();
        // 'done' is used to indicate whether this is a pre-rename or a post-rename
        // call. In pre-rename, a Used edge is created from the process to the old
        // file. In post-rename, a WasGeneratedBy edge is created from the process
        // to the new file and a WasDerivedEdge created between the two file
        // artifacts.
        if (done == 0) {
            // Create file artifact depending on whether this is a link or not.
            // Link artifacts are created differently to avoid recursion that may
            // cause FUSE to crash.
            File fileVertex = (link == 1) ? createLinkVertex(pathfrom) : createFileVertex(pathfrom);
            putVertex(fileVertex);
            // Put the file artifact in the localCache to be removed on post-rename.
            localCache.put(pathfrom, fileVertex);
            Used edge = new Used((Program) localCache.get(Integer.toString(pid)), fileVertex);
            edge.addAnnotation("endtime", Long.toString(now));
            putEdge(edge);
        } else {
            // Create file artifact depending on whether this is a link or not.
            // Link artifacts are created differently to avoid recursion that may
            // cause FUSE to crash.
            File fileVertex = (link == 1 ? createLinkVertex(pathto) : createFileVertex(pathto));
            putVertex(fileVertex);
            WasGeneratedBy writeEdge = new WasGeneratedBy(fileVertex, (Program) localCache.get(Integer.toString(pid)));
            writeEdge.addAnnotation("endtime", Long.toString(now));
            putEdge(writeEdge);
            WasDerivedFrom renameEdge = new WasDerivedFrom(fileVertex, (File) localCache.remove(pathfrom));
            renameEdge.addAnnotation("iotime", Integer.toString(iotime));
            renameEdge.addAnnotation("endtime", Long.toString(now));
            renameEdge.addAnnotation("operation", "rename");
            putEdge(renameEdge);
            if (links.containsKey(pathfrom)) {
                // If the rename is on a link then update the link name.
                String linkedLocation = links.get(pathfrom);
                links.remove(pathfrom);
                links.put(pathto, linkedLocation);
            }
        }
    }

    /**
     * Link event triggered by FUSE.
     *
     * @param pid PID of the triggering process.
     * @param originalFilePath The original file path.
     * @param linkPath Path to link to.
     */
    public void link(int pid, String originalFilePath, String linkPath) {
        checkProgramTree(Integer.toString(pid));
        originalFilePath = sanitizePath(originalFilePath);
        linkPath = sanitizePath(linkPath);
        long now = System.currentTimeMillis();
        File link = createLinkVertex(linkPath);
        putVertex(link);
        File original = createFileVertex(originalFilePath);
        putVertex(original);
        WasDerivedFrom linkEdge = new WasDerivedFrom(original, link);
        linkEdge.addAnnotation("operation", "link");
        linkEdge.addAnnotation("endtime", Long.toString(now));
        putEdge(linkEdge);
        // Add the link to the links map.
        links.put(linkPath, originalFilePath);
    }

    /**
     * Unlink event triggered by FUSE.
     *
     * @param pid PID of the triggering process.
     * @param path The path to unlink.
     */
    public void unlink(int pid, String path) {
        checkProgramTree(Integer.toString(pid));
        path = sanitizePath(path);
        links.remove(path);
    }
    
    /**
     * @param remotePath remote path of the file
     * @param remoteIP remote IP address
     * @param localPath local path of the file, however in current design, it's same with remote path
     */
    public void receivefile(String remotePath, String remoteIP, String localPath, String size, String mtime) {
    	
    	// cshou debug 
    	fsLogger.info("Enter RECEIVE FILE ================================");
    	
    	File fileVertex = new File();
    	java.io.File file = new java.io.File(localPath);
    	fileVertex.addAnnotation("current hostaddress", localHostAddress);
    	fileVertex.addAnnotation("filename", file.getName());
    	fileVertex.addAnnotation("path", localPath);
        fileVertex.addAnnotation("size", size);
        fileVertex.addAnnotation("lastmodified", mtime);
    	fileVertex.addAnnotation("temp", "true");
    	putVertex(fileVertex);
    	fsLogger.info("Putting File vertex: " + fileVertex);
    	
    	Program prog = new Program();
    	prog.addAnnotation("pidname", "FusionFS");
    	prog.addAnnotation("hostname", localHostName);
    	prog.addAnnotation("hostaddress", localHostAddress);
    	putVertex(prog);
    	fsLogger.info("Putting Program vertex: " + prog);
    	
    	Network network = new Network();
    	network.addAnnotation("source host", localHostAddress);
    	network.addAnnotation("source port", "9000");
    	network.addAnnotation("destination host", remoteIP);
    	network.addAnnotation("destination port", "9000");
    	putVertex(network);
    	fsLogger.info("Putting Network vertex: " + network);
    	
    	Used usedEdge = new Used(prog, network);
    	WasGeneratedBy wasGeneratedByEdge = new WasGeneratedBy(fileVertex, prog);
    	putEdge(usedEdge);
    	putEdge(wasGeneratedByEdge);
    	fsLogger.info("Putting Used edge: " + usedEdge);
    	fsLogger.info("Putting Used edge: " + wasGeneratedByEdge);
    }
    
    public void sendfile(String sourcePath, String distPath, String distIP) {
    	
    	// cshou debug
    	fsLogger.info("Enter SEND FILE =========================");
    	
    	File fileVertex = createFileVertex(sourcePath);
    	putVertex(fileVertex);
    	fsLogger.info("Putting file vertex: " + fileVertex);
    	
    	Program prog = new Program();
    	prog.addAnnotation("pidname", "FusionFS");
    	prog.addAnnotation("hostname", localHostName);
    	prog.addAnnotation("hostaddress", localHostAddress);
    	putVertex(prog);
    	fsLogger.info("Putting Program vertex: " + prog);
    	
    	Network network = new Network();
    	network.addAnnotation("source host", localHostAddress);
    	network.addAnnotation("source port", "9000");
    	network.addAnnotation("destination host", distIP);
    	network.addAnnotation("destination port", "9000");
    	putVertex(network);
    	fsLogger.info("Putting Network vertex: " + network);
    	
    	Used usedEdge = new Used(prog, fileVertex);
    	WasGeneratedBy wasGeneratedByEdge = new WasGeneratedBy(network, prog);
    	putEdge(usedEdge);
    	putEdge(wasGeneratedByEdge);
    	fsLogger.info("Putting Used edge: " + usedEdge);
    	fsLogger.info("Putting Used edge: " + wasGeneratedByEdge);

    }
    
    /**
     * @param path The path to the created file
     * @param pid The PID of the creating process
     */
    public void create(int pid, String path) {
    	
    	// cshou debug 
    	fsLogger.info("Enter CREATE ================================");
    	fsLogger.info("Trying to reach process with PID=" + pid);
    	
    	checkProgramTree(Integer.toString(pid));
        path = sanitizePath(path);
        long now = System.currentTimeMillis();
        // Create file artifact depending on whether this is a link or not.
        // Link artifacts are created differently to avoid recursion that may
        // cause FUSE to crash.
        File fileVertex = createFileVertex(path);
        putVertex(fileVertex);
        
        // cshou debug
        fsLogger.info("Putting File vertext: \n" + fileVertex);
        
        WasGeneratedBy edge = new WasGeneratedBy(fileVertex, (Program)localCache.get(Integer.toString(pid)));
        edge.addAnnotation("createtime", Long.toString(now));
        putEdge(edge);
        
        // cshou debug
        fsLogger.info("Putting Used edge: \n" + edge);

    }

    private File createFileVertex(String path) {
        File fileVertex = new File();
        java.io.File file = new java.io.File(path);

        if (!file.exists())
            return null;

        fileVertex.addAnnotation("hostaddress", localHostAddress);
        fileVertex.addAnnotation("filename", file.getName());
        fileVertex.addAnnotation("path", path);
        long filesize = file.length();
        long lastmodified = file.lastModified();
        fileVertex.addAnnotation("size", Long.toString(filesize));
        String lastmodified_readable = new java.text.SimpleDateFormat(simpleDatePattern).format(new java.util.Date(lastmodified));
        fileVertex.addAnnotation("lastmodified_unix", Long.toString(lastmodified));
        fileVertex.addAnnotation("lastmodified_simple", lastmodified_readable);
        return fileVertex;
    }

    private File createLinkVertex(String path) {
        File linkVertex = new File();
        linkVertex.addAnnotation("path", path);
        linkVertex.addAnnotation("filetype", "link");
        return linkVertex;
    }

    private String sanitizePath(String path) {
        // Sanitize path to avoid recursion inside FUSE which can cause the
        // reporter to crash.
        /*if (path.startsWith(mountPath)) {
            path = path.substring(mountPath.length());
        }*/
        return path;
    }

    @Override
    public boolean shutdown() {
        try {
            fusionFUSEServer.join();
            return true;
        } catch (Exception exception) {
            Logger.getLogger(FusionFUSE.class.getName()).log(Level.SEVERE, null, exception);
            return false;
        }
    }
}
