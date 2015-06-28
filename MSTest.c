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

        int startPoint = idx1 - text;
        int endPoint = idx2 - text;
        int length = endPoint - startPoint + 1;
        int i;
        for(i = 0; i < length; i++){
            newString[i] = text[startPoint + i];
        }
    }    

    return newString;
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

void callGoogleTranslatorAPI(char* text, const char* from, const char* to){
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

    // return result;
    
}

int main(){

    callGoogleTranslatorAPI("well I say to them tonight there is not a liberal America and a conservative America there is the United States of America", "en", "jp");

    return 0;
}



