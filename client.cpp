#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

using namespace std;

int main() {
  //  while (1) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  const char * hostname = "0";
  const char * port = "1917";

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  cout << "Connecting to " << hostname << " on port " << port << "..." << endl;

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;

    return -1;
  }  //if

  const char * message = "GET / HTTP/1.1\r\nHost: "
                         "www.google.com\r\n\r\n";
  cout << "Client sent: " << message << endl;
  /*
  const char * target_hostname = "www.bing.com";
  const char * target_port = "80";
  int x = strlen(target_hostname);
  send(socket_fd, &x, sizeof(int), 0);
  send(socket_fd, target_hostname, strlen(target_hostname), 0);
  int y = strlen(target_port);
  send(socket_fd, &y, sizeof(int), 0);
  send(socket_fd, target_port, strlen(target_port), 0);
  
  */
  send(socket_fd, message, strlen(message), 0);

  char buffer[65535];

  recv(socket_fd, buffer, 65535, 0);
  buffer[65535] = 0;
  cout << "Client received: " << buffer << endl;

  freeaddrinfo(host_info_list);

  close(socket_fd);
  // }
  return 0;
}
