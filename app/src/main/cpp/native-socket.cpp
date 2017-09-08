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

//最大日志消息长度
#define MAX_LOG_MESSAGE_LENGTH 256
#define MAX_BUFFER_SIZE 80

/**
 * 将给定的消息记录到应用程序
 */
static void LogMessage(JNIEnv* env,jobject obj,const char* format,...){
    //缓存日志方法ID
    static jmethodID  methodID = NULL;
    //如果方法ID未缓存
    if(NULL == methodID){
        //从对象获取类
        jclass clazz = env->GetObjectClass(obj);
        //从给定方法获取方法ID
        methodID = env->GetMethodID(clazz,"logMessage","(Ljava/lang/String;)V");
        //释放类引用
        env->DeleteLocalRef(clazz);
    }
    if(NULL != methodID){
        //格式化日志消息
        char buffer[MAX_LOG_MESSAGE_LENGTH];
        va_list ap;
        va_start(ap,format);
        vsnprintf(buffer,MAX_LOG_MESSAGE_LENGTH,format,ap);
        va_end(ap);

        //将缓冲区转换为Java字符串
        jstring message = env->NewStringUTF(buffer);
        //如果字符串构造正确
        if(NULL != message){
            //记录消息
            env->CallVoidMethod(obj,methodID,message);
            //释放消息引用
            env->DeleteLocalRef(message);
        }
    }
}

/**
 * 用给定的异常类和异常消息抛出新的异常
 */
static void ThrowException(
        JNIEnv* env,
        const char* className,
        const char* message){
    //获取异常类
    jclass clazz = env->FindClass(className);
    if(NULL != clazz){
        //抛出异常
        env->ThrowNew(clazz,message);
        //释放原生类引用
        env->DeleteLocalRef(clazz);
    }
}

/**
 * 用给定异常类和错误号的错误消息抛出新异常
 */
static void ThrowErrnoException(JNIEnv* env, const char* className,int errnum){
    char buffer[MAX_LOG_MESSAGE_LENGTH];
    //获取错误号消息
    if(-1 == strerror_r(errnum,buffer,MAX_LOG_MESSAGE_LENGTH)){
        strerror_r(errno,buffer,MAX_LOG_MESSAGE_LENGTH);
    }
    //抛出异常
    ThrowException(env,className,buffer);
}