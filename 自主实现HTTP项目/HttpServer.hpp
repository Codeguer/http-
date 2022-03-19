#pragma once

#include<signal.h>
#include"Log.hpp"
#include"Task.hpp"
#include"ThreadPool.hpp"
#include"TcpServer.hpp"

#define PORT 8080

class HttpServer{
    private:
        int port;
        int stop;
    public:
        HttpServer(int _port=PORT):port(_port),stop(false){
        }
        void InitServer(){
            LOG(INFO,"..............................");
            LOG(INFO,"Server starting");
            signal(SIGPIPE,SIG_IGN);//将管道信号忽略
            //在创建多线程之前进行忽略
            //当某一个线程获取到任务后，该线程会创建子进程，然后用匿名管道进行通信
            //为了防止子进程（作为读端时)被关闭而导致该线程被信号所杀的情况
            //因此需要将SIGPIPE信号给设置为忽略
        }

        void Loop(){
            TcpServer* tsvr=TcpServer::GetInstance(port);
            LOG(INFO,"HttpServer Loop Begin");
            while(!stop){
                struct sockaddr_in peer;
                socklen_t len=sizeof(peer);
                //获取请求
                int sock=accept(tsvr->Sock(),(struct sockaddr*)&peer,&len);
                if(sock < 0){
                    LOG(WARNING,"accept sock error");
                    //重新获取
                    continue;
                }
                Task t(sock);
                ThreadPool::GetInstance()->PushTask(t);
            }
        }

        ~HttpServer(){

        }

};
