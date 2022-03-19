#include<iostream>
#include"comm.hpp"

int main(){
    std::string query_string;
    GetQueryString(query_string);
    if(query_string.empty()){
        query_string="a=100&b=200";
    }
    //假如数据是这样的a=100&b=200
    
    std::string str1,str2;
    CutString(query_string,"&",str1,str2);

    std::string name1,value1;
    CutString(str1,"=",name1,value1);

    std::string name2,value2;
    CutString(str2,"=",name2,value2);
    
    //cgi程序将处理后的数据写回管道
    std::cout<<name1<<" : "<<value1<<std::endl;
    std::cout<<name2<<" : "<<value2<<std::endl;

    //用于测试的
    std::cerr<<name1<<" : "<<value1<<std::endl;
    std::cerr<<name2<<" : "<<value2<<std::endl;
    int x=atoi(value1.c_str());
    int y=atoi(value2.c_str());

    //cgi可以实现如下功能：计算、搜索、登陆、存储（比如注册的用户名与密码）
    std::cout<<"<html>";
    std::cout<<"<head><meta charset=\"utf-8\"></head>";
    std::cout<<"<body>";

    std::cout<<"<h3>"<<value1<<" + "<< value2<<" = "<<x+y<<"</h3>";
    std::cout<<"<h3>"<<value1<<" - "<< value2<<" = "<<x-y<<"</h3>";
    std::cout<<"<h3>"<<value1<<" x "<< value2<<" = "<<x*y<<"</h3>";
    std::cout<<"<h3>"<<value1<<" ÷ "<< value2<<" = "<<x/y<<"</h3>";

    std::cout<<"</body>";
    std::cout<<"</html>";
    return 0;
}
