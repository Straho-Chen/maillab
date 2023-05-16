#include "base64_utils.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_SIZE 4095

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

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char *receiver, const char *subject, const char *msg,
               const char *att_path) {
  const char *end_msg = "\r\n.\r\n";
  const char *host_name = "smtp.qq.com"; // Specify the mail server domain name
  const unsigned short port = 25;        // SMTP server port
  const char *user = "783356434@qq.com"; // Specify the user
  const char *pass = "vcnsolkttuynbcae"; // Specify the password
  const char *from = "straho@qq.com"; // Specify the mail address of the sender
  char dest_ip[16];                   // Mail server IP address
  int s_fd;                           // socket file descriptor
  struct hostent *host;
  struct in_addr **addr_list;
  int i = 0;
  int r_size;

  // general end
  const char *end = "\r\n";
  // boundary
  const char *boundary_bare = "qwertyuiopasdfghjklzxcvbnm";
  const char *boundary = "--qwertyuiopasdfghjklzxcvbnm\r\n";

  // Get IP from domain name
  if ((host = gethostbyname(host_name)) == NULL) {
    herror("gethostbyname");
    exit(EXIT_FAILURE);
  }

  addr_list = (struct in_addr **)host->h_addr_list;
  while (addr_list[i] != NULL)
    ++i;
  strcpy(dest_ip, inet_ntoa(*addr_list[i - 1]));

  // Create a socket, return the file descriptor to s_fd, and establish a TCP
  // connection to the mail server
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

  // Send EHLO command and print server response
  const char *EHLO = str_connect("EHLO qq.com", end); // Enter EHLO command here
  send(s_fd, EHLO, strlen(EHLO), 0);
  // Print server response to EHLO command
  general_print(s_fd);

  // Authentication. Server response should be printed out.
  const char *AUTH = str_connect("AUTH login", end);
  send(s_fd, AUTH, strlen(AUTH), 0);
  general_print(s_fd);

  const char *user_encode = str_connect(encode_str(user), end);
  send(s_fd, user_encode, strlen(user_encode), 0);
  general_print(s_fd);

  const char *password_encode = str_connect(encode_str(pass), end);
  send(s_fd, password_encode, strlen(password_encode), 0);
  general_print(s_fd);

  // Send MAIL FROM command and print server response
  const char *MAIL_FROM =
      str_connect(str_connect("MAIL FROM:<", str_connect(from, ">")), end);
  send(s_fd, MAIL_FROM, strlen(MAIL_FROM), 0);
  general_print(s_fd);

  // Send RCPT TO command and print server response
  const char *RCPT_TO =
      str_connect(str_connect("RCPT TO:<", str_connect(receiver, ">")), end);
  send(s_fd, RCPT_TO, strlen(RCPT_TO), 0);
  general_print(s_fd);

  // Send DATA command and print server response
  const char *DATA = str_connect("DATA", end);
  send(s_fd, DATA, strlen(DATA), 0);
  general_print(s_fd);

  // Send message data
  const char *FROM = str_connect(str_connect("From: ", from), end);
  send(s_fd, FROM, strlen(FROM), 0);

  const char *TO = str_connect(str_connect("To: ", receiver), end);
  send(s_fd, TO, strlen(TO), 0);

  const char *MIME_VERSION = str_connect("MIME-Version: 1.0", end);
  send(s_fd, MIME_VERSION, strlen(MIME_VERSION), 0);

  const char *CONTENT_DESCRIPTION =
      str_connect("Content-Description: HITSZ Mail Lab test", end);
  send(s_fd, CONTENT_DESCRIPTION, strlen(CONTENT_DESCRIPTION), 0);

  const char *CONTENT_TYPE = str_connect(
      str_connect("Content-Type: multipart/mixed; boundary=", boundary_bare),
      end);
  send(s_fd, CONTENT_TYPE, strlen(CONTENT_TYPE), 0);

  const char *SUBJECT =
      str_connect(str_connect(str_connect("Subject: ", subject), end), end);
  send(s_fd, SUBJECT, strlen(SUBJECT), 0);

  send(s_fd, boundary, strlen(boundary), 0);
  // try to read the file from msg.
  // If failed, then msg is a string which is ready to be sent.
  CONTENT_TYPE = str_connect(str_connect("Content-Type: text/plain", end), end);
  send(s_fd, CONTENT_TYPE, strlen(CONTENT_TYPE), 0);

  char MSG[1024];
  FILE *f_msg = fopen(msg, "r");
  if (f_msg == NULL) {
    strcpy(MSG, str_connect(msg, end));
  } else {
    fseek(f_msg, 0L, SEEK_END);
    int len = ftell(f_msg);
    fseek(f_msg, 0L, SEEK_SET);
    char *file_content = (char *)malloc(len + 1);
    fread(file_content, sizeof(char), len, f_msg);
    file_content[len] = '\0';
    fclose(f_msg);
    strcpy(MSG, file_content);
  }
  const char *MSG_BODY = str_connect(str_connect(MSG, end), end);
  send(s_fd, MSG_BODY, strlen(MSG_BODY), 0);

  send(s_fd, boundary, strlen(boundary), 0);

  const char *ENCODE = str_connect("Content-Transfer-Encoding: base64", end);
  send(s_fd, ENCODE, strlen(ENCODE), 0);

  CONTENT_TYPE = str_connect(
      str_connect(str_connect("Content-Type: application/octet-stream; name=",
                              att_path),
                  end),
      end);
  send(s_fd, CONTENT_TYPE, strlen(CONTENT_TYPE), 0);

  // send attchment
  char ATTACHMENT[1024];
  FILE *f_attachment = fopen(att_path, "rb");
  if (f_attachment == NULL) {
    fprintf(stderr, "File %s not found\n", att_path);
    exit(EXIT_FAILURE);
  } else {
    FILE *f_encode = fopen("file_base64.txt", "wb+");
    encode_file(f_attachment, f_encode);
    fclose(f_attachment);
    fseek(f_encode, 0L, SEEK_END);
    int len = ftell(f_encode);
    fseek(f_encode, 0L, SEEK_SET);
    char *file_content = (char *)malloc(len + 1);
    fread(file_content, sizeof(char), len, f_encode);
    file_content[len] = '\0';
    fclose(f_encode);
    strcpy(ATTACHMENT, file_content);
  }
  const char *ATTACHMENT_BODY = str_connect(str_connect(ATTACHMENT, end), end);
  send(s_fd, ATTACHMENT_BODY, strlen(ATTACHMENT_BODY), 0);

  send(s_fd, boundary, strlen(boundary), 0);

  // Message ends with a single period
  send(s_fd, end_msg, strlen(end_msg), 0);
  general_print(s_fd);

  // Send QUIT command and print server response
  const char *QUIT = str_connect("QUIT", end);
  send(s_fd, QUIT, strlen(QUIT), 0);
  general_print(s_fd);

  close(s_fd);
}

int main(int argc, char *argv[]) {
  int opt;
  char *s_arg = NULL;
  char *m_arg = NULL;
  char *a_arg = NULL;
  char *recipient = NULL;
  const char *optstring = ":s:m:a:";
  while ((opt = getopt(argc, argv, optstring)) != -1) {
    switch (opt) {
    case 's':
      s_arg = optarg;
      break;
    case 'm':
      m_arg = optarg;
      break;
    case 'a':
      a_arg = optarg;
      break;
    case ':':
      fprintf(stderr, "Option %c needs an argument.\n", optopt);
      exit(EXIT_FAILURE);
    case '?':
      fprintf(stderr, "Unknown option: %c.\n", optopt);
      exit(EXIT_FAILURE);
    default:
      fprintf(stderr, "Unknown error.\n");
      exit(EXIT_FAILURE);
    }
  }

  if (optind == argc) {
    fprintf(stderr, "Recipient not specified.\n");
    exit(EXIT_FAILURE);
  } else if (optind < argc - 1) {
    fprintf(stderr, "Too many arguments.\n");
    exit(EXIT_FAILURE);
  } else {
    recipient = argv[optind];
    send_mail(recipient, s_arg, m_arg, a_arg);
    exit(0);
  }
}
