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
#include <fcntl.h>

struct Buffer {
    char *data;
    int data_size;
};

void*   thread_func(void*);                                 // function executed in sub thread
void*   thread_func2(void*);                                 // function executed in sub thread
int     makeSocket(char*);                                  // making socket
int     startRecAndRead(int);                               // start read voice data with popen func
void    sendData(int, int);                                 // send recorded voice data
char*   callGoogleSpeechAPI(char*);                         // get text from voice data
void    callGoogleTranslatorAPI(char*, char*, char*);       // get translated text.(text, from, to)
size_t  buffer_writer(char*, size_t, size_t, void*);        // callback func for SpeechAPI http session
char*   splitResultString(char*);                           // split the result string from google speech api
char*   getAimedStringFromText(char*, char*);               // generalize above
void    speechText(char*);                                  // speech text with say command
char*   makeStringForParam(char*);                          // convert plain text to the one suite for GET param
        
const int n_length = 1;   // length of bytes to send
    
// file name for the text to translate is "toToranslate.txt"
// the one for the text translated is "translated.txt" 
// the one for voice data is "voice.raw" and "voice.flac"

int main(int argc, char** argv) {

    pthread_t thread;

    int s = -2;               // means not connected 
    s = makeSocket(argv[1]);
    
    // recieve and play peer voice data in sub thread
    // also, translate and speech will be done.
    pthread_create(&thread, NULL, &thread_func, &s);
    
    // send voice data in main thread
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
    }
  close(s);

  return 0;
}


// function executed in sub thread
void * thread_func(void * s){
    int *sock = (int *)s;
    short *buff;
    buff = (short *)malloc(sizeof(short) * n_length);

    int fileSize = 520000;
    int count = 0;
    while(1){
        int n = read(*sock, buff, n_length);
        if(n == -1){
            perror("read");
            exit(1);
        }
        if(n == 0) break;
        write(1, buff, n_length);

        count++;

        if(count == fileSize){
            fprintf(stderr, "API叩くよ！\n");
            count = 0;
            // remove voice.raw every 5sec(aproximately)
            FILE *fp;
            char *cmdline;
            cmdline = (char *)malloc(sizeof(char) * 128);

            sprintf(cmdline, "sox -b 16 -c 1 -e s -r 16000 voice.raw voice.flac");
            if((fp=popen(cmdline, "w")) == NULL){
                printf("popen error\n");
            }

            //sprintf(cmdline, "rm voice.raw");
            //if((fp=popen(cmdline, "w")) == NULL){
            //    printf("popen error\n");
            //}

            (void)pclose(fp);
            free(cmdline);

            // call speech API
            char *toTranslateString = callGoogleSpeechAPI("voice.flac");
            // call translate API
            callGoogleTranslatorAPI(toTranslateString, "en", "ja");
            // run 'say' command
            speechText("translated.txt");
        }
    }

}

// making socket
int makeSocket(char* portNo){
    int ss = socket(PF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("socket");
        exit(1);
    }

    //bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(portNo));

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


int startRecAndRead(int s){
    FILE *fp;
    char *cmdline;
    cmdline = (char *)malloc(sizeof(char) * 128);
    sprintf(cmdline, "rec -t raw -b 16 -c 1 -e s -r 16000 - | ./read %d", s);
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

char* callGoogleSpeechAPI(char* fileName){
    struct Buffer *buf;
    buf = (struct Buffer *)malloc(sizeof(struct Buffer));
    buf->data = NULL;
    buf->data_size = 0;

    CURL *curl;
    struct curl_httppost* post = NULL;
    struct curl_httppost* last = NULL;
    struct curl_slist *headerlist = NULL;

    char *url[256];
    strcpy(url,"https://www.google.com/speech-api/v2/recognize?xjerr=1&client=chromium&lang=en&maxresults=10&pfilter=0&xjerr=1&key=AIzaSyDoB9Kn2IUJdJ1SvBGIdrYLdGvhEnttE-4");


    curl = curl_easy_init();
    //接続先の設定
    headerlist = curl_slist_append(headerlist, "Content-Type:audio/x-flac; rate=16000");
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "voice.flac",  CURLFORM_FILE, "voice.flac", CURLFORM_END);
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

    //セッション開始
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    char *result = splitResultString(buf->data);
    // printf("%s\n", result);

    free(buf->data);
    free(buf);

    return result;
}

void callGoogleTranslatorAPI(char* text, char* from, char* to){
    struct Buffer *buf;
    buf = (struct Buffer *)malloc(sizeof(struct Buffer));
    buf->data = NULL;
    buf->data_size = 0;

    char *textForURL = makeStringForParam(text);

    char *url[256];
    sprintf(url,"https://www.googleapis.com/language/translate/v2?key=AIzaSyDoB9Kn2IUJdJ1SvBGIdrYLdGvhEnttE-4&target=ja&q=%s", textForURL);
    //char *url = "https://private-4d400-doodoo.apiary-mock.com/v1/shop/publish";


    CURL *curl;
    curl = curl_easy_init();
    //接続先の設定
    curl_easy_setopt(curl, CURLOPT_URL, url);

    //SSLのサーバー証明書の認証を行わない
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_writer);

    //セッション開始
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    //char *demoString = "{\"data\": {\"translations\": [{\"translatedText\": \"よく私はAMERICの米国がある今夜リベラルアメリカと保守的なアメリカがない彼らに言います\",\"detectedSourceLanguage\": \"en\"}]}";
    char *result = getAimedStringFromText("translatedText",buf -> data);
    printf("%s\n", result);

    free(buf->data);
    free(buf);

    // write out translated string to .txt file
    FILE *fp;
    if ((fp = fopen("translated.txt", "w")) == NULL) {
        printf("file open error!!\n");
        exit(EXIT_FAILURE);
    }
    int sizeOfResult = strlen(result);
    fwrite(result, sizeof(char), sizeOfResult, fp);
    fclose(fp);

    char *cmdline;
    cmdline = (char *)malloc(sizeof(char) * 128);
    sprintf(cmdline, "nkf -w --overwrite translated.txt ");
    fp=popen(cmdline, "w");
    free(cmdline);
    if(fp == NULL) {
        (void)pclose(fp);
    }
    (void)pclose(fp);

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

char* makeStringForParam(char *str){
    char *newString;
    int length = strlen(str);
    newString = (char *)malloc(sizeof(char) * length);
    int i;
    for(i = 0; i < length - 1; i++){
        if(str[i] == ' '){
            newString[i] = '+';
        }else{
            newString[i] = str[i];
        }
    }

    return newString;

}

char* getAimedStringFromText(char* str, char* txt){
    int textLength = strlen(txt);
    int aimedStringLength = strlen(str);
    char *aimedString;
    char *text;
    aimedString = (char*)malloc(sizeof(char) * aimedStringLength);
    text = (char*)malloc(sizeof(char) * textLength);
    strcpy(aimedString, str);
    strcpy(text, txt);
    char *newString;
    newString = (char*)malloc(sizeof(char) * textLength);

    
    char *idx1, *idx2;
    if((idx1 = strstr(text, aimedString)) != NULL){
        idx2 = strstr(text, ",");

        int startPoint = idx1 - text + aimedStringLength + 4;
        int endPoint = idx2 - text - 2;
        int length = endPoint - startPoint + 1;
        int i;
        for(i = 0; i < length; i++){
            newString[i] = text[startPoint + i];
        }
    }    

    return newString;
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


void speechText(char* fileName){
    FILE *fp;
    char *cmdline;
    cmdline = (char *)malloc(sizeof(char) * 256);
    sprintf(cmdline, "say -v Kyoko -f translated.txt");
    //sprintf(cmdline, "say -v Kyoko -f %s", fileName);
    fp=popen(cmdline, "w");
    free(cmdline);
    if(fp == NULL) {
        (void)pclose(fp);
    }
    (void)pclose(fp);
}
