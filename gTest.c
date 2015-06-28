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

void speechText(char*);
void callGoogleSpeechAPI();
CURL* HTTPSession(char*, struct Buffer*); 
size_t buffer_writer(char*, size_t, size_t, void*);
char* splitResultString(char*);

int main(){
    callGoogleSpeechAPI();

    return 0;
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
    speechText(result);

    free(buf->data);
    free(buf);

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

CURL* HTTPSession(char* url, struct Buffer *buf){
    CURL *curl;

    struct curl_httppost* post = NULL;
    struct curl_httppost* last = NULL;
    struct curl_slist *headerlist = NULL;

    //strcpy(url,"https://www.google.com/speech-api/v2/recognize?xjerr=1&client=chromium&lang=en&maxresults=10&pfilter=0&xjerr=1&key=AIzaSyDoB9Kn2IUJdJ1SvBGIdrYLdGvhEnttE-4");
    //curlの初期化
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