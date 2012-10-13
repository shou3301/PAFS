#include "libFusionFUSE.h"

#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>

#define SPADE_PORT 3301
#define LEN_OF_LISTEN_QUEUE 30
#define BUFFER_SIZE 1024

// All JVM paramters
JavaVM* jvm;
JNIEnv* env;

jclass FUSEReporterClass;
jobject reporterInstance;

jmethodID readMethod;
jmethodID writeMethod;
jmethodID readlinkMethod;
jmethodID renameMethod;
jmethodID linkMethod;
jmethodID unlinkMethod;
jmethodID receivefileMethod;
jmethodID sendfileMethod;
jmethodID createMethod;


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *pvt) {
    jvm = vm;
    return JNI_VERSION_1_2;
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

static void print_log(char* oper, char* param1, int param2) {
    printf("> Current operation catched is : %s\n", oper);
    printf("> Path is : %s\n", param1);
    printf("> Pid is : %d\n", param2);
}

static int spade_readlink(const char *path, pid_t pid, int iotime) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    print_log("readlink", path, (int)pid);

    jstring jpath = (*env)->NewStringUTF(env, path);

    (*env)->CallVoidMethod(env, reporterInstance, readlinkMethod, pid, iotime, jpath);

    return 0;
}

static int spade_unlink(const char *path, pid_t pid) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    print_log("unlink", path, (int)pid);

    jstring jpath = (*env)->NewStringUTF(env, path);

    (*env)->CallVoidMethod(env, reporterInstance, unlinkMethod, pid, jpath);

    return 0;
}

static int spade_symlink(const char *from, const char *to, pid_t pid) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    print_log("symlink", strcat(from, to), (int)pid);

    jstring jpathOriginal = (*env)->NewStringUTF(env, from);
    jstring jpathLink = (*env)->NewStringUTF(env, to);

    (*env)->CallVoidMethod(env, reporterInstance, linkMethod, pid, jpathOriginal, jpathLink);

    return 0;
}

static int spade_rename(const char *from, const char *to, pid_t pid, int link, int iotime) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    print_log("rename", strcat(from, to), (int)pid);

    jstring jpathOld = (*env)->NewStringUTF(env, from);
    jstring jpathNew = (*env)->NewStringUTF(env, to);

    (*env)->CallVoidMethod(env, reporterInstance, renameMethod, pid, 0, jpathOld, jpathNew, link, 0);

    return 0;
}

static int spade_link(const char *from, const char *to, pid_t pid) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    print_log("link", strcat(from, to), (int)pid);

    jstring jpathOriginal = (*env)->NewStringUTF(env, from);
    jstring jpathLink = (*env)->NewStringUTF(env, to);

    (*env)->CallVoidMethod(env, reporterInstance, linkMethod, pid, jpathOriginal, jpathLink);

    return 0;
}

static int spade_read(const char *path, pid_t pid, int iotime, int link) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);
    
    print_log("read", path, (int)pid);

    jstring jpath = (*env)->NewStringUTF(env, path);

    (*env)->CallVoidMethod(env, reporterInstance, readMethod, pid, iotime, jpath, link);

    return 0;
}

static int spade_write(const char *path, pid_t pid, int iotime, int link) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);
    
    print_log("write", path, (int)pid);

    jstring jpath = (*env)->NewStringUTF(env, path);

    (*env)->CallVoidMethod(env, reporterInstance, writeMethod, pid, iotime, jpath, link);

    return 0;
}

static int spade_receivefile(const char *remote_path, const char *remote_ip, const char *local_path) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    jstring j_remote_path = (*env)->NewStringUTF(env, remote_path);
    jstring j_remote_ip = (*env)->NewStringUTF(env, remote_ip);
    jstring j_local_path = (*env)->NewStringUTF(env, local_path);

    (*env)->CallVoidMethod(env, reporterInstance, receivefileMethod, j_remote_path, j_remote_ip, j_local_path);

    return 0;
}

static int spade_sendfile(const char *source_path, const char *dist_path, const char *dist_ip) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    jstring j_source_path = (*env)->NewStringUTF(env, source_path);
    jstring j_dist_ip = (*env)->NewStringUTF(env, dist_ip);
    jstring j_dist_path = (*env)->NewStringUTF(env, dist_path);

    (*env)->CallVoidMethod(env, reporterInstance, sendfileMethod, j_source_path, j_dist_path, j_dist_ip);

    return 0;
}

static int spade_create(const char *path, pid_t pid) {

    (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

    jstring jpath = (*env)->NewStringUTF(env, path);

    (*env)->CallVoidMethod(env, reporterInstance, createMethod, pid, jpath);

    return 0;
}

int bridge_readlink(int sock) {

    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_iotime = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- readlink path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(path, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- readlink pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    // Read io time
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- readlink iotime -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_iotime, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    int iotime = atoi(s_iotime);
    free(s_pid);
    free(s_iotime);

    spade_readlink(path, pid, iotime);

    return 1;
}

int bridge_unlink(int sock) {

    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));

    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- unlink path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(path, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- unlink pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    free(s_pid);

    spade_unlink(path, pid);

    return 1;
}

int bridge_symlink(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *from = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *to = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- symlink from -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(from, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- symlink to -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(to, buffer);
    }

    // Read io time
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- symlink pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    free(s_pid);

    spade_symlink(from, to, pid);

    return 1;
}

int bridge_rename(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *from = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *to = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_link = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_iotime = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- rename from -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(from, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- rename to -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(to, buffer);
    }

    // Read io time
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- rename pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- rename link -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_link, buffer);
    }

    // Read io time
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- rename iotime -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_iotime, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    int link = atoi(s_link);
    int iotime = atoi(s_iotime);
    free(s_pid);
    free(s_iotime);
    free(s_link);

    spade_rename(from, to, pid, link, iotime);

    return 1;
}

int bridge_link(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *from = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *to = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- link from -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(from, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- link to -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(to, buffer);
    }

    // Read io time
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- link pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    free(s_pid);

    spade_link(from, to, pid);

    return 1;
} 

int bridge_read(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_iotime = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_link = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- read path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(path, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- read pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    // Read io time
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- read iotime -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_iotime, buffer);
    }

    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- read link -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_link, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    int iotime = atoi(s_iotime);
    int link = atoi(s_link);
    free(s_pid);
    free(s_link);
    free(s_iotime);

    spade_read(path, pid, iotime, link);

    return 1;
}

int bridge_write(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_iotime = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *s_link = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(path, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    // Read io time
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write iotime -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_iotime, buffer);
    }

    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write link -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_link, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    int iotime = atoi(s_iotime);
    int link = atoi(s_link);
    free(s_pid);
    free(s_link);
    free(s_iotime);

    spade_write(path, pid, iotime, link);

    return 1;
}

int bridge_receivefile(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *remote_path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *remote_ip = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *local_path = (char *) malloc(BUFFER_SIZE * sizeof(char));

    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(remote_path, buffer);
    }

    // Read ip
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(remote_ip, buffer);
    }

    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(local_path, buffer);
    }

    spade_receivefile(remote_path, remote_ip, local_path);

    return 1;
}

int bridge_sendfile(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *dist_path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *source_path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *dist_ip = (char *) malloc(BUFFER_SIZE * sizeof(char));

    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(source_path, buffer);
    }

    // Read ip
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(dist_path, buffer);
    }

    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(dist_ip, buffer);
    }

    spade_sendfile(source_path, dist_path, dist_ip);

    return 1;
}

int bridge_create(int sock) {
    int new_server_socket = sock;
    char buffer[BUFFER_SIZE];
    int length;

    char *path = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char *s_pid = (char *) malloc(BUFFER_SIZE * sizeof(char));

    // Read path
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write path -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(path, buffer);
    }

    // Read pid
    bzero(buffer, BUFFER_SIZE);
    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- write pid -- ERROR reading from socket", new_server_socket);
        return 0;
    }
    else {
        strcpy(s_pid, buffer);
    }

    pid_t pid = (pid_t) atoi(s_pid);
    free(s_pid);

    spade_create(path, pid);

    return 1;
}

void* dispatch(void* para) {

    int new_server_socket = *((int*)para);      // new socket id

    printf("Accept from %d\n", new_server_socket);

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    // Receive data
    int length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0) {
        printf("Thread %d -- Server receive data failed!\n", new_server_socket);
        close(new_server_socket);
        return;
    }

    // For test
    printf("Thread %d -- Here is the message: %s;\n", new_server_socket, buffer);

    if (strcmp(buffer, "readlink") == 0) {

        bridge_readlink(new_server_socket);
        
    } else if (strcmp(buffer, "unlink") == 0) {

        bridge_unlink(new_server_socket);
        
    } else if (strcmp(buffer, "symlink") == 0) {

        bridge_symlink(new_server_socket);

    } else if (strcmp(buffer, "rename") == 0) {

        bridge_rename(new_server_socket);

    } else if (strcmp(buffer, "link") == 0) {

        bridge_link(new_server_socket);

    } else if (strcmp(buffer, "read") == 0) {

        bridge_read(new_server_socket);

    } else if (strcmp(buffer, "write") == 0) {

        bridge_write(new_server_socket);

    } else if (strcmp(buffer, "receivefile") == 0) {

        bridge_receivefile(new_server_socket);

    } else if (strcmp(buffer, "create") == 0) {
        
        bridge_create(new_server_socket);
    } else if (strcmp(buffer, "sendfile") == 0) {

        bridge_sendfile(new_server_socket);
    }

    close(new_server_socket);
    pthread_exit(NULL);
}

int launch_Service() {

    printf("Starting service...\n");

    // Initialize sockaddr_in
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SPADE_PORT);

    printf("Creating server socket...\n");
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Create socket failed!\n");
        exit(1);
    }

    printf("Binding server socket...\n");
    // Bind socket to server address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        printf("Server bind port: %d failed!\n", SPADE_PORT);
        exit(1);
    }

    printf("Start listening...\n");
    // Listen port
    if (listen(server_socket, LEN_OF_LISTEN_QUEUE)) {
        printf("Server lisen failed!\n");
        exit(1);
    }

    while (1) {

        // Client socket address structure
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);

        // accept a new socket connection from client
        int new_server_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
        if (new_server_socket < 0) {
            printf("Server accept failed!\n");
            break;
        }
        else {
            printf("New thread %d created!\n", new_server_socket);
        }

        // Init child thread
        pthread_t child_thread;
        pthread_attr_t child_thread_attr;
        pthread_attr_init(&child_thread_attr);
        pthread_attr_setdetachstate(&child_thread_attr, PTHREAD_CREATE_DETACHED);

        // Create child thread
        if (pthread_create(&child_thread, &child_thread_attr, dispatch, (void*)&new_server_socket) < 0) {
            printf("Thread create failed!\n");
        }
    }

    close(server_socket);

    return 0;
}

JNIEXPORT jint JNICALL Java_spade_reporter_FusionFUSE_launchFUSEServer (JNIEnv *e, jobject o, jstring mountPoint) {

	reporterInstance = o;
    env = e;

    FUSEReporterClass = (*env)->FindClass(env, "spade/reporter/FusionFUSE");
    readMethod = (*env)->GetMethodID(env, FUSEReporterClass, "read", "(IILjava/lang/String;I)V");
    writeMethod = (*env)->GetMethodID(env, FUSEReporterClass, "write", "(IILjava/lang/String;I)V");
    readlinkMethod = (*env)->GetMethodID(env, FUSEReporterClass, "readlink", "(IILjava/lang/String;)V");
    renameMethod = (*env)->GetMethodID(env, FUSEReporterClass, "rename", "(IILjava/lang/String;Ljava/lang/String;II)V");
    linkMethod = (*env)->GetMethodID(env, FUSEReporterClass, "link", "(ILjava/lang/String;Ljava/lang/String;)V");
    unlinkMethod = (*env)->GetMethodID(env, FUSEReporterClass, "unlink", "(ILjava/lang/String;)V");
    receivefileMethod = (*env)->GetMethodID(env, FUSEReporterClass, "receivefile", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    createMethod = (*env)->GetMethodID(env, FUSEReporterClass, "create", "(ILjava/lang/String;)V");
    sendfileMethod = (*env)->GetMethodID(env, FUSEReporterClass, "sendfile", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

    launch_Service();

    return 0;

}

// For test
// main(int argc, char *argv) {
//     launch_Service();
// }