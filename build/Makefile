CXX = g++
CFLAGS = -std=c++2a -O2 -Wall -g 
#CFLAGS+=-U__FILE__ -D__FILE__='"$(subst $(dir $<),,$<)"'

TARGET = server
OBJS = ../code/http/*.cpp \
	   ../code/server/*.cpp \
       ../code/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)




