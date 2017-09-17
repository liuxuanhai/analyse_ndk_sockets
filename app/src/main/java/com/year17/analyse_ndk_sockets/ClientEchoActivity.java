package com.year17.analyse_ndk_sockets;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.widget.EditText;

/**
 * 作者：张玉辉
 * 时间：2017/9/14.
 */

public class ClientEchoActivity extends AbstractEchoActivity {
    /** IP地址 **/
    private EditText ipEdit;
    /** 消息编辑 **/
    private EditText messageEdit;

    public ClientEchoActivity(){
        super(R.layout.activity_echo_client);
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ipEdit = (EditText)findViewById(R.id.ip_edit);
        messageEdit = (EditText)findViewById(R.id.message_edit);
    }

    @Override
    protected void onStartButtonClicked() {
        String ip = ipEdit.getText().toString();
        Integer port = getPort();
        String message = messageEdit.getText().toString();
        if((0!=ip.length())&&(port!= null)&&(0 != message.length())){
            ClientTask clientTask = new ClientTask(ip,port,message);
            clientTask.start();
        }
    }

    /**
     * 根据给定服务器IP地址和端口号启动TCP客户端，并且发送给定消息
     * @param ip ip地址
     * @param port 端口号
     * @param message 消息文本
     * @throws Exception
     */
    private native void nativeStartTcpClient(String ip, int port, String message) throws Exception;

    /**
     * 用给定的服务器端IP地址和端口号启动UDP客户端
     * @param ip
     * @param port
     * @param message
     * @throws Exception
     */
    private native void nativeStartUdpClient(String ip,int port,String message) throws Exception;

    /** 客户端任务 **/
    private class ClientTask extends AbstractEchoTask{
        /** 连接的IP地址 **/
        private final String ip;
        /** 端口号 **/
        private final int port;
        /** 发送的文本消息 **/
        private final String message;

        /**
         * 构造函数
         * @param ip 连接的IP地址
         * @param port 连接的端口号
         * @param message 发送的消息文本
         */
        public ClientTask(String ip, int port, String message){
            this.ip = ip;
            this.port = port;
            this.message = message;
        }
        @Override
        protected void onBackground() {
            logMessage("Starting client");
            try{
//                nativeStartTcpClient(ip,port,message);
                nativeStartUdpClient(ip,port,message);
            }catch (Throwable e){
                logMessage(e.getMessage());
            }
            logMessage("Client terminated.");
        }
    }
}
