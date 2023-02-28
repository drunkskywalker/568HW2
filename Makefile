TARGETS=proxy
proxy: Proxy.cpp Cache.cpp
	g++ $^ -g -o $@ -lpthread

clean:
	rm $(TARGETS)
