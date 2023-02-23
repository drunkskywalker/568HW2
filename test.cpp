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
  //tcp::acceptor acc;
  int status;
  int recv_fd;

  void init_usersock() {
    struct addrinfo recv_host_info;
    struct addrinfo * recv_host_info_list;
    const char * recv_hostname = NULL;
    const char * recv_port = port;

    memset(&recv_host_info, 0, sizeof(recv_host_info));

    recv_host_info.ai_family = AF_UNSPEC;
    recv_host_info.ai_socktype = SOCK_STREAM;
    recv_host_info.ai_flags = AI_PASSIVE;

    status = getaddrinfo(recv_hostname, recv_port, &recv_host_info, &recv_host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
      exit(1);
    }  //if

    recv_fd = socket(recv_host_info_list->ai_family,
                     recv_host_info_list->ai_socktype,
                     recv_host_info_list->ai_protocol);
    if (recv_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
      exit(1);
    }  //if

    int yes = 1;
    status = setsockopt(recv_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(recv_fd, recv_host_info_list->ai_addr, recv_host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot bind socket" << endl;
      cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
      exit(1);
    }  //if

    status = listen(recv_fd, 100);
    if (status == -1) {
      cerr << "Error: cannot listen on socket" << endl;
      cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
      exit(1);
    }
    cout << "listening on " << recv_port << endl;

    freeaddrinfo(recv_host_info_list);
  }

 public:
  Proxy(const char * port) :

      port(port) {  //, acc(io_context, tcp::endpoint(tcp::v4(), atoi(port))) {
    //tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), atoi(port))) ;
  }

  Proxy() : Proxy("1917") {}

  void begin_proxy() {
    init_usersock();
    while (1) {
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      int client_connection_fd =
          accept(recv_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
      thread t(&Proxy::transmit, this, client_connection_fd);
      t.detach();
    }
  }

  int connect_server(const char * dest_hostname, const char * dest_port) {
    struct addrinfo host_info;
    struct addrinfo * host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(dest_hostname, dest_port, &host_info, &host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << dest_hostname << "," << dest_port << ")" << endl;
      return -1;
    }  //if

    int server_fd = socket(host_info_list->ai_family,
                           host_info_list->ai_socktype,
                           host_info_list->ai_protocol);
    if (server_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << dest_hostname << "," << dest_port << ")" << endl;
      return -1;
    }  //if

    cout << "Connecting to " << dest_hostname << " on port " << dest_port << "..."
         << endl;

    status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << dest_hostname << "," << dest_port << ")" << endl;

      return -1;
    }  //if
    freeaddrinfo(host_info_list);
    return server_fd;
  }

  string receive(int fd) {
    string str;
    char buffer[65536];
    int l = 65536;
    do {
      l = recv(fd, buffer, 65536, 0);
      str.append(string(buffer));
    } while (l == 65536);
    return str;
  }

  void transmit(int user_fd) {
    /*
    string request_str;
    char buffer[65536];
    int l = 65536;
    do {
      l = recv(user_fd, buffer, 65536, 0);
      request_str.append(string(buffer));
    } while (l == 65536);
    */
    string request_str = receive(user_fd);
    cout << "ended recv, got " << request_str;
    /*
    try {
      asio::streambuf u_buffer;

      asio::read_until(*user_sock, u_buffer, "\r\n\r\n");
      string request(asio::buffers_begin(u_buffer.data()),
                     asio::buffers_end(u_buffer.data()));

    */
    http::request<http::dynamic_body> req;

    //beast::tcp_stream u_stream(io_context);
    //http::read(*user_sock, u_buffer, req, err);

    http::request_parser<http::dynamic_body> parser;
    parser.put(asio::buffer(request_str.data(), request_str.size()), err);

    // do stuff with request
    req = parser.get();
    cout << req.at("HOST") << endl;

    const char * s_hostname = string(req.at("HOST")).c_str();

    // only for HTTP; check later
    const char * s_port = "80";
    int server_fd = connect_server(s_hostname, s_port);
    send(server_fd, request_str.c_str(), strlen(request_str.c_str()), 0);
    /*
    char recv_buffer[65536];
    string response_str;
    l = 65536;
    do {
      l = recv(server_fd, recv_buffer, 65536, 0);
      response_str.append(string(recv_buffer));
    } while (l == 65536);
    */
    string response_str = receive(server_fd);
    http::response_parser<http::dynamic_body> res_parser;
    res_parser.put(asio::buffer(response_str.data(), response_str.size()), err);
    http::response<http::dynamic_body> res;

    //TODO: do stuff with response
    res = res_parser.get();
    // This only sends the request back!!!
    send(user_fd, response_str.c_str(), strlen(response_str.c_str()), 0);
    close(user_fd);
    close(server_fd);

    /*
      asio::streambuf s_buffer;
      beast::tcp_stream s_stream(io_context);

      tcp::resolver resolver(io_context);

      s_stream.connect(resolver.resolve(hostname, port));

      http::write(s_stream, req);
      http::response<http::dynamic_body> res;

      string response_string;
      beast::flat_buffer buffer;
      http::read(s_stream, buffer, res);
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
    */
  }
};

void daemon() {
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
  daemon();
  Proxy p;
  p.begin_proxy();
}
