#include <boost/asio.hpp>
#include <boost/beast/http.hpp>

#include <iostream>
using namespace std;
using namespace boost;
using namespace boost::beast;
using namespace asio::ip;
class Proxy {
 private:
  system::error_code err;
  const char * hostname;
  const char * port;

 public:
  Proxy(const char * hostname, const char * port) : hostname(hostname), port(port) {}

  string recv_from(tcp::socket * sock, asio::streambuf * buffer) {
    size_t a = asio::read_until(*sock, *buffer, "\r\n\r\n");
    cout << "received\n" << buffer << endl;
    asio::streambuf::const_buffers_type bufs = buffer->data();
    string str(asio::buffers_begin(bufs), asio::buffers_begin(bufs) + a);
    return str;
  }

  void transmit() {
    asio::io_context io_context;
    tcp::socket user_sock(io_context);
    tcp::socket server_sock(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 1917));
    tcp::resolver resolver(io_context);
    acceptor.accept(user_sock);
    asio::connect(server_sock, resolver.resolve(hostname, port));
    asio::streambuf u_buffer;

    asio::read_until(user_sock, u_buffer, "\r\n\r\n");
    string request(asio::buffers_begin(u_buffer.data()),
                   asio::buffers_end(u_buffer.data()));
    cout << "sending to server, waiting server response\n";
    cout << asio::write(server_sock, asio::buffer(request)) << " bytes wrote\n";
    cout << "waiting...\n";

    asio::streambuf s_buffer;

    string response_string;
    while (true) {
      asio::read(server_sock,
                 s_buffer,
                 asio::transfer_at_least(1),
                 err);  //, asio::transfer_at_least(1)
      cout << &s_buffer;
      if (err == asio::error::eof) {
        break;
      }
    }
  }
};

int main() {
  /*
  asio::io_context iocontext;
  tcp::acceptor acceptor(iocontext, tcp::endpoint(tcp::v4(), 1917));
  tcp::socket sock(iocontext);
  acceptor.accept(sock);
  cout << "receiving..." << endl;
  asio::streambuf buffer;
  system::error_code err;
  asio::read(sock, buffer, asio::transfer_at_least(1), err);
  cout << &buffer << endl;

  cout << "ending..." << endl;
  */
  const char * hostname = "httpbin.org";
  const char * port = "80";
  Proxy p(hostname, port);
  p.transmit();
}
