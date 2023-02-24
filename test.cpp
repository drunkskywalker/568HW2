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
      beast::flat_buffer u_buffer;
      http::request<http::dynamic_body> req;
      http::read(*user_sock, u_buffer, req);

      //http::request_parser<http::dynamic_body> parser;
      //parser.put(asio::buffer(request.data(), request.size()), err);
      //req = parser.get();

      string s_info = string(req.at("HOST"));
      int col = s_info.find(":");
      string hostname;
      string port;
      if (col == -1) {
        hostname = string(req.at("HOST"));
        port = "80";
      }
      else {
        hostname = s_info.substr(0, col);
        port = s_info.substr(col + 1, s_info.length() - col + 1);
      }

      cout << hostname << endl << port << endl;
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

void be_daemon() {
  pid_t pid = fork();
  if (pid > 0) {
    exit(0);
  }
  else if (pid < 0) {
    cerr << "fork() returned -1. exiting\n";
    exit(1);
  }
  else {
    pid_t sid = setsid();
    if (sid < 0) {
      cerr << "setsid() returned -1. exiting\n";
      exit(1);
    }

    pid = fork();
    if (pid == 0) {
      exit(0);
    }
    else if (pid < 0) {
      cerr << "fork() returned -1. exiting\n";
      exit(1);
    }
    else {
      int c = chdir("/");
      if (c < 0) {
        cerr << "chdir() returned -1. exiting\n";
        exit(1);
      }
      umask(0);
      int fd = open("/dev/null", O_RDWR);
      if (fd < 0) {
        cerr << "open() returned -1. exiting\n";
        exit(1);
      }
      int d1 = dup2(fd, 0);
      if (d1 < 0) {
        cerr << "dup2() returned -1. exiting\n";
        exit(1);
      }
      int d2 = dup2(fd, 1);
      if (d2 < 0) {
        cerr << "dup2() returned -1. exiting\n";
        exit(1);
      }
      int d3 = dup2(fd, 2);
      if (d3 < 0) {
        cerr << "dup2() returned -1. exiting\n";
        exit(1);
      }
      if (close(0) < 0 || close(1) < 0 || close(2) < 0 || (fd > 2 && close(fd) < 0)) {
        cerr << "close() returned -1. exiting\n";
        exit(1);
      }
    }
  }
}

int main() {
  //  be_daemon();
  Proxy p;
  p.begin_proxy();
  return 1;
}
