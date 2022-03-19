#pragma once

#include<cstdlib>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>
#include"Log.hpp"

#define BACKLOG 5

class TcpServer{
    private:
        int port;//服务器要使用的端口号(自己执行服务器程序时自己传入)
        int listen_sock;//要被监听的套接字
        static TcpServer* svr;//
    //将TcpServer设计为单例模式中的懒汉模式
    private:
        TcpServer(int _port):port(_port),listen_sock(-1){

        }
        TcpServer(const TcpServer &){

        }

    public:
        static TcpServer* GetInstance(int _port){
            if(svr==nullptr){
                pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;//静态生成一把锁
                pthread_mutex_lock(&mutex);
                if(svr==nullptr){
                    svr=new TcpServer(_port);
                    if(svr==nullptr){
                        LOG(FATAL,"tcp_server object create failed!");
                        exit(1);
                    }
                    LOG(INFO,"tcp_server object create success");
                    svr->InitServer();
                }
                pthread_mutex_unlock(&mutex);
            }
            return svr;
        }

        void InitServer(){
            Socket();
            Bind();
            Listen();
            LOG(INFO,"tcp_server init success");
        }
    private:
        void Socket(){
            listen_sock=socket(AF_INET,SOCK_STREAM,0);//创建套接字调用socket
            //第一个参数指明使用IPv4协议，第二个参数胡表明基于流式套接字进行数据运输
            //第三个参数表示你要使用的传输层的协议，因为基于流式套接字的协议只有一个TCP协议
            //因此第三个参数可以直接指定为0
            if(listen_sock < 0){//socket失败返回-1
                LOG(FATAL,"listen_sock create fail!");
                exit(2);
            }
            int obt=1;
            //复用端口号，防止服务器第二次启动时出现bind error的错误
            //setsockopt成功返回0，失败返回-1
            //SOL_SOCKET表示在套接字级别上进行设置
            //SO_REUSEADDR表示允许服务器第二次启动时重用该监听套接字
            //obt相当于bool值，表示是否开启该功能
            while(setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&obt,sizeof(obt)) < 0){}
            LOG(INFO,"listen_sock create seccess");
        }

        void Bind(){
            struct sockaddr_in local;
            local.sin_family=AF_INET;
            local.sin_port=htons(port);//htons将port从主机序列转为网络序列
            local.sin_addr.s_addr=INADDR_ANY;//设置IP地址，INADDR_ANY为0值，不涉及大小端问题表示让系统自己填充IP地址

            if(bind(listen_sock,(struct sockaddr*)&local,sizeof(local)) < 0){//bind失败返回-1
                LOG(FATAL,"listen_sock bind fail!");
                exit(3);
            }
            LOG(INFO,"listen_sock bind seccess");
        }

        void Listen(){
            if(listen(listen_sock,BACKLOG) < 0){//失败返回-1
                LOG(FATAL,"listen_sock listen fail!");
                exit(4);
            }
            LOG(INFO,"listen_sock Listen seccess");
        }
    public:
        int Sock(){//提供获取监听套接字的函数接口
            return listen_sock;
        }

        ~TcpServer(){//析构函数必须为公有的
            if(listen_sock >=0)close(listen_sock);//文件描述符也是资源，需要手动释放
        }

};

TcpServer* TcpServer::svr=nullptr;
