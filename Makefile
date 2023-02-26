TARGETS=server client  proxy
all: $(TARGETS)

server: server.cpp
	g++ -g -o $@ $<
  
client: client.cpp
	g++ -g -o $@ $<
 
#testing: test.cpp##
	#g++ -g -o $@ $< 

proxy: Proxy.cpp Cache.cpp
	g++ $^ -g -o $@