#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_SIZE 65535

char buf[MAX_SIZE + 1];

char *str_connect(const char *first, const char *second) {
  char *result = malloc(strlen(first) + strlen(second) + 1); // need '\0'
  if (result == NULL)
    exit(EXIT_FAILURE);
  strcpy(result, first);
  strcat(result, second);
  return result;
}

void general_print(int s_fd) {
  int r_size;
  if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1) {
    perror("recv");
    exit(EXIT_FAILURE);
  }
  buf[r_size] = '\0'; // Do not forget the null terminator
  printf("%s", buf);
}

void recv_mail() {
  const char *host_name = "pop.qq.com";  // Specify the mail server domain name
  const unsigned short port = 110;       // POP3 server port
  const char *user = "783356434@qq.com"; // Specify the user
  const char *pass = "vcnsolkttuynbcae"; // Specify the password
  char dest_ip[16];
  int s_fd; // socket file descriptor
  struct hostent *host;
  struct in_addr **addr_list;
  int i = 0;
  int r_size;

  // general end
  const char *end = "\r\n";

  // Get IP from domain name
  if ((host = gethostbyname(host_name)) == NULL) {
    herror("gethostbyname");
    exit(EXIT_FAILURE);
  }

  addr_list = (struct in_addr **)host->h_addr_list;
  while (addr_list[i] != NULL)
    ++i;
  strcpy(dest_ip, inet_ntoa(*addr_list[i - 1]));

  // Create a socket,return the file descriptor to s_fd, and establish a TCP
  // connection to the POP3 server
  s_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (s_fd < 0) {
    perror("ERROR opening socket");
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(dest_ip);
  server_addr.sin_port = htons(port);
  bzero(server_addr.sin_zero, sizeof(unsigned char) * 8);
  connect(s_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));

  // Print welcome message
  general_print(s_fd);

  // Send user and password and print server response
  const char *USER = str_connect(str_connect("USER ", user), end);
  send(s_fd, USER, strlen(USER), 0);
  general_print(s_fd);

  const char *PASS = str_connect(str_connect("PASS ", pass), end);
  send(s_fd, PASS, strlen(PASS), 0);
  general_print(s_fd);

  // Send STAT command and print server response
  const char *STAT = str_connect("STAT", end);
  send(s_fd, STAT, strlen(STAT), 0);
  general_print(s_fd);

  // Send LIST command and print server response
  const char *LIST = str_connect("LIST", end);
  send(s_fd, LIST, strlen(LIST), 0);
  general_print(s_fd);

  // Retrieve the first mail and print its content
  const char *RETR = str_connect("RETR 1", end);
  send(s_fd, RETR, strlen(RETR), 0);
  general_print(s_fd);

  // Send QUIT command and print server response
  const char *QUIT = str_connect("QUIT", end);
  send(s_fd, QUIT, strlen(QUIT), 0);
  general_print(s_fd);

  close(s_fd);
}

int main(int argc, char *argv[]) {
  recv_mail();
  exit(0);
}
