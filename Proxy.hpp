#include "Cache.hpp"

using namespace std;
using namespace boost;
using namespace boost::beast;
using namespace asio::ip;

class Proxy {
 private:
  long long unsigned id = 0;
  pthread_rwlock_t cache_rwlock = PTHREAD_RWLOCK_INITIALIZER;
  pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
  ofstream lFile;
  system::error_code err;
  asio::io_context io_context;
  const char * port;
  tcp::acceptor acc;
  Cache cache;
  vector<string> get_addr(string s_info);
  void handle_connect(tcp::socket * user_sock, tcp::socket * server_sock, int x);
  bool check_with_cache(request<dynamic_body> & req,
                        tcp::socket * user_sock,
                        tcp::socket * server_sock,
                        int x);
  string get_ver(request<dynamic_body> & req);
  string get_ver(response<dynamic_body> & res);

  void handle_exception(tcp::socket * user_sock,
                        tcp::socket * server_sock,
                        int x,
                        int err);

 public:
  Proxy(const char * port);
  Proxy();
  void begin_proxy();
  void transmit(tcp::socket * user_sock, int x);
};
