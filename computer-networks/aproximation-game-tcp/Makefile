CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

CLIENT_SRC := $(wildcard src/client/*.cpp)
SERVER_SRC := $(wildcard src/server/*.cpp)

all: approx-client approx-server

approx-client: $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread

approx-server: $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread

clean:
	rm -f approx-client approx-server