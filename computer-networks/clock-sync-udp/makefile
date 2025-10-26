# Nazwa kompilatora – można nadpisać zewnętrznie
CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -Wall

TARGET = peer-time-sync
SRCS   = peer_time_sync.cpp

.PHONY: all debug clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# debug: kompilacja z -g
debug: CXXFLAGS += -g
debug: clean all

clean:
	rm -f $(TARGET) *.o
