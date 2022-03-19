#pragma once
#include<string>
#include<sys/types.h>
#include<sys/socket.h>

class Util{
    public:
        static int ReadLine(int sock,std::string &out){
            char ch='X';//初始化一个值，只要不是'\n'即可
            while(ch!='\n'){
                ssize_t s=recv(sock,&ch,1,0);
                if(s > 0){
                    if(ch=='\r'){
                        recv(sock,&ch,1,MSG_PEEK);//窥探
                        if(ch=='\n'){
                            recv(sock,&ch,1,0);//取走
                        }
                        else{
                            ch='\n';
                        }
                    }
                    out.push_back(ch);
                }
                else if(s == 0){//这种情况说明写端即浏览器关闭了
                    return 0;
                }

                else{
                    //error
                    return -1;
                }
            }
            return out.size();
        }

        static bool CurString(const std::string &target,std::string &sub1_out,std::string &sub2_out,std::string sep){
            size_t pos=target.find(sep);//以sep=": "为例
            if(pos!=std::string::npos){
                sub1_out=target.substr(0,pos);
                sub2_out=target.substr(pos+sep.size());
                return true;
            }
            return false;
        }

};
