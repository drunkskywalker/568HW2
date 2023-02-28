#include "Proxy.hpp"
#include <unistd.h>

using namespace std;
using namespace boost;
using namespace boost::beast;
using namespace asio::ip;

long long unsigned id = 0;
pthread_rwlock_t cache_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
ofstream lFile("proxy.log");

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
    cache(Cache(1000, &cache_rwlock, &log_lock, lFile)) {
}
Proxy::Proxy() : Proxy("12345") {
}

void Proxy::begin_proxy() {
  while (1) {
    tcp::socket * user_sock = new tcp::socket(io_context);
    acc.accept(*user_sock);
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
    http::write(*user_sock, cache.get_cached_response(req));
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
  system::error_code ec;
  tcp::resolver resolver(io_context);
  tcp::socket server_sock(io_context);
  beast::flat_buffer u_buffer;
  http::request<http::dynamic_body> req;

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
  pthread_mutex_lock(&log_lock);
    lFile << x << ": Received \"" << ver << " 400 Bad Request " 
          << "\" from " << req.at("HOST") << endl;
    pthread_mutex_unlock(&log_lock);
    asio::write(*user_sock, asio::buffer("HTTP/1.1 400 Bad Request\r\n\r\n"), ec);
    server_sock.close();
    user_sock->close();
    delete user_sock;
    return;
  }
  try {
    if (req.method_string() == "CONNECT") {
      handle_connect(user_sock, &server_sock, x);
      return;
    }

    if (req.method_string() == "GET") {
      if(check_with_cache(req, user_sock, &server_sock, x)){return;}
    }

    try {
      http::write(server_sock, req);
    }
    catch (...) {
    
      pthread_mutex_lock(&log_lock);
    lFile << x << ": Received \"" << ver << " 502 Bad Gateway " 
          << "\" from " << req.at("HOST") << endl;
    pthread_mutex_unlock(&log_lock);
      asio::write(*user_sock, asio::buffer("HTTP/1.1 502 Bad Gateway\r\n\r\n"), ec);
      server_sock.close();
      user_sock->close();
      delete user_sock;
      return;
    }
    http::response<http::dynamic_body> res;
    beast::flat_buffer response_buffer;
    http::read(server_sock, response_buffer, res);
    string ver = get_ver(res);
    pthread_mutex_lock(&log_lock);
    lFile << x << ": Received \"" << ver << " " << res.result_int() << " " << res.result()
          << "\" from " << req.at("HOST") << endl;
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
  
        pthread_mutex_lock(&log_lock);
    lFile << x << ": Received \"" << ver << " 500 Internal Server Error " 
          << "\" from " << req.at("HOST") << endl;
    pthread_mutex_unlock(&log_lock);
    asio::write(*user_sock, asio::buffer("HTTP/1.1 500 Internal Server Error\r\n\r\n"), ec);
    server_sock.close();
    user_sock->close();
    delete user_sock;
    return;
  }
}

int main() {
  // be_daemon();
 // daemon(1, 1);
  Proxy p;
  p.begin_proxy();
  return 1;
}
