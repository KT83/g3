#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

void* thread_func(void*);   /* function exected in sub thread */
int makeSocket(char*);
int startRecAndRead(int);   /* start read with popen func */
void sendData(int, int);


int main(int argc, char** argv) {
  const int n_length = 1;   /* length of bytes to send */

  int pid;
  pthread_t thread;
  pid = getpid();

  int s = -2;               /* means not connected */
  s = makeSocket(argv[1]);

  pthread_create(&thread, NULL, &thread_func, &s);


  if (s == -1) {
    perror("accept\n");
    exit(1); 
  }else if(s >= 0){
    if(startRecAndRead(s) == 0){
      printf("popen error\n");
    }else{
      sendData(s, n_length);
    }
  }
  close(s);
}



int makeSocket(char* portNo){
  int ss = socket(PF_INET, SOCK_STREAM, 0);
  if (ss == -1) {
    perror("socket");
    exit(1);
  }


  //bind
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  if ((addr.sin_port = htons(atoi(portNo))) == -1) { //port_number
    printf("port error");
    exit(1);
  }
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(ss, (const struct sockaddr*)&addr, sizeof(addr)) == -1) {
    exit(1);
  }

  //listen
  if (listen(ss, 10) == -1){
    perror("listen");
    exit(1);
  }

  //accept
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(struct sockaddr_in);
  int s = accept(ss, (struct sockaddr*)&client_addr, &len);
  close(ss);
  return s;
}

void * thread_func(void * s){
  int *sock = (int *)s;
  printf("%d\n", *sock);
  const long n_length = 1;
  int n;
  short *buff;
  buff = (short *)malloc(sizeof(short) * n_length);
  while(1){
    int n = read(*sock, buff, n_length);
    if(n == -1){
      perror("read");
      exit(1);
    }
    if(n == 0) break;

    write(1, buff, n_length);
  }
}

int startRecAndRead(int s){
  FILE *fp;
  char *cmdline;
  cmdline = (char *)malloc(sizeof(char) * 128);
  sprintf(cmdline, "rec -t raw -b 16 -c 1 -e s -r 44100 - | ./read %d", s);
  fp=popen(cmdline, "w");
  free(cmdline);
  if(fp == NULL) {
    (void)pclose(fp);
    return 0;
  }
  (void)pclose(fp);
  return 1;
}

void sendData(int s, int n_length){
  short *buff;
  buff = (short *)malloc(sizeof(short) * n_length);
  while(1){
    fprintf(stderr, "pthread");
    int n = read(0, buff, n_length);
    int k = send(s, buff, n_length, 0);
    if(n == 0) return;
  }
}
