#pragma once

#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<string>
#include<vector>
#include<unordered_map>
#include<sstream>
#include<algorithm>
#include<sys/stat.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include"Log.hpp"
#include"Util.hpp"

#define SEP ": "
#define LINE_END "\r\n"
#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define PAGE_404 "404/index.html"

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define SERVER_ERROR 500

static std::string Code2Desc(int code){
    std::string desc;
    switch(code){
        case 200:
            desc="OK";
            break;
        case 400:
            desc="Bad Request";
            break;
        case 404:
            desc="Not Found";
            break;
        case 500:
            desc="Server Error";
            break;
        default:
            desc="Error Code";
            break;
    }
    return desc;
}

static std::string Suffix2Desc(const std::string& suffix){
    std::unordered_map<std::string,std::string> suffix2desc{
        {".html","text/html"},
        {".css","text/css"},       
        {".js","application/javascript"},
        {".jpg","application/x-jpg"},
        {".xml","application/xml"}, 
    };
    auto iter=suffix2desc.find(suffix);
    if(iter!=suffix2desc.end()){
        return iter->second;
    }
    return "text/html";
}

class HttpRequest{
    public:
        std::string request_line;//请求行
        std::vector<std::string> request_header;//请求报头
        std::string blank;//空行
        std::string request_body;//请求正文

        //对请求行做进一步处理
        std::string method;//请求方法
        std::string uri;//请求资源
        std::string version;//Http协议版本

        //对uri做进一步处理
        std::string path;//所访问的资源路径
        std::string suffix;//所访问资源的文件后缀名
        std::string query_string;//参数(如果uri有参数的话)

        //对请求报头进行处理
        std::unordered_map<std::string,std::string> header_kv;

        int content_length;//记录请求正文的大小

        bool cgi;//判断是否要用到cgi程序
        int size;//记录所访问的资源的大小

        HttpRequest():query_string(""),content_length(0),cgi(false){}
        ~HttpRequest(){}
};

class HttpResponse{
    public:
        std::string response_line;//响应行
        std::vector<std::string> response_header;//响应报头
        std::string blank;//空行
        std::string response_body;//响应正文

        //响应行需要的状态码
        int status_code;

        //响应是要发送服务器上的某种资源的，因此需要打开该资源，那么
        //需要保存所打开的服务器资源文件的文件描述符的
        int fd;

        HttpResponse():blank(LINE_END),status_code(OK),fd(-1){

        }
        ~HttpResponse(){}
};

class EndPoint{
    private:
        int sock;
        HttpRequest http_request;
        HttpResponse http_reponse;
        bool stop;//控制该执行流是否应该退出
        //1、读取错误，直接退出即可
        //2、逻辑错误(比如浏览器访问了服务器里没有的资源),发送响应后退出
        //3、写入错误 - 1)如果客户端（即读端）出现异常 - 服务器（写端）可能会直接被终止 - 将SIGPIPE设置为忽略即可 
        //            - 2)send出错,但因其概率较小，因此就不通过send的返回值判断将stop进行设置,避免了大量的if判断语句)
        //即让SendHttpResponse()函数正常走完，哪怕send出问题了，走完了就delete ep了,就结束了,不影响整个执行流
        
    public:
        EndPoint(int _sock):sock(_sock),stop(false){

        }
    private:
        bool RecvHttpRequestLine(){//获取请求行
            auto&line=http_request.request_line;
            if(Util::ReadLine(sock,line) > 0){
                line.resize(line.size()-1);//去掉换行符
                LOG(INFO,http_request.request_line);
            }
            else{
                stop=true;//读取错误，直接设置stop为true即可
            }
            return stop;
        }

        bool RecvHttpRequestHeader(){//获取请求报头与设置空行
            std::string line;
            while(true){
                line.clear();
                if(Util::ReadLine(sock,line) <= 0){
                    stop=true;
                    break;
                }
                if(line=="\n"){
                    http_request.blank=line;
                    break;
                }
                line.resize(line.size()-1);//去掉"\n"换行符
                http_request.request_header.push_back(line);
                LOG(INFO,line);
            }
            return stop;
        }

        void ParseHttpRequestLine(){//分析请求行
            auto const&line=http_request.request_line;
            std::stringstream ss(line);
            ss>>http_request.method>>http_request.uri>>http_request.version;

            //将method转为大写
            auto &method=http_request.method;
            std::transform(method.begin(),method.end(),method.begin(),::toupper);//toupper属于C语言的
        }

        void ParseHttpRequestHeader(){//分析请求报头
            std::string key,value;
            for(auto const&iter:http_request.request_header){
                if(Util::CurString(iter,key,value,SEP)){
                    http_request.header_kv.insert({key,value});
                }
            }
        }

        bool IsNeedRecvHttpRequestBody(){//判断是否需要获取请求正文以及记录其大小
            auto const&method=http_request.method;
            if(method=="POST"){
                auto const&header_kv=http_request.header_kv;
                auto const &iter=header_kv.find("Content-Length");
                if(iter!=header_kv.end()){
                    http_request.content_length=atoi(iter->second.c_str());
                    return true;
                }
            }
            return false;
        }

        bool RecvHttpRequestBody(){//获取请求正文
            if(IsNeedRecvHttpRequestBody()){
                auto content_length=http_request.content_length;
                auto &body=http_request.request_body;
                char ch;
                while(content_length > 0){
                    if(recv(sock,&ch,1,0) > 0){
                        body.push_back(ch);
                        --content_length;
                    }
                    else{
                        stop=true;//读取错误，直接退出
                        break;
                    }
                }
                LOG(INFO,body);
            }
            return stop;
        }

    public:
        void RecvHttpRequest(){
            if((!RecvHttpRequestLine())&&(!RecvHttpRequestHeader())){
                ParseHttpRequestLine();
                ParseHttpRequestHeader();
                RecvHttpRequestBody();
                LOG(INFO,"recv http request success ");
            }
            else{
                LOG(ERROR,"recv http request fail!!! ");
            }
        }

        bool IsStop(){
            return stop;
        }

        void BuildHttpResponse(){
            auto &code=http_reponse.status_code;
            struct stat st;
            size_t found=0;
            if(http_request.method!="POST"&&http_request.method!="GET"){
                LOG(WARNING,"http resquest method is error");
                code=BAD_REQUEST;
                goto END;
            } 

            if(http_request.method=="GET"){
                size_t pos=http_request.uri.find("?");
                if(pos!=std::string::npos){
                    if(Util::CurString(http_request.uri,http_request.path,http_request.query_string,"?")){
                        LOG(INFO,"GET method uri CurString success");
                        http_request.cgi=true;
                    }
                    else{
                        LOG(WARNING,"GET method uri CurString fail!!!");
                    }
                }
                else{
                    http_request.path=http_request.uri;
                }
            }

            else if(http_request.method=="POST"){
                //因为HttpRequest我们设置了content_length字段，默认值为0
                //因此POST方法默认调用cgi处理参数
                http_request.cgi=true;
                http_request.path=http_request.uri;
            }

            http_request.path.insert(0,WEB_ROOT);//更改为web根目录

            if(http_request.path[http_request.path.size()-1]=='/'){//添加默认访问的文件名
                //处理类似下面这些请求
                //        http://39.107.67.128:8080/a/b/
                //        http://39.107.67.128:8080/
                //注意：不存在这样的请求http://39.107.67.128:8080(这种请求会被浏览器自动在末尾添加/)
                http_request.path+=HOME_PAGE;
            }

            //判断path中的资源是否存在
            if(stat(http_request.path.c_str(),&st)==0){//存在该资源则返回0,不存在返回-1
                //判断所访问的资源是目录还是可执行程序
                if(S_ISDIR(st.st_mode)){//即判断是否是这个请求:http://39.107.67.128:8080/a/b
                    //st_mode的类型是无符号整形
                    http_request.path+="/";
                    http_request.path+=HOME_PAGE;
                    stat(http_request.path.c_str(),&st);//更新st为首页信息
                }
                //判断是否为可执行程序
                else if((st.st_mode&S_IXUSR)||(st.st_mode&S_IXGRP)||(st.st_mode&S_IXOTH)){
                    http_request.cgi=true;
                }
                http_request.size=st.st_size;
            }

            else{//说明资源是不存在的
                LOG(WARNING,http_request.path+" Not Found");
                code=NOT_FOUND;
                goto END;
            }

            //处理参数
            if(http_request.cgi){
                //触发cgi机制说明有以下几种可能的情况    
                //1、方法是POST    
                //2、方法是GET且带参了    
                //3、访问的资源是可执行程序    
                code=ProcessCgi();//该函数作用如下    
                //调用cgi处理数据，cgi将处理好的数据通过管道给父进程,然后父进程将该数据放入响应正文中
            }

            else{
                //进入了这里那么资源文件一定是首页文件
                code=ProcessNonCgi();//打开该首页文件并将其文件描述符保存到HttpResponse中的文件描述符
            }

            found=http_request.path.rfind(".");
            if(found==std::string::npos){
                http_request.suffix=".html";//找不到默认填充.html
            }

            else{
                http_request.suffix=http_request.path.substr(found);
            }
END:
            BuildHttpResponseHelper();
        }

    private:
        int ProcessCgi(){//调用cgi处理数据，cgi将处理好的数据通过管道给父进程,然后父进程将该数据放入响应正文中
            LOG(INFO,"Process Cgi is Initializing");
            int code=OK;
            auto const&method=http_request.method;
            auto const&query_string=http_request.query_string;
            auto const&bin=http_request.path;
            auto const&content_length=http_request.content_length;
            auto const&body=http_request.request_body;
            auto &response_body=http_reponse.response_body;

            //子进程中要让cgi程序知道参数如果是GET方法通过环境变量，如果是POST方法通过管道
            //cgi程序需要知道是从环境变量还是管道中提取数据，因此需要告诉其method,通过环境变量告诉
            //cgi程序如果从管道里拿参数，那么需要知道要拿多少才拿完，因此需要告诉其数据大小，通过环境变量
            //创建如下环境变量
            std::string query_string_env;//参数
            std::string method_env;//方法
            std::string content_length_env;//数据量大小

            //站在父进程角度上命名这两个数组
            int output[2];//父进程往output[1]写，子进程从output[0]读
            int input[2];//父进程从input[0]读，子进程往input[1]写

            if(pipe(output) < 0){
                LOG(ERROR,"pipe output create fail!!!");
                code=SERVER_ERROR;
                return code;
                
            }
            if(pipe(input) < 0){
                LOG(ERROR,"pipe input create fail!!!");
                code=SERVER_ERROR;
                return code;
            }

            pid_t pid=fork();

            if(pid==0){//child
                close(output[1]);
                close(input[0]);

                method_env+="METHOD=";
                method_env+=method;
                putenv((char*)method_env.c_str());
                
                if(method=="GET"){
                    query_string_env+="QUERY_STRING=";
                    query_string_env+=query_string;
                    putenv((char*)query_string_env.c_str());
                    LOG(INFO,"Get Method,Add Query_String_Env");
                }

                else if(method=="POST"){
                    content_length_env+="CONTENT_LENGTH=";
                    content_length_env+=std::to_string(content_length);
                    putenv((char*)content_length_env.c_str());
                    LOG(INFO,"Post Method,Add Content_Length Env");
                }
                
                while(dup2(output[0],0) < 0){    
                }    
                while(dup2(input[1],1) < 0){//1是input[1]的别名     
                }    

                execl(bin.c_str(),bin.c_str(),nullptr);//替换为cgi程序并执行
                LOG(ERROR,"execl cgi error!!!");
                exit(1);
            }

            else if(pid < 0){//error
                LOG(ERROR,"fork fail!!!");
                code=SERVER_ERROR;
                return code;
            }

            else if(pid > 0){//parent
                close(output[0]);
                close(input[1]);

                if(method=="POST"){
                    const char* start=body.c_str();
                    int size=0,total=0;
                    while(total < content_length&&(size=write(output[1],start+total,body.size()-total)) > 0){
                        total+=size;
                    }
                }

                char ch=0;
                //这里读即使没有数据也是不会被阻塞的
                //因为cgi程序将处理后的数据塞入管道后就结束了其生命周期，
                //那么就代表着管道的写端关闭，根据管道的特性，那么
                //管道的另一端读端，当管道内没有数据时读端就会读到0
                while(read(input[0],&ch,1) > 0){
                    response_body.push_back(ch);
                }

                //回收子进程
                int status=0;
                pid_t ret=waitpid(pid,&status,0);
                if(ret==pid){
                    LOG(INFO,"DEBUG: ret==pid");
                    if(WIFEXITED(status)){
                        if(WEXITSTATUS(status)==0){
                            LOG(INFO,"DEBUG: code=OK");
                            code=OK;
                        }
                        else{
                            LOG(INFO,"DEBUG: code=BAD_REQUEST");
                            code=BAD_REQUEST;
                        }
                    }
                    else{
                        LOG(INFO,"DEBUG: code=SERVER_ERROR");
                        code=SERVER_ERROR;
                    }
                    
                }
                //关闭管道
                close(output[1]);
                close(input[0]);
            }
            return code;
        }

        int ProcessNonCgi(){//填充HttpResponse中的fd成员
            http_reponse.fd=open(http_request.path.c_str(),O_RDONLY);
            //这里检测是否打开成功是因为虽然该文件一定存在，但怕因为其它原因打开失败（比如文件描述符不够）
            //又或者是因为某些原因导致该文件被销毁而打开失败
            if(http_reponse.fd >= 0){
                return OK;
            }
            return NOT_FOUND;
        }

        void BuildHttpResponseHelper(){//构建响应
            auto const &code=http_reponse.status_code;
            //构建响应行
            auto& response_line=http_reponse.response_line;
            response_line+=http_request.version;
            response_line+=" ";
            response_line+=std::to_string(code);
            response_line+=" ";
            response_line+=Code2Desc(code);
            response_line+=LINE_END;

            //构建响应报头
            std::string path=WEB_ROOT;
            path+="/";
            switch(code){
                case OK:
                    BuileOkResponse();
                    break;
                case NOT_FOUND:
                    path+=PAGE_404;
                    HandlerError(path);
                    break;
                case BAD_REQUEST:
                    path+=PAGE_404;
                    HandlerError(path);
                    break;
                case SERVER_ERROR:
                    path+=PAGE_404;
                    HandlerError(path);
                    break;
                default:
                    break;
            }
        }

    private:
        void BuileOkResponse(){//构建成功的响应报头
            std::string line="Content-Type: ";
            //如果浏览器访问的资源是首页文件、图片、音频等，那么我们就将该资源
            //给浏览器，因此其Content-Type就是http_request中的suffix转换
            //如果浏览器访问的是cgi程序，那么cgi程序将处理好的结果以
            //html的形式写入到管道中，因此默认的http_request.suffix就是.html
            line+=Suffix2Desc(http_request.suffix);
            line+=LINE_END;
            http_reponse.response_header.push_back(line);

            line="Content-Length: ";
            if(http_request.cgi){
                line+=std::to_string(http_reponse.response_body.size());
            }
            else{
                line+=std::to_string(http_request.size);
            }
            line+=LINE_END;
            http_reponse.response_header.push_back(line);
        }
        void HandlerError(std::string page){
            //构建错误的静态页面的响应报头
            http_request.cgi=false;
            http_reponse.fd=open(page.c_str(),O_RDONLY);
            if(http_reponse.fd > 0){
                struct stat st;
                stat(page.c_str(),&st);
                http_request.size=st.st_size;

                std::string line ="Content-Type: text/html";
                line+=LINE_END;
                http_reponse.response_header.push_back(line);

                line="Content-Length: ";
                line+=std::to_string(st.st_size);
                line+=LINE_END;
                http_reponse.response_header.push_back(line);
            }
        }

    public:
        void SendHttpResponse(){
            //发送响应行
            send(sock,http_reponse.response_line.c_str(),http_reponse.response_line.size(),0);
            //发送响应报头
            for(auto const &iter:http_reponse.response_header){
                send(sock,iter.c_str(),iter.size(),0);
            }
            //发送空行
            send(sock,http_reponse.blank.c_str(),http_reponse.blank.size(),0);
            //发送响应正文
            if(http_request.cgi){
                auto const &response_body=http_reponse.response_body;
                size_t size=0;
                size_t total=0;
                char const *start=response_body.c_str();
                while(total < response_body.size()&&(size=send(sock,start+total,response_body.size()-total,0))){
                    total+=size;
                }
            }
            else{
                //调用sendfile直接在内核中讲资源发送给sock
                sendfile(sock,http_reponse.fd,nullptr,http_request.size);
                close(http_reponse.fd);
            }
        }
        ~EndPoint(){
            close(sock);
        }
};

//#define DEBUG
class CallBack{
    public:
        CallBack(){
        }
        void operator()(int sock){
            HandlerRequest(sock);//处理请求
        }
        void HandlerRequest(int sock){
#ifdef DEBUG
            char buffer[4096];
            recv(sock,buffer,sizeof(buffer),0);
            std::cout<<"..........................begin............................"<<std::endl;
            std::cout<<buffer<<std::endl;
            std::cout<<"..........................end.............................."<<std::endl;
#else
            EndPoint*ep=new EndPoint(sock);
            ep->RecvHttpRequest();
            if(!ep->IsStop()){
                ep->BuildHttpResponse();
                ep->SendHttpResponse();
            }

            else{
                LOG(WARNING,"Recv Error,Stop Buile And Send");
            }
            delete ep;
#endif
            LOG(INFO,"Hander Request End");
        }
        ~CallBack(){
        }
};
