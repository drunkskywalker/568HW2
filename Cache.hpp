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
  Cache(size_t capacity);

  bool try_add(string url, response<dynamic_body> res);
  int get_index(string url);
  // TODO: check if this cache is ready to be reused
  bool check_availibility(string url);
  // only works if check_if_cached() return >= 0
  response<dynamic_body> get_cached_response(string url);
};
