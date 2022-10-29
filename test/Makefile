CXX = g++
CFLAGS = -std=c++20 -O2 -Wall -g 

TARGET = test
OBJS = ../test/test.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)




