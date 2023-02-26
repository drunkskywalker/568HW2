
#include "Cache.hpp"
using namespace std;
using namespace boost;
using namespace boost::beast;
using namespace asio::ip;

class Proxy {
 private:
  system::error_code err;
  asio::io_context io_context;
  const char * port;
  tcp::acceptor acc;
  Cache cache;
  vector<string> get_addr(string s_info);

 public:
  Proxy(const char * port);
  Proxy();
  void begin_proxy();
  void transmit(tcp::socket * user_sock, int x);
};
