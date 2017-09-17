package com.year17.analyse_ndk_sockets;

/**
 * 作者：张玉辉
 * 时间：2017/9/12.
 */

public class EchoServerActivity extends AbstractEchoActivity {

    public EchoServerActivity() {
        super(R.layout.activity_echo_server);
    }

    @Override
    protected void onStartButtonClicked() {
        Integer port = getPort();
        if(port != null){
            ServerTask serverTask = new ServerTask(port);
            serverTask.start();
        }
    }

    /**
     * 根据给定端口启动TCP服务器
     * @param port 端口号
     * @throws Exception
     */
    private native void nativeStartTcpServer(int port) throws Exception;

    /**
     * 根据给定端口启动UDP服务
     * @param port 端口号
     * @throws Exception
     */
    private native void nativeStartUdpServer(int port) throws Exception;

    /**
     * 服务器端任务
     */
    private class ServerTask extends AbstractEchoTask{
        /** 端口号 **/
        private final int port;

        public ServerTask(int port){
            this.port = port;
        }
        @Override
        protected void onBackground() {
            logMessage("Starting server.");
            try{
//                nativeStartTcpServer(port);
                nativeStartUdpServer(port);
            }catch (Exception e){
                logMessage(e.getMessage());
            }
            logMessage("Server terminated.");
        }
    }
}
