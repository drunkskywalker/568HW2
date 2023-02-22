#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <iostream>
#include <thread>

using namespace std;
using namespace boost;
using namespace boost::beast;
using namespace asio::ip;

// only works for
class Proxy {
 private:
  system::error_code err;
  asio::io_context io_context;
  const char * port;
  tcp::acceptor acc;

 public:
  Proxy(const char * port) :
      port(port), acc(io_context, tcp::endpoint(tcp::v4(), atoi(port))) {
    //tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), atoi(port))) ;
  }

  Proxy() : Proxy("1917") {}

  void begin_proxy() {
    while (1) {
      tcp::socket * user_sock = new tcp::socket(io_context);
      acc.accept(*user_sock);
      thread t(&Proxy::transmit, this, user_sock);
      t.detach();
    }
  }

  void transmit(tcp::socket * user_sock) {
    try {
      asio::streambuf u_buffer;

      asio::read_until(*user_sock, u_buffer, "\r\n\r\n");
      string request(asio::buffers_begin(u_buffer.data()),
                     asio::buffers_end(u_buffer.data()));

      http::request<http::string_body> req;
      http::request_parser<http::string_body> parser;
      parser.put(asio::buffer(request.data(), request.size()), err);
      req = parser.get();
      cout << req.at("HOST") << endl;
      string hostname = string(req.at("HOST"));
      string port = "80";
      asio::streambuf s_buffer;
      beast::tcp_stream stream(io_context);

      tcp::resolver resolver(io_context);
      tcp::socket server_sock(io_context);
      asio::connect(server_sock, resolver.resolve(hostname, port));

      stream.connect(resolver.resolve(hostname, port));

      http::write(stream, req);
      http::response<http::dynamic_body> res;

      string response_string;
      beast::flat_buffer buffer;
      http::read(stream, buffer, res);
      stringstream ss;
      ss << res;
      asio::write(*user_sock, asio::buffer(ss.str()));
      // prove this is multithreading
      // cout << "I AM SLEEPING" << endl;
      // sleep(1000);
    }

    catch (...) {
      cout << "What?" << endl;
    }
  }
};

int main() {
  Proxy p;
  p.begin_proxy();
}
