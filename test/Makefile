.PHONY:all server client tcp_srv
all:client server tcp_srv
server:server.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client:tcp_cli.cc
	g++ -std=c++17 -O0 -g $^ -o $@
tcp_srv:tcp_srv.cc
	g++ -std=c++17 -O0 -g $^ -o $@
.PHONY:clean
clean:
	rm -rf server client tcp_srv