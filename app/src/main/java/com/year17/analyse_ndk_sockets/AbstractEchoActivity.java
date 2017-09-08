package com.year17.analyse_ndk_sockets;

import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * 作者：张玉辉
 * 时间：2017/9/8.
 */

public abstract class AbstractEchoActivity extends AppCompatActivity implements View.OnClickListener {
    /** 端口号 **/
    protected EditText portEdit;
    /** 服务按钮 **/
    protected Button startButton;
    /** 日志滚动 **/
    protected ScrollView logScroll;
    /** 日志视图 **/
    protected TextView logView;
    /** 布局ID **/
    private final int layoutID;

    public AbstractEchoActivity(int layoutID){
        this.layoutID = layoutID;
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(layoutID);

        portEdit = (EditText)findViewById(R.id.port_edit);
        startButton = (Button)findViewById(R.id.start_button);
        logScroll = (ScrollView)findViewById(R.id.log_scroll);
        logView = (TextView)findViewById(R.id.log_view);

        startButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if(view == startButton){
            onStartButtonClicked();
        }
    }
    /** 单击开始按钮 **/
    protected abstract void onStartButtonClicked();

    /** 以整型获取端口号 **/
    protected Integer getPort(){
        Integer port;
        try{
            port = Integer.valueOf(portEdit.getText().toString());
        }catch (NumberFormatException e){
            port = null;
        }

        return port;
    }

    /**
     * 记录给定的消息
     * @param message 日志消息
     */
    protected void logMessage(final String message){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                logMessageDirect(message);
            }
        });
    }

    /**
     * 直接记录给定的消息
     * @param message 日志消息
     */
    protected void logMessageDirect(final String message){
        logView.append(message);
        logView.append("\n");
        logScroll.fullScroll(View.FOCUS_DOWN);
    }

    /**
     * 抽象异步echo任务
     */
    protected abstract class AbstractEchoTask extends Thread{
        /** Handler对象.**/
        private final Handler handler;

        public AbstractEchoTask(){
            handler = new Handler();
        }

        /** 在调用线程中先执行回调 **/
        protected void onPreExecute(){
            startButton.setEnabled(false);
            logView.setText("");
        }

        public synchronized void start(){
            onPreExecute();
            super.start();
        }

        @Override
        public void run() {
            onBackground();
            handler.post(new Runnable() {
                @Override
                public void run() {
                    onPostExecute();
                }
            });
        }

        /** 新线程中的背景回调 **/
        protected abstract void onBackground();

        /** 在调用线程后执行回调 **/
        protected void onPostExecute(){
            startButton.setEnabled(true);
        }
    }
    static {
        System.loadLibrary("NDK_SOCKETS");
    }
}
