/**
 * This is a simple wrapper for easy reconfiguration in FusionFS
 * All config command for SPADE control should be listed in ../cfg/reconfig
 */
package spade.client;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;

import spade.core.Kernel;

/**
 * @author cshou
 *
 */
public class FfsConfig {

    private static PrintStream SPADEControlIn;
    // private static BufferedReader SPADEControlOut;
    private static final String reconfigFile = "../cfg/reconfig";
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		try {
			SocketAddress sockaddr = new InetSocketAddress("localhost", Kernel.LOCAL_CONTROL_PORT);
			Socket remoteSocket = new Socket();
			remoteSocket.connect(sockaddr, Kernel.CONNECTION_TIMEOUT);
			OutputStream outStream = remoteSocket.getOutputStream();
			// InputStream inStream = remoteSocket.getInputStream();
			// SPADEControlOut = new BufferedReader(new InputStreamReader(inStream));
			SPADEControlIn = new PrintStream(outStream);
			
			File reconfig = new File(reconfigFile);
			InputStream input = new FileInputStream(reconfig);
			BufferedReader reader = new BufferedReader(new InputStreamReader(input));
			String line;
			
			while ((line = reader.readLine()) != null) {
				SPADEControlIn.println(line);
			}
			
			SPADEControlIn.close();
			reader.close();
			
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
