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

int n_length = 1;

void startRead(int s){
    short *buff;
    buff = (short *)malloc(sizeof(short) * n_length);
    while(1){
        int n = read(0, buff, n_length);
        int k = send(s, buff, n_length, 0);
        if(n == 0){
            break;
        }
    }
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

int main(int argc, char** argv) {
 int pid;
 pthread_t thread;
 pid = getpid();

 int ss = socket(PF_INET, SOCK_STREAM, 0);
 if (ss == -1) {
  perror("socket");
  exit(1);
 }
 

 //bind
 struct sockaddr_in addr;
 addr.sin_family = AF_INET;
 if ((addr.sin_port = htons(atoi(argv[1]))) == -1) { //port_number
  printf("port error");
  exit(1);
 }
 addr.sin_addr.s_addr = INADDR_ANY;
 if (bind(ss, (const struct sockaddr*)&addr, sizeof(addr)) == -1) {
//  perror("bind");
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
 int s = -2;
 s = accept(ss, (struct sockaddr*)&client_addr, &len);

  pthread_create(&thread, NULL, &thread_func, &s);


 if (s == -1) {
   perror("accept\n");
   exit(1); 
 }else if(s >= 0){
     FILE *fp;
     char *cmdline;
     cmdline = (char *)malloc(sizeof(char) * 128);
     sprintf(cmdline, "rec -t raw -b 16 -c 1 -e s -r 44100 - | ./read %d", s);
     //cmdline = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
     if((fp=popen(cmdline, "w")) == NULL){
         printf("popen error\n");
    }
    (void)pclose(fp);
    free(cmdline);
 }


 //close
 close(ss);

 //close
 close(s);
}

