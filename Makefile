TARGETS=server client testing
all: $(TARGETS)

server: server.cpp
	g++ -g -o $@ $<
  
client: client.cpp
	g++ -g -o $@ $<
 
testing: test.cpp
	g++ -g -o $@ $< 
