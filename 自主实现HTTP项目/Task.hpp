#pragma once
#include"Protocol.hpp"

class Task{
    private:
        int sock;//通信套接字
        CallBack handler;//回调对象
    public:
        Task(){

        }
        Task(int _sock):sock(_sock){

        }

        //通过handler对象将sock交给仿函数CallBack去处理
        void ProcessOn(){
            handler(sock);
        }

        ~Task(){

        }

};
