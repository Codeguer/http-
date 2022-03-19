#pragma once
#include<iostream>
#include<string>
#include<cstring>
#include<cstdlib>
#include<unistd.h>

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf =(char*) malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf =(char*) malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') { 
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

bool GetQueryString(std::string &query_string){
    //子进程中的文件描述符0,1都被重定向了，因此要想在显示器上看到
    //输出结果要使用cerr，而不是使用cout
    bool result=false;
    std::string method=getenv("METHOD");
    if(method=="GET"){
        query_string=getenv("QUERY_STRING");
        result=true;
    }
    else if("POST"==method){
        //cgi如何得知需要从标准输入读取多少个字节呢？
        //通过导入的环境变量
        int content_length=atoi(getenv("CONTENT_LENGTH"));
        char c=0;
        while(content_length){
            read(0,&c,1);
            query_string.push_back(c);
            content_length--;
        }
        result=true;
    }
    else{
        result=false;
    }
    char*buf=url_decode((char*)query_string.c_str());
    query_string.clear();
    query_string.append(buf);
    free(buf);

    return result;
}

void CutString(std::string&in,const std::string &sep,std::string&out1,std::string &out2){
    auto pos=in.find(sep);
    if(std::string::npos != pos){
        out1=in.substr(0,pos);
        out2=in.substr(pos+sep.size());
    }
}
