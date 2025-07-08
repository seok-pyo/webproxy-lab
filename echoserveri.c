#include "csapp.h" // 헤더 가져오기

void echo(int connfd); // 함수 원형 선언(prototype). 이 선언을 통해 링커가 연결해준다.

int main(int argc, char **argv) // argc 인자 개수, argv 문자열 배열로 전달된 인자를 담고 있음. 문자열들의 배열이기 때문에 이중포인터 사용
{
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE];

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  listenfd = Open_listenfd(argv[1]); // port 번호 > 8000 문자열을 Open_listenfd(argv[1])로 넘겨서 8000번 포트에서 리슨(listen) 하도록 설정.

  while (1)
  {
    clientlen = sizeof(struct sockaddr_storage);
    // connection이 들어올때까지 대기한다. accept 함수가 대기할 수 있도록 한다.
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // accept 함수를 호출해서 클라이언트로부터의 응답을 기다린다.
    // socket 구조체로 타입 캐스팅
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    echo(connfd);
    Close(connfd); // socket, 연결된 스트림만 끊긴다.
  }
  exit(0);
}
