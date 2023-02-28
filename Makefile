proxy: Proxy.cpp Cache.cpp
	g++ $^ -g -o $@ -lpthread

clean:
	rm proxy
