#include <boost/beast/http.hpp>

#include <iostream>
#include <map>
#include <thread>

using namespace std;
using namespace boost::beast::http;

class Cache {
 private:
  unordered_map<string, response<dynamic_body> > response_map;
  vector<string> request_list;
  size_t capacity;

 public:
  Cache(size_t capacity) : capacity(capacity) {}

  bool try_add(string url, response<dynamic_body> res) {
    if (request_list.size() < capacity) {
      response_map[url] = res;
      request_list.push_back(url);
      return true;
    }
    else {
      string old = request_list.front();
      response_map.erase(old);
      request_list.erase(request_list.begin());
      response_map[url] = res;
      request_list.push_back(url);
      return true;
    }
    return false;
  }
  int get_index(string url) {
    for (size_t i = 0; i < request_list.size(); i++) {
      if (request_list[i] == url) {
        return i;
      }
    }
    return -1;
  }

  // TODO: check if this cache is ready to be reused
  bool check_availibility(string url) { return (get_index(url) > -1); }
  // only works if check_if_cached() return >= 0
  response<dynamic_body> get_cached_response(string url) {
    int index = get_index(url);

    request_list.erase(request_list.begin() + index);
    request_list.push_back(url);
    return response_map[url];
  }
};
