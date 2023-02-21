TARGETS=server client
all: $(TARGETS)

server: server.cpp
	g++ -g -o $@ $<
  
client: client.cpp
	g++ -g -o $@ $<
