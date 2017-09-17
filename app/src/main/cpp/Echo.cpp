//
// Created by zyh on 2017/9/8.
//
//JNI
#include <jni.h>
#include <string>
//NULL
#include <stdio.h>
//va_list, vsnprintf
#include <stdarg.h>
//errno
#include <errno.h>
// strerror_r, memset
#include <string.h>
//socket,bind,getsockname,listen,accept,recv,send,connect
#include <sys/types.h>
#include <sys/socket.h>
//sockaddr_un
#include <sys/un.h>
//htons,sockaddr_in
#include <netinet/in.h>
//inet_ntop
#include <arpa/inet.h>
//close,unlink
#include <unistd.h>
//offsetof
#include <stddef.h>
#include "com_year17_analyse_ndk_sockets_EchoServerActivity_ServerTask.h"
#include "com_year17_analyse_ndk_sockets_EchoServerActivity.h"
#include "com_year17_analyse_ndk_sockets_ClientEchoActivity_ClientTask.h"
#include "com_year17_analyse_ndk_sockets_ClientEchoActivity.h"
#include "com_year17_analyse_ndk_sockets_LocalEchoActivity.h"
//最大日志消息长度
#define MAX_LOG_MESSAGE_LENGTH 256
#define MAX_BUFFER_SIZE 80

/**
 * 将给定的消息记录到应用程序
 * @param env
 * @param obj
 * @param format 输出的字符格式
 */
static void LogMessage(JNIEnv *env, jobject obj, const char *format, ...) {
    //缓存日志方法ID
    static jmethodID methodID = NULL;
    //如果方法ID未缓存
    if (NULL == methodID) {
        //从对象获取类
        jclass clazz = env->GetObjectClass(obj);
        //从给定方法获取方法ID
        methodID = env->GetMethodID(clazz, "logMessage", "(Ljava/lang/String;)V");
        //释放类引用
        env->DeleteLocalRef(clazz);
    }
    if (NULL != methodID) {
        //格式化日志消息
        char buffer[MAX_LOG_MESSAGE_LENGTH];
        va_list ap;
        va_start(ap, format);
        vsnprintf(buffer, MAX_LOG_MESSAGE_LENGTH, format, ap);
        va_end(ap);

        //将缓冲区转换为Java字符串
        jstring message = env->NewStringUTF(buffer);
        //如果字符串构造正确
        if (NULL != message) {
            //记录消息
            env->CallVoidMethod(obj, methodID, message);
            //释放消息引用
            env->DeleteLocalRef(message);
        }
    }
}

/**
 * 用给定的异常类和异常消息抛出新的异常
 * @param env
 * @param className
 * @param message 要输出的异常信息
 */
static void ThrowException(
        JNIEnv *env,
        const char *className,
        const char *message) {
    //获取异常类
    jclass clazz = env->FindClass(className);
    if (NULL != clazz) {
        //抛出异常
        env->ThrowNew(clazz, message);
        //释放原生类引用
        env->DeleteLocalRef(clazz);
    }
}

/**
 * 用给定异常类和错误号的错误消息抛出新异常
 * @param env
 * @param className
 * @param errnum 异常编号
 */
static void ThrowErrnoException(JNIEnv *env, const char *className, int errnum) {
    char buffer[MAX_LOG_MESSAGE_LENGTH];
    //获取错误号消息
    if (-1 == strerror_r(errnum, buffer, MAX_LOG_MESSAGE_LENGTH)) {
        //根据异常编号打印异常信息
        strerror_r(errno, buffer, MAX_LOG_MESSAGE_LENGTH);
    }
    //抛出异常
    ThrowException(env, className, buffer);
}

/**
 * 构造新的TCP socket
 * @param env
 * @param obj
 * @return socket descriptor
 */
static int NewTcpSocket(JNIEnv *env, jobject obj) {
    LogMessage(env, obj, "Constructing a new TCP socket ...");
    //构造socket
    int tcpSocket = socket(PF_INET, SOCK_STREAM, 0);
    //检查socket构造是否正确
    if (-1 == tcpSocket) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    }
    return tcpSocket;
}

/**
 * 将socket绑定到某一端口号
 * @param env
 * @param obj
 * @param sd socket descriptor
 * @param port port number or zero for random port
 */
static void BindSocketToPort(JNIEnv *env, jobject obj, int sd, unsigned short port) {
    struct sockaddr_in address;
    //绑定socket的地址
    memset(&address, 0, sizeof(address));
    address.sin_family = PF_INET;

    //绑定到所有地址
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    //将端口转换为网络字节顺序
    address.sin_port = htonl(port);

    // 绑定socket
    LogMessage(env, obj, "Binding to port %hu.", port);
    if (-1 == bind(sd, (struct sockaddr *) &address, sizeof(address))) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    }
}

/**
 * 获取当前绑定的端口号socket
 * @param env
 * @param obj
 * @param sd socket descriptor
 * @return port number
 * @throws IOException
 */
static unsigned short GetSocketPort(JNIEnv *env, jobject obj, int sd) {
    unsigned short port = 0;
    struct sockaddr_in address;
    socklen_t addressLength = sizeof(address);
    // 获取socket地址
    if (-1 == getsockname(sd, (struct sockaddr *) &address, &addressLength)) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else {
        //将端口转换为主机字节顺序
        port = ntohs(address.sin_port);
        LogMessage(env, obj, "Binded to random port %hu.", port);
    }
    return port;
}

/**
 * 监听指定的待处理连接的backlog的socket，当backlog已满时拒绝新的连接。
 * @param env
 * @param obj
 * @param sd socket descriptor
 * @param backlog backlog size
 * @throws IOException
 */
static void ListenOnSocket(JNIEnv *env, jobject obj, int sd, int backlog) {
    //监听给定backlog的socket
    LogMessage(env, obj,
               "Listening on socket with a backlog of %d pending connections.",
               backlog);
    if (-1 == listen(sd, backlog)) {
        //抛出带错误
        ThrowErrnoException(env, "java/io/IOException", errno);
    }
}

/**
 * 记录给定地址的IP地址和端口号
 * @param env
 * @param obj
 * @param message
 * @param address
 */
static void LogAddress(
        JNIEnv *env,
        jobject obj,
        const char *message,
        const struct sockaddr_in *address) {
    char ip[INET_ADDRSTRLEN];
    //将IP地址转换为字符串
    if (NULL == inet_ntop(PF_INET, &(address->sin_addr), ip, INET_ADDRSTRLEN)) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else {
        //将端口转换为主机字节顺序
        unsigned short port = ntohs(address->sin_port);
        //记录地址
        LogMessage(env, obj, "%s %s:%hu.", message, ip, port);
    }
}

/**
 * 在给定的socket上阻塞和等待进来的客户连接
 * @param env
 * @param obj
 * @param sd
 * @return
 */
static int AcceptOnSocket(JNIEnv *env, jobject obj, int sd) {
    struct sockaddr_in address;
    socklen_t addressLength = sizeof(address);

    //阻塞和等待进来的客户连接，并且接受它
    LogMessage(env, obj, "Waiting for a client connection ...");
    int clientSocket = accept(sd, (struct sockaddr *) &address, &addressLength);
    //如果客户socket无效
    if (-1 == clientSocket) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else {
        //记录地址
        LogAddress(env, obj, "Client connection from ", &address);
    }
    return clientSocket;
}

/**
 * 阻塞并接收来自socket的数据放到缓冲区
 * @param env
 * @param obj
 * @param sd
 * @param buffer data buffer
 * @param bufferSize buffer size
 * @return receive size
 */
static ssize_t ReceiveFromSocket(
        JNIEnv *env,
        jobject obj,
        int sd,
        char *buffer,
        size_t bufferSize) {
    // 阻塞并接收来自socket的数据放到缓冲区
    LogMessage(env, obj, "Receiving from the socket...");
    ssize_t recvSize = recv(sd, buffer, bufferSize - 1, 0);
    // 如果接收失败
    if (-1 == recvSize) {
        // 抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else {
        //以NULL结尾缓冲区形成一个字符串
        buffer[recvSize] = NULL;
        //如果数据接收成功
        if (recvSize > 0) {
            LogMessage(env, obj, "Received %d bytes: %s", recvSize, buffer);
        } else {
            LogMessage(env, obj, "Client disconnected.");
        }
    }
    return recvSize;
}

/**
 * 从给定的缓冲区向给定的socket发送数据
 * @param env
 * @param obj
 * @param sd
 * @param buffer
 * @param bufferSize
 * @return
 */
static ssize_t SendToSocket(
        JNIEnv *env,
        jobject obj,
        int sd,
        const char *buffer,
        size_t bufferSize) {
    //将数据缓冲区发送到socket
    LogMessage(env, obj, "Sending to the socket ...");
    ssize_t sendSize = send(sd, buffer, bufferSize, 0);
    //如果发送失败
    if (-1 == sendSize) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else {
        if (sendSize > 0) {
            LogMessage(env, obj, "Send %d bytes: %s", sendSize, buffer);
        } else {
            LogMessage(env, obj, "Client disconnected.");
        }
    }
    return sendSize;
}

void Java_com_year17_analyse_1ndk_1sockets_EchoServerActivity_nativeStartTcpServer
        (JNIEnv *env, jobject obj, jint port) {
    //构造新的TCP socket
    int serverSocket = NewTcpSocket(env, obj);
    if (NULL == env->ExceptionOccurred()) {
        //将socket绑定到某端口号
        BindSocketToPort(env, obj, serverSocket, (unsigned short) port);
        if (NULL != env->ExceptionOccurred())
            goto exit;
        //如果请求了随机端口号
        if (0 == port) {
            //获取当前绑定的端口号 socket
            GetSocketPort(env, obj, serverSocket);
            if (NULL != env->ExceptionOccurred())
                goto exit;
        }
        //监听有4个等待连接的backlog的socket
        ListenOnSocket(env, obj, serverSocket, 4);
        if (NULL != env->ExceptionOccurred())
            goto exit;
        //接受socket的一个客户连接
        int clientSocket = AcceptOnSocket(env, obj, serverSocket);
        if (NULL != env->ExceptionOccurred()) {
            goto exit;
        }
        char buffer[MAX_BUFFER_SIZE];
        ssize_t recvSize;
        ssize_t sendSize;
        //接收并发送回数据
        while (1) {
            //从sockeet中接收
            recvSize = ReceiveFromSocket(env, obj, clientSocket, buffer, MAX_BUFFER_SIZE);
            if ((0 == recvSize) || (NULL != env->ExceptionOccurred()))
                break;
            //发送给socket
            sendSize = SendToSocket(env, obj, clientSocket, buffer, (size_t) recvSize);
            if ((0 == sendSize) || (NULL != env->ExceptionOccurred()))
                break;
        }
        //关闭客户端socket
        close(clientSocket);
    }

    exit:
    if (serverSocket > 0) {
        close(serverSocket);
    }
}

static void ConnectToAddress(
        JNIEnv *env,
        jobject obj,
        int sd,
        const char *ip,
        unsigned short port) {
    //连接到给定的IP地址和给定的端口号
    LogMessage(env, obj, "Connecting to %s:%uh", ip, port);

    struct sockaddr_in address;

    memset(&address, 0, sizeof(address));
    address.sin_family = PF_INET;
    //将IP地址字符串转换为网络地址
    if (0 == inet_aton(ip, &(address.sin_addr))) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else {
        //将端口号转换为网络字节顺序
        address.sin_port = htons(port);
        //转换为地址
        if (-1 == connect(sd, (const sockaddr *) &address, sizeof(address))) {
            //抛出带错误号的异常
            ThrowErrnoException(env, "java/io/IOException", errno);
        } else {
            LogMessage(env, obj, "Connect.");
        }
    }
}

void Java_com_year17_analyse_1ndk_1sockets_ClientEchoActivity_nativeStartTcpClient
        (JNIEnv *env, jobject obj, jstring ip, jint port, jstring message) {
    //构造新的TCP socket
    int clientSocket = NewTcpSocket(env, obj);
    if (NULL == env->ExceptionOccurred()) {
        //以C字符串形式获取IP地址
        const char *ipAddress = env->GetStringUTFChars(ip, NULL);
        if (NULL == ipAddress)
            goto exit;

        //连接到IP地址和端口
        ConnectToAddress(env, obj, clientSocket, ipAddress, (unsigned short) port);

        //释放IP地址
        env->ReleaseStringUTFChars(ip, ipAddress);

        //如果连接成功
        if (NULL != env->ExceptionOccurred())
            goto exit;

        //以C字符串形式获取消息
        const char *messageText = env->GetStringUTFChars(message, NULL);
        if (NULL == messageText)
            goto exit;

        //获取消息大小
        jsize messageSize = env->GetStringUTFLength(message);

        //发送消息给socket
        SendToSocket(env, obj, clientSocket, messageText, messageSize);

        //释放消息文本
        env->ReleaseStringUTFChars(message, messageText);

        //如果发送未成功
        if (NULL != env->ExceptionOccurred())
            goto exit;
        char buffer[MAX_BUFFER_SIZE];

        //从socket接收
        ReceiveFromSocket(env, obj, clientSocket, buffer, MAX_BUFFER_SIZE);
    }
    exit:
    if (clientSocket > -1) {
        close(clientSocket);
    }
}

/**
 * 构造一个新的UDP socket
 * @param env
 * @param obj
 * @return socket descriptor
 */
static int NewUdpSocket(JNIEnv *env, jobject obj) {
    // 构造socket
    LogMessage(env, obj, "Constructing a new UDP socket...");
    int udpSocket = socket(PF_INET, SOCK_DGRAM, 0);
    //检查socket构造是否正确
    if (-1 == udpSocket) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    }
    return udpSocket;
}

/**
 * 从socket中阻塞并接收数据保存到缓冲区，填充客户端地址
 * @param env
 * @param obj
 * @param sd
 * @param address client address
 * @param buffer data buffer
 * @param bufferSize
 * @return receive size
 */
static ssize_t ReceiveDatagramFromSocket(
        JNIEnv *env,
        jobject obj,
        int sd,
        struct sockaddr_in *address,
        char *buffer,
        size_t bufferSize) {
    socklen_t addressLength = sizeof(struct sockaddr_in);
    //从socket中接收数据报
    LogMessage(env, obj, "Receiving from the socket...");
    ssize_t recvSize = recvfrom(sd, buffer, bufferSize, 0,
                                (struct sockaddr *) address,
                                &addressLength);
    //如果接收失败
    if (-1 == recvSize) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else {
        // 记录地址
        LogAddress(env, obj, "Received from", address);
        //以NULL终止缓冲区使其为一个字符串
        buffer[recvSize] = NULL;
        //如果数据已经接收
        if (recvSize > 0) {
            LogMessage(env, obj, "Received %d bytes: %s", recvSize, buffer);
        }
    }
    return recvSize;
}

/**
 * 用给定的socket发送数据报到给定的地址
 * @param env
 * @param obj
 * @param sd
 * @param address
 * @param buffer
 * @param bufferSize
 * @return
 */
static ssize_t SendDatagramToSocket(
        JNIEnv *env,
        jobject obj,
        int sd,
        const struct sockaddr_in *address,
        const char *buffer,
        size_t bufferSize) {
    //将数据发送至socket的数据缓冲区
    LogMessage(env, obj, "Sending to", address);
    ssize_t sendSize = sendto(sd, buffer, bufferSize, 0,
                              (const sockaddr *) address,
                              sizeof(struct sockaddr_in));
    //如果发送失败
    if (-1 == sendSize) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    } else if (sendSize > 0) {
        LogMessage(env, obj, "Send %d bytes: %s", sendSize, buffer);
    }
    return sendSize;
}

void Java_com_year17_analyse_1ndk_1sockets_EchoServerActivity_nativeStartUdpServer
        (JNIEnv *env, jobject obj, jint port) {
    //构造一个新的UDP socket
    int serverSocket = NewUdpSocket(env, obj);
    if (NULL == env->ExceptionOccurred()) {
        //将socket绑定到某一端口号
        BindSocketToPort(env, obj, serverSocket, (unsigned short) port);
        if (NULL != env->ExceptionOccurred())
            goto exit;

        //如果请求随机端口号
        if (0 == port) {
            // 获取当前绑定的端口号socket
            GetSocketPort(env, obj, serverSocket);
            if (NULL != env->ExceptionOccurred())
                goto exit;
        }
        //客户端地址
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));

        char buffer[MAX_BUFFER_SIZE];
        ssize_t recvSize;
        ssize_t sendSize;

        //从socket中接收
        recvSize = ReceiveDatagramFromSocket(env, obj, serverSocket,
                                             &address, buffer, MAX_BUFFER_SIZE);
        if ((0 == recvSize) || (NULL != env->ExceptionOccurred()))
            goto exit;
        //发送给socket
        sendSize = SendDatagramToSocket(env, obj, serverSocket,
                                        &address, buffer, (size_t) recvSize);
    }
    exit:
    if (serverSocket > 0) {
        close(serverSocket);
    }
}

void Java_com_year17_analyse_1ndk_1sockets_ClientEchoActivity_nativeStartUdpClient
        (JNIEnv *env, jobject obj, jstring ip, jint port, jstring message) {
    //构造一个新的UDP socket
    int clientSocket = NewUdpSocket(env, obj);
    if (NULL == env->ExceptionOccurred()) {
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = PF_INET;

        //以C字符串形式获取IP地址
        const char *ipAddress = env->GetStringUTFChars(ip, NULL);
        if (NULL == ipAddress)
            goto exit;
        //将IP地址字符串转换为网络地址
        int result = inet_aton(ipAddress, &(address.sin_addr));
        //释放IP地址
        env->ReleaseStringUTFChars(ip, ipAddress);
        //如果转换失败
        if (0 == result) {
            //抛出带错误号的异常
            ThrowErrnoException(env, "java/io/IOException", errno);
            goto exit;
        }

        //将端口转换为网络字节顺序
        address.sin_port = htons(port);

        //以C字符串形式获取消息
        const char *messageText = env->GetStringUTFChars(message, NULL);
        if (NULL == messageText)
            goto exit;
        //获取消息大小
        jsize messageSize = env->GetStringUTFLength(message);
        //发送消息给socket
        SendDatagramToSocket(env, obj, clientSocket, &address, messageText, messageSize);
        //如果发送未成功
        if (NULL != env->ExceptionOccurred())
            goto exit;
        char buffer[MAX_BUFFER_SIZE];
        //清除地址
        memset(&address, 0, sizeof(address));
        //从socket接收
        ReceiveDatagramFromSocket(env, obj, clientSocket, &address, buffer, MAX_BUFFER_SIZE);
    }
    exit:
    if (clientSocket > 0) {
        close(clientSocket);
    }
}

/**
 * 构造一个新的原生UNIX socket
 * @param env
 * @param obj
 * @return
 */
static int NewLocalSocket(JNIEnv *env, jobject obj) {
    //构造socket
    LogMessage(env, obj, "Constructing a new Local UNIX socket...");
    //创建本地socket，使用PF_LOCAL协议族中的socket函数实现。
    //本地socket族既支持基于流的socket协议，也支持基于数据报的socket协议
    int localSocket = socket(PF_LOCAL, SOCK_STREAM, 0);
    //检查socket构造是否正确
    if (-1 == localSocket) {
        //抛出带错误号的异常
        ThrowErrnoException(env, "java/io/IOException", errno);
    }
    return localSocket;
}

/**
 * 与TCP及UDP sockets相同，一旦创建就不再需要分配协议地址，本地socket就在其socket族
 * 空间中存在。可以用同一个bind函数将本地socket与客户端用来连接的本地socket名绑定，
 * 通过sockaddr_un结构指定本地socket的协议地址。
 * struct sockaddr_un{
 *     sa_family_t sun_family;
 *     char sun_path[UNIX_PATH_MAX];
 * };
 * local socket协议地址只由一个名字构成，它没有IP地址或者端口号，可以在两个不同的命名
 * 空间中创建本地socket名：
 * 1）Abstract namespace：在本地socket通信协议模块中维护，socket名以NULL字符为前缀以
 * 绑定socket名。
 * 2）Filesystem namespace：通过文件系统以一个特殊socket文件的形式维护，socket名直接
 * 传递给sockaddr_un结构，将socket名与socket绑定。
 * 另见：http://blog.csdn.net/degwei/article/details/51477519
 * @param env
 * @param obj
 * @param sd socket descriptor
 * @param name socket name
 */
static void BindLocalSocketToName(JNIEnv *env, jobject obj, int sd, const char *name) {
    struct sockaddr_un address;

    //名字长度
    const size_t nameLength = strlen(name);

    //路径长度初始化与名称长度相等
    size_t pathLength = nameLength;
    //如果名字不是以“/”开头，即它在抽象命名空间里
    bool abstractNamespace = ('/' != name[0]);
    //抽象命名空间要求目录的第一个字节是0字节，更新路径长度包括0字节
    if (abstractNamespace) {
        pathLength++;
    }
    //检查路径长度
    if (pathLength > sizeof(address.sun_path)) {
        //抛出带错误号的异常
        ThrowException(env, "java/io/IOException", "Name is too big.");
    } else {
        //清除地址字节
        memset(&address,0,sizeof(address));
        address.sun_family = PF_LOCAL;
        //socket路径
        char* sunPath = address.sun_path;
        //第一字节必须是0以使用抽象命名空间
        if(abstractNamespace){
            *sunPath++ = NULL;
        }
        //追加本地名字
        strcpy(sunPath,name);
        //地址长度
        socklen_t addressLength = (offsetof(struct sockaddr_un,sun_path))+pathLength;
        //如果socket名已经绑定，取消连接
        unlink(address.sun_path);
        //绑定socket
        LogMessage(env,obj,"Binding to local name %s%s",
                   (abstractNamespace)?"(null)":"",name);
        if(-1 == bind(sd,(struct sockaddr*)&address,addressLength)){
            ThrowErrnoException(env,"java/io/IOException",errno);
        }
    }
}

/**
 * 用同一个接收函数接收本地socket的输入连接，唯一的区别是由接收函数返回的客户端协议是
 * socketaddr_un类型的。
 * @param env
 * @param obj
 * @param sd
 * @return
 */
static int AcceptOnLocalSocket(JNIEnv* env,jobject obj,int sd){
    //阻塞并等待即将到来的客户端连接并且接收它
    LogMessage(env,obj,"Waiting for a client connection...");
    int clientSocket = accept(sd,NULL,NULL);
    //如果客户端socket无效
    if(-1 == clientSocket){
        ThrowErrnoException(env,"java/io/IOException",errno);
    }
    return clientSocket;
}

void JNICALL Java_com_year17_analyse_1ndk_1sockets_LocalEchoActivity_nativeStartLocalServer
        (JNIEnv *env, jobject obj, jstring name) {
    //构造一个新的本地UNIX socket
    int serverSocket = NewLocalSocket(env,obj);
    if(NULL == env->ExceptionOccurred()){
        //以C字符串形式获取名称
        const char* nameText = env->GetStringUTFChars(name,NULL);
        if(NULL == nameText)
            goto exit;
        //绑定socket到某一端口号
        BindLocalSocketToName(env,obj,serverSocket,nameText);
        //释放name文本
        env->ReleaseStringUTFChars(name,nameText);
        //如果绑定失败
        if(NULL != env->ExceptionOccurred())
            goto exit;
        //监听有4个挂起连接的带backlog的socket
        ListenOnSocket(env,obj,serverSocket,4);
        if(NULL != env->ExceptionOccurred())
            goto exit;
        //接受socket的一个客户连接
        int clientSocket = AcceptOnLocalSocket(env,obj,serverSocket);
        if(NULL != env->ExceptionOccurred())
            goto exit;
        char buffer[MAX_BUFFER_SIZE];
        ssize_t recvSize;
        ssize_t sendSize;

        //接收并发送回数据
        while(1){
            // 从socket中接收
            recvSize = ReceiveFromSocket(env,obj,clientSocket,buffer,MAX_BUFFER_SIZE);
            if((0==recvSize)||(NULL != env->ExceptionOccurred()))
                break;
            //发送给socket
            sendSize = SendToSocket(env,obj,clientSocket,buffer,(size_t)recvSize);
            if((0 == sendSize)||(NULL != env->ExceptionOccurred()))
                break;
        }
        //关闭客户端socket
        close(clientSocket);
    }
    exit:
        if(serverSocket >0 ){
            close(serverSocket);
        }
}