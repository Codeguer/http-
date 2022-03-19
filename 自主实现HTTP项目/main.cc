#include<iostream>
#include<string>
#include<cstdlib>
#include<memory>
#include"HttpServer.hpp"

void Usage(std::string proc){
    std::cout<<"Usage：\v"<<proc<<" port"<<std::endl;

}
int main(int arc,char*arv[]){
    if(arc != 2){
        Usage(arv[0]);
        exit(6);
    }
    int port=atoi(arv[1]);

    //使用智能指针管理HttpServer资源
    std::shared_ptr<HttpServer> http_server(new HttpServer(port));

    http_server->InitServer();
    http_server->Loop();

    return 0;
}
