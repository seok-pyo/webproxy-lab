#include "csapp.h"

// echo client의 메인 루틴
int main(int argc, char **argv) // argc: argument 프로그램의 이름을 포함한 전체 인자 개수 / count argv: argument vector 문자열 포인터들의 배열
{
  int clientfd;
  char *host, *port, buf[MAXLINE]; // 어플리케이션 버퍼
  rio_t rio;

  if (argc != 3)
  {
    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    exit(0);
  }
  host = argv[1]; // 입력값 1
  port = argv[2]; // 입력값 2

  /*
  open client를 하게 되면 'getaddrinfo'를 이용해서 주소 리스트를 만들고, 해당 리스트를 순회하면서 연결을 시도한다. 사용한 주소 리스트는
  바로 free로 정리해준다. */
  /*
  getaddrinfo는 소켓 주소 구조체들과 호스트 이름, 호스트 주소, 서비스 이름, 포트번호과 사이에서 변환을 해준다.
  */
  clientfd = Open_clientfd(host, port); // client 연결 // socket - connect 연결을 맺음. 해당 소켓에 대한 디스크립터를 리턴.

  Rio_readinitb(&rio, clientfd); // Rio 버퍼 초기화

  while (Fgets(buf, MAXLINE, stdin) != NULL) // fgets 함수는 버퍼에서 한 줄씩 읽어오는 함수 / read
  {
    Rio_writen(clientfd, buf, strlen(buf)); // 사용자 입력을 서버로 전송(write) // clientfd는 열린 파일 데이블의 인덱스를 의미
    // 여기서 fd가 clientfd이고 , 이는 커널이 추적하고 있는 소켓 객체(소켓 구조체)에 해당한다. 커널은 이 fd를 보고 네트워크 소켓인지, 어떤 서버와 연결되어 있는지 파악하고,
    // 그에 맞는 TCP 패킷을 구성해서 서버로 보낸다.
    // 열린 파일은 커널이 관리하는 I/O 자원, fd를 통해 접근 가능. // open(), socket(), pipe() 등의 시스템 콜로 생성되어야 한다.
    // 프로그램이 하는 일 : fd만 가지고 read, write 등을 수행. 커널이 하는 일 : fd를 실제 I/O 자원으로 매핑해서 처리
    Rio_readlineb(&rio, buf, MAXLINE); // 서버로부터 응답을 읽어와서 가져온다.
    Fputs(buf, stdout);                // write
  }
  Close(clientfd);
  exit(0);
}
