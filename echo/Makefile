.PHONY:all main client
all:main client
main:main.cc
	g++ -std=c++17 $^ -o $@
client:client.cc
	g++ -std=c++17 $^ -o $@
.PHONY:clean
clean:
	rm -rf main client