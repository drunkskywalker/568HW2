#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
int main() {

  int status;
  

  int recv_fd;
  struct addrinfo recv_host_info;
  struct addrinfo * recv_host_info_list;  
  const char * recv_hostname = NULL;
  const char * recv_port = "1917";
  
 memset(&recv_host_info, 0, sizeof(recv_host_info));

  recv_host_info.ai_family   = AF_UNSPEC;
  recv_host_info.ai_socktype = SOCK_STREAM;
  recv_host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(recv_hostname, recv_port, &recv_host_info, &recv_host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
    return -1;
  } //if

  recv_fd = socket(recv_host_info_list->ai_family, 
		     recv_host_info_list->ai_socktype, 
		     recv_host_info_list->ai_protocol);
  if (recv_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
    return -1;
  } //if

  int yes = 1;
  status = setsockopt(recv_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(recv_fd, recv_host_info_list->ai_addr, recv_host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
    return -1;
  } //if

  status = listen(recv_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl; 
    cerr << "  (" << recv_hostname << "," << recv_port << ")" << endl;
    return -1;
  } //if

  cout << "Waiting for connection on port " << recv_port << endl;
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connection_fd;
  client_connection_fd = accept(recv_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return -1;
  } //if  
  
  char buffer[65535];
  recv(client_connection_fd, buffer, 65535, 0);
  buffer[65535] = 0;

  cout << "Server received: " << buffer << endl;
  
  free(recv_host_info_list);
  
  
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  const char * dest_hostname = "www.google.com";
  const char * dest_port = "80";  
  
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(dest_hostname, dest_port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << dest_hostname << "," << dest_port << ")" << endl;
    return -1;
  }  //if

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << dest_hostname << "," << dest_port << ")" << endl;
    return -1;
  }  //if

  cout << "Connecting to " << dest_hostname << " on port " << dest_port << "..." << endl;

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << dest_hostname << "," << dest_port << ")" << endl;

    return -1;
  }  //if

  //const char * message = "GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n";
  cout << "Client sent: " << buffer << endl;
  send(socket_fd, buffer, strlen(buffer), 0);

  memset(buffer, 65535, 0);
  recv(socket_fd, buffer, 65535, 0);
  buffer[65535] = 0;
  cout << "Client received: " << buffer << endl;
  
  
  send(client_connection_fd, buffer, 65535, 0);
  cout << "Server sent: " << buffer << endl;
  freeaddrinfo(host_info_list);

// close(socket_fd);

  return 0;
}