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
#include <curl/curl.h>
#include <string.h>

struct Buffer {
    char *data;
    int data_size;
};

void* thread_func(void*);                               // function executed in sub thread
int makeSocket(char*);                                  // making socket
int startRecAndRead(int);                               // start read voice data with popen func
void sendData(int, int);                                // send recorded voice data
void callGoogleSpeechAPI();                             // get text from voice data
CURL* setCURLWithURLAndBuffer(char*, struct Buffer*);   // set curl for http session
size_t buffer_writer(char*, size_t, size_t, void*);     // callback func for SpeechAPI http session
char* splitResultString(char*);                         // split the result string from google speech api
void speechText(char*);                                 // speech text with say command
        
const int n_length = 1;   /* length of bytes to send */


int main(int argc, char** argv) {

    pthread_t thread;

    int s = -2;               // means not connected 
    s = makeSocket(argv[1]);
    
    pthread_create(&thread, NULL, &thread_func, &s);
    printf("thread created\n");
    if (s == -1) {
        perror("accept\n");
        exit(1); 
    }else if(s >= 0){
        printf("start rec and read\n");
        if(startRecAndRead(s) == 0){
            printf("popen error\n");
        }else{
            sendData(s, n_length);
        }
        printf("data sended\n");
    }
  close(s);

  return 0;
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
    /* 返り値が-1にはならないっぽい */
    if ((addr.sin_port = htons(atoi(portNo))) == 0) { //port_number
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
        
        // ここがキモ．
        FILE *fp;
        char *cmdline;
        cmdline = (char *)malloc(sizeof(char) * 128);
        sprintf(cmdline, "sox -b 16 -c 1 -e s -r 44100 write.raw write.flac");
        if((fp=popen(cmdline, "w")) == NULL){
            printf("popen error\n");
        }
        (void)pclose(fp);
        free(cmdline);

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
        send(s, buff, n_length, 0);
        if(n == 0) return;
    }
}

void callGoogleSpeechAPI(){
    struct Buffer *buf;
    buf = (struct Buffer *)malloc(sizeof(struct Buffer));
    buf->data = NULL;
    buf->data_size = 0;

    CURL *curl = HTTPSession("https://www.google.com/speech-api/v2/recognize?xjerr=1&client=chromium&lang=en&maxresults=10&pfilter=0&xjerr=1&key=AIzaSyDoB9Kn2IUJdJ1SvBGIdrYLdGvhEnttE-4", buf);

    //セッション開始
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    char *result = splitResultString(buf->data);
    printf("%s\n", result);

    free(buf->data);
    free(buf);

    printf("its here 4.\n");

}

size_t buffer_writer(char *ptr, size_t size, size_t nmemb, void *stream) {
    struct Buffer *buf = (struct Buffer *)stream;
    int block = size * nmemb;
    if (!buf) {
        return block;
    }

    if (!buf->data) {
        buf->data = (char *)malloc(block);
    }
    else {
        buf->data = (char *)realloc(buf->data, buf->data_size + block);
    }

    if (buf->data) {
        memcpy(buf->data + buf->data_size, ptr, block);
        buf->data_size += block;
    }

    return block;
}

char* splitResultString(char* resultString){
    char *str0, *str1, *str2, *str3, *stringToReturn;
    str0 = strtok(resultString,"}");
    str1 = strtok(NULL,":");
    str2 = strtok(NULL,":");
    str3 = strtok(NULL,":");
    stringToReturn = strtok(NULL, ",");

    return stringToReturn;
}

CURL* setCURLWithURLAndBuffer(char* url, struct Buffer *buf){
    CURL *curl;

    struct curl_httppost* post = NULL;
    struct curl_httppost* last = NULL;
    struct curl_slist *headerlist = NULL;

    curl = curl_easy_init();
    //接続先の設定
    headerlist = curl_slist_append(headerlist, "Content-Type:audio/x-flac; rate=16000");
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "obama.flac",  CURLFORM_FILE, "obama.flac", CURLFORM_END);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl,CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

    //SSLのサーバー証明書の認証を行わない
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_writer);

    return curl;
}

void speechText(char* text){
    FILE *fp;
    char *cmdline;
    cmdline = (char *)malloc(sizeof(char) * 256);
    sprintf(cmdline, "say -v Alex %s", text);
    fp=popen(cmdline, "w");
    free(cmdline);
    if(fp == NULL) {
        (void)pclose(fp);
    }
    (void)pclose(fp);
}
