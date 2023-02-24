#include "Proxy.hpp"

using namespace std;
using namespace boost;
using namespace boost::beast;
using namespace asio::ip;

vector<string> Proxy::get_addr(string s_info) {
  int col = s_info.find(":");
  string hostname;
  string port;
  if (col == -1) {
    hostname = s_info;
    port = "80";
  }
  else {
    hostname = s_info.substr(0, col);
    port = s_info.substr(col + 1, s_info.length() - col + 1);
  }
  vector<string> res;
  res.push_back(hostname);
  res.push_back(port);
  return res;
}

Proxy::Proxy(const char * port) :
    port(port),
    acc(io_context, tcp::endpoint(tcp::v4(), atoi(port))),
    cache(Cache(1000)) {
}
Proxy::Proxy() : Proxy("1917") {
}

void Proxy::begin_proxy() {
  while (1) {
    tcp::socket * user_sock = new tcp::socket(io_context);
    acc.accept(*user_sock);
    thread t(&Proxy::transmit, this, user_sock);
    t.detach();
  }
}

void Proxy::transmit(tcp::socket * user_sock) {
  try {
    beast::flat_buffer u_buffer;
    http::request<http::dynamic_body> req;
    http::read(*user_sock, u_buffer, req);

    string s_info = string(req.at("HOST"));
    vector<string> hp = get_addr(s_info);

    tcp::resolver resolver(io_context);
    tcp::socket server_sock(io_context);
    asio::connect(server_sock, resolver.resolve(hp[0], hp[1]));

    if (req.method_string() == "CONNECT") {
      string ok = "HTTP/1.1 200 OK\r\n\r\n";
      asio::write(*user_sock, asio::buffer(ok));
      int u = 1;
      int s = 1;
      while (1) {
        u = user_sock->available();
        s = server_sock.available();

        if (u > 0) {
          vector<char> buf(u);
          asio::read(*user_sock, asio::buffer(buf));
          asio::write(server_sock, asio::buffer(buf));
        }
        else if (s > 0) {
          vector<char> buf(s);
          asio::read(server_sock, asio::buffer(buf));
          asio::write(*user_sock, asio::buffer(buf));
        }
      }
      server_sock.close();
      user_sock->close();
      delete user_sock;
      return;
    }

    //no checking

    if (req.method_string() == "GET") {
      string key = string(req.at("HOST")) + string(req.target());
      if (cache.check_availibility(key)) {
        http::write(*user_sock, cache.get_cached_response(key));
        cout << "received from cache\n";
        return;
      }
    }

    http::write(server_sock, req);
    http::response<http::dynamic_body> res;
    beast::flat_buffer response_buffer;
    http::read(server_sock, response_buffer, res);
    stringstream ss;
    ss << res;
    asio::write(*user_sock, asio::buffer(ss.str()));

    if (req.method_string() == "GET") {
      string key = string(req.at("HOST")) + string(req.target());
      cache.try_add(key, res);
      cout << key << endl;
    }

    server_sock.close();
    user_sock->close();
    delete user_sock;
    return;
  }

  catch (const std::exception & e) {
    cout << "What?" << e.what() << endl;
  }
}

int main() {
  //  be_daemon();
  Proxy p;
  p.begin_proxy();
  return 1;
}
