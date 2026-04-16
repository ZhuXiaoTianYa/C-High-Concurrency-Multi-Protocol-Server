CC = g++
CFLAGS = -std=c++17 -g

TARGET = main
SRCS = main.cc

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)