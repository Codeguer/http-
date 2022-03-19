#g++ -o $@ $^ -Iinclude -Llib -lmysqlclient -lpthread -std=c++11 -ldl -static
#g++ -o $@ $^ -Iinclude -Llib -lmysqlclient -std=c++11
.PHONY:all
all:test_cgi mysql_cgi

test_cgi:test_cgi.cc
		g++ -o $@ $^ -std=c++11

mysql_cgi:mysql_cgi.cc
		g++ -o $@ $^ -Iinclude -Llib -lmysqlclient -std=c++11
	
.PHONY:clean
clean:
		rm -f mysql_cgi test_cgi
