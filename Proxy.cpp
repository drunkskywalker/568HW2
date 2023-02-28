#include "Proxy.hpp"

#include <unistd.h>

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
    lFile(ofstream("proxy.log")),

    port(port),
    acc(io_context, tcp::endpoint(tcp::v4(), atoi(port))),
    cache(Cache(1000, &cache_rwlock, &log_lock, lFile)) {
}

Proxy::Proxy() : Proxy("12345") {
}

void Proxy::begin_proxy() {
  while (1) {
    tcp::socket * user_sock = new tcp::socket(io_context);
    acc.accept(*user_sock);
    //cout << "connected to client\n";
    int x;
    x = id;
    id++;
    thread t(&Proxy::transmit, this, user_sock, x);
    t.detach();
  }
}

void Proxy::handle_connect(tcp::socket * user_sock, tcp::socket * server_sock, int x) {
  string ok = "HTTP/1.1 200 OK\r\n\r\n";
  asio::write(*user_sock, asio::buffer(ok));
  int u = 1;
  int s = 1;
  while (1) {
    try {
      if (!user_sock->is_open() || !server_sock->is_open()) {
        server_sock->close();
        user_sock->close();
        delete user_sock;
        pthread_mutex_lock(&log_lock);
        lFile << x << ": tunnel closed" << endl;
        pthread_mutex_unlock(&log_lock);
        return;
      }
      u = user_sock->available();
      s = server_sock->available();

      if (u > 0) {
        vector<char> buf(u);
        asio::read(*user_sock, asio::buffer(buf));
        asio::write(*server_sock, asio::buffer(buf));
      }
      else if (s > 0) {
        vector<char> buf(s);
        asio::read(*server_sock, asio::buffer(buf));
        asio::write(*user_sock, asio::buffer(buf));
      }
    }
    catch (...) {
      server_sock->close();
      user_sock->close();
      delete user_sock;
      pthread_mutex_lock(&log_lock);
      lFile << x << ": tunnel closed" << endl;
      pthread_mutex_unlock(&log_lock);
      return;
    }
  }
}

// on catching exception, write error log, respond client with error http response, close both sockets
void Proxy::handle_exception(tcp::socket * user_sock,
                             tcp::socket * server_sock,
                             int x,
                             int err) {
  system::error_code ec;
  string pre = "HTTP/1.1 ";
  string post = "\r\n\r\n";
  string b;
  if (err == 400) {
    b = "400 Bad Request";
  }
  else if (err == 502) {
    b = "502 Bad Gateway";
  }

  pthread_mutex_lock(&log_lock);
  lFile << x << ": Responding \"" << pre << b << "\"" << endl;
  pthread_mutex_unlock(&log_lock);
  asio::write(*user_sock, asio::buffer(pre + b + post), ec);
  server_sock->close();
  user_sock->close();
  delete user_sock;
  return;
}

bool Proxy::check_with_cache(request<dynamic_body> & req,
                             tcp::socket * user_sock,
                             tcp::socket * server_sock,
                             int x) {
  string ver;
  if (req.version() == 10) {
    ver = "HTTP/1.0";
  }
  else {
    ver = "HTTP/1.1";
  }
  if (cache.check_read(x, req, user_sock, server_sock) == 0) {
    response<dynamic_body> res = cache.get_cached_response(req);
    http::write(*user_sock, res);

    pthread_mutex_lock(&log_lock);
    lFile << x << ": Responding \"" << ver << " " << res.result_int() << " "
          << res.result() << "\"" << endl;
    pthread_mutex_unlock(&log_lock);

    server_sock->close();
    user_sock->close();
    delete user_sock;
    return true;
  }
  else {
    pthread_mutex_lock(&log_lock);
    lFile << x << ": Requesting \"" << req.method() << " " << req.target() << " " << ver
          << "\" from " << req.at("HOST") << endl;
    pthread_mutex_unlock(&log_lock);
  }
  return false;
}

string Proxy::get_ver(request<dynamic_body> & req) {
  string ver;
  if (req.version() == 10) {
    ver = "HTTP/1.0";
  }
  else {
    ver = "HTTP/1.1";
  }
  return ver;
}

string Proxy::get_ver(response<dynamic_body> & res) {
  string ver;
  if (res.version() == 10) {
    ver = "HTTP/1.0";
  }
  else {
    ver = "HTTP/1.1";
  }
  return ver;
}

void Proxy::transmit(tcp::socket * user_sock, int x) {
  tcp::resolver resolver(io_context);
  tcp::socket server_sock(io_context);
  beast::flat_buffer u_buffer;
  http::request<http::dynamic_body> req;
  //cout << "here\n";
  try {
    http::read(*user_sock, u_buffer, req);
    string ver = get_ver(req);
    pthread_mutex_lock(&log_lock);
    posix_time::ptime recv_time = posix_time::second_clock::universal_time();
    lFile << x << ": " << req.method() << " " << req.target() << " " << ver << " from "
          << user_sock->remote_endpoint().address() << " @ " << recv_time << endl;
    pthread_mutex_unlock(&log_lock);

    string s_info = string(req.at("HOST"));
    vector<string> hp = get_addr(s_info);
    asio::connect(server_sock, resolver.resolve(hp[0], hp[1]));
  }

  catch (...) {
    handle_exception(user_sock, &server_sock, x, 400);
    return;
  }
  try {
    if (req.method_string() == "CONNECT") {
      handle_connect(user_sock, &server_sock, x);
      return;
    }

    if (req.method_string() == "GET") {
      if (check_with_cache(req, user_sock, &server_sock, x)) {
        return;
      }
    }

    try {
      http::write(server_sock, req);
    }
    catch (...) {
      handle_exception(user_sock, &server_sock, x, 502);
      return;
    }
    http::response<http::dynamic_body> res;
    beast::flat_buffer response_buffer;
    http::read(server_sock, response_buffer, res);
    string ver = get_ver(res);
    pthread_mutex_lock(&log_lock);
    lFile << x << ": Received \"" << ver << " " << res.result_int() << " " << res.result()
          << "\" from " << req.at("HOST") << endl;
    lFile << x << ": Responding \"" << ver << " " << res.result_int() << " "
          << res.result() << "\"" << endl;
    pthread_mutex_unlock(&log_lock);

    if (cache.check_store(x, req, res)) {
      cache.try_add(req, res);
    }
    stringstream ss;
    ss << res;
    asio::write(*user_sock, asio::buffer(ss.str()));

    server_sock.close();
    user_sock->close();
    delete user_sock;
    return;
  }

  catch (...) {
    handle_exception(user_sock, &server_sock, x, 502);
    return;
  }
}

int main() {
  // daemon(1, 1);
  // daemon(0, 0);
  Proxy p;
  p.begin_proxy();
  return 1;
}
