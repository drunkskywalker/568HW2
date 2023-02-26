#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/date_time.hpp>
#include <pthread.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string_view>
#include <thread>

using namespace std;
using namespace boost;
using namespace boost::beast::http;
using namespace asio::ip;

class Cache {
 private:
  unordered_map<string, response<dynamic_body> > response_map;
  vector<string> request_list;
  size_t capacity;

  pthread_mutex_t * log_lock;
  ofstream & lFile;

  //checks if response has expired
  string check_time(response<dynamic_body> & res);
  //revalidates with server
  bool check_with_server(int id,
                         request<dynamic_body> & req,
                         response<dynamic_body> & res,
                         tcp::socket * user_sock,
                         tcp::socket * server_sock);

  bool check_nocache(response<dynamic_body> & res);
  string get_url(request<dynamic_body> & req);

 public:
  Cache(size_t capacity, pthread_mutex_t * log_lock, ofstream & lFile);

  bool try_add(request<dynamic_body> & req, response<dynamic_body> & res);
  int get_index(string url);
  bool check_store(int id, request<dynamic_body> & req, response<dynamic_body> & res);
  int check_read(int id,
                 request<dynamic_body> & req,
                 tcp::socket * user_sock,
                 tcp::socket * server_sock);
  response<dynamic_body> get_cached_response(request<dynamic_body> & req);
};
