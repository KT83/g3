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

char*   callGoogleSpeechAPI();                              // get text from voice data
char*   callGoogleTranslatorAPI(char*, char*, char*);       // get translated text.(text, from, to)
size_t  buffer_writer(char*, size_t, size_t, void*);        // callback func for SpeechAPI http session
char*   splitResultString(char*);                           // split the result string from google speech api
char*   getAimedStringFromText(char*, char*);               // generalize above
void    speechText(char*);                                  // speech text with say command
char*   makeStringForParam(char*);                          // convert plain text to the one suite for GET param
        
const int n_length = 1;   // length of bytes to send


int main(int argc, char** argv) {
    char *text1 = callGoogleSpeechAPI();
    printf("text1 : %s\n", text1);

    char *text2 = callGoogleTranslatorAPI("hello hello  ", "en", "ja");
    printf("text2 : %s\n", text2);

    speechText(text2);

    return 0;
}


char* callGoogleSpeechAPI(){
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

char* callGoogleTranslatorAPI(char* text, char* from, char* to){
    struct Buffer *buf;
    buf = (struct Buffer *)malloc(sizeof(struct Buffer));
    buf->data = NULL;
    buf->data_size = 0;

    char *textForURL = makeStringForParam(text);

    printf("%s\n", textForURL);

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
    char *demoString = "{\"data\": {\"translations\": [{\"translatedText\": \"よく私はAMERICの米国がある今夜リベラルアメリカと保守的なアメリカがない彼らに言います\",\"detectedSourceLanguage\": \"en\"}]}";
    //printf("%s\n", buf->data);
    char *result = getAimedStringFromText("translatedText",buf->data);

    free(buf->data);
    free(buf);

    FILE *fp;   /* (1)ファイルポインタの宣言 */
    /* (2)ファイルのオープン */
    /*  ここで、ファイルポインタを取得する */
    if ((fp = fopen("translated.txt", "w")) == NULL) {
        printf("file open error!!\n");
        exit(EXIT_FAILURE); /* (3)エラーの場合は通常、異常終了する */
    }
    
    int sizeOfResult = strlen(result);

    fwrite(result, sizeof(char), sizeOfResult, fp);
    
    fclose(fp); /* (5)ファイルのクローズ */

    char *cmdline;
    cmdline = (char *)malloc(sizeof(char) * 128);
    sprintf(cmdline, "nkf -w --overwrite translated.txt ");
    fp=popen(cmdline, "w");
    free(cmdline);
    if(fp == NULL) {
        (void)pclose(fp);
        return 0;
    }
    (void)pclose(fp);

    return result;
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


void speechText(char* text){
    FILE *fp;
    char *cmdline;
    cmdline = (char *)malloc(sizeof(char) * 256);
    sprintf(cmdline, "say -v Kyoko -f translated2.txt");
    fp=popen(cmdline, "w");
    free(cmdline);
    if(fp == NULL) {
        (void)pclose(fp);
    }
    (void)pclose(fp);
}
