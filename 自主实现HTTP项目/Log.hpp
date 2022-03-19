#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<ctime>

#define JOURNALNAME "Journalname.txt"

#define INFO 1
#define WARNING 2
#define ERROR 3
#define FATAL 4

//因为levle是个数字，加个#就可以转为字符串了
#define LOG(level,message) log(#level,message,__FILE__,__LINE__)

void HandTime(std::string&information){
    char myStr[25] = { 0 };//用来保存北京时间
	  time_t cur_t = time(nullptr);//获取当前时间，以时间戳形式返回
    struct tm t;
	  while(localtime_r(&cur_t,&t)==nullptr){}//将时间戳转为UTF时间
	  std::string myFormat = "%Y-%m-%d:%H:%M:%S";
	  strftime(myStr, sizeof(myStr), myFormat.c_str(), &t);
	  information.append(myStr);    
}

void log(std::string level,std::string message,std::string file_name,int line){
    std::string information;
    information+="[";
    information+=level;
    information+="]";

    information+="[";
    HandTime(information);
    information+="]";

    information+="[";
    information+=message;
    information+="]";

    information+="[";
    information+=file_name;
    information+="]";

    information+="[";
    information+=std::to_string(line);
    information+="]\n";
    
    std::ofstream ofs(JOURNALNAME,std::ios::app);//以W+模式打开文件
    ofs.write((char*)information.c_str(),information.size());
    //std::cout<<information;
}
