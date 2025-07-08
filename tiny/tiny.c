/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd; // 파일 디스크립터
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 듣기 소켓 오픈
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept // 반복적으로 연결 요청을 접수하고
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit // 트랜잭션을 수행하고
    Close(connfd); // line:netp:tiny:close // 자신의 연결을 끊는다.
  }
}

void serve_dynamic(int fd, char *filename, char *cgiargs) //
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  sprintf(buf, "Content-Type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Connection: close\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) // Fork 시그널로 새로운 자식 프로세스 생성 // 반환값이 0인 쪽이 자식.
  {
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);                      // 프로세스의 환경변수를 추가
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */ // 파일 디스크립터 복사, 표준출력을 클라이언트와 연결된 연결식별자로 재지정한다.
    // 입출력 redirection // Dup2는 표준 출력을 클라이언트 소켓에 연결해둔다.
    Execve(filename, emptylist, environ); /* Run CGI program */ // 자식 프로세스에서 execve로 cgi 바이너리를 실행한 후에 동적 컨텐츠를 출력한다.
  }
  Wait(NULL); /* Parent waits for and reaps child */
}

// serve_static 함수는 정적 파일을 클라이언트에 서비스한다.
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  // sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  // sprintf(buf, "%sConnection: close\r\n", buf);
  // sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  // sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf + strlen(buf), "Server: Tiny Web Server\r\n"); // sprintf는 문자열 버퍼에 저장한다.
  sprintf(buf + strlen(buf), "Connection: close\r\n");
  sprintf(buf + strlen(buf), "Content-length: %d\r\n", filesize);
  sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n", filetype);

  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers::::\n");
  printf("%s", buf);

  /* Send response body to client */
  // srcfd = Open(filename, O_RDONLY, 0);                        // Open 함수로 요청된 파일을 읽기 전용으로 열고 파일 디스크립터를 얻는다.
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // Mmap은 파일의 내용을 명시적으로 복사하지 않고, 파일의 페이지들을 프로세스의 가상 주소 공간에 "연결"한다.
  // Close(srcfd);                                               // 이 시점에서 파일 디스크립터는 더 이상 필요 없으므로, Close(srcfd)로 즉시 닫는다.
  // Rio_writen(fd, srcp, filesize);                             // mmap이 반환한 포인터(srcp)가 가리키는 메모리 영역에서 filesize 만큼의 데이터를 클라이언트의 소켓 디스크립터로 직접 전송한다.
  // Munmap(srcp, filesize);                                     // 파일 전송이 완료된 후, Munmap 함수로 mmap으로 생성된 가상 메모리 영역을 해제한다.
  // mmap 방식은

  /* malloc으로 구현 */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
}

/* get_filetype - Derive file type from filename */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mpg"))
  {
    strcpy(filetype, "video/mpeg");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin")) // haystack - needle
  {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  {
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
    {
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n ",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);           // 초기화
  Rio_readlineb(&rio, buf, MAXLINE); // 라인 읽기
  printf("Reqeust headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny coundn't find this file");
    return;
  }

  if (is_static)
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}
