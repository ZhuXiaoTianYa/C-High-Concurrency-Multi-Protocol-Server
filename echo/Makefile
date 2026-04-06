.PHONY:all main client
all:main client
main:main.cc
	g++ -std=c++17 $^ -g -O0 -o $@ -lpthread
client:client.cc
	g++ -std=c++17 $^ -o $@
.PHONY:clean
clean:
	rm -rf main client