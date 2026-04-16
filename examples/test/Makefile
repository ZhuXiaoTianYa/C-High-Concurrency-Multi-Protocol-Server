.PHONY:all server client tcp_srv client_1 client_2 client_3 client_4 client_5 client_6
all:client server tcp_srv client_1 client_2 client_3 client_4 client_5 client_6
server:server.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client:tcp_cli.cc
	g++ -std=c++17 -O0 -g $^ -o $@
tcp_srv:tcp_srv.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client_1:client_1.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client_2:client_2.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client_3:client_3.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client_4:client_4.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client_5:client_5.cc
	g++ -std=c++17 -O0 -g $^ -o $@
client_6:client_6.cc
	g++ -std=c++17 -O0 -g $^ -o $@
.PHONY:clean
clean:
	rm -rf server client tcp_srv client_1 client_2 client_3 client_4 client_5 client_6