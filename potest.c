#include <stdio.h>
#include <stdlib.h>

int main(){
    FILE    *fp;
    char    *cmdline = "./ctest";
    if( (fp=popen(cmdline, "w")) == NULL){
        printf("somthing wrong\n");
    }
    int n_length = 1;
    short *buff;
    buff = (short *)malloc(sizeof(short) * n_length);
   /* while(1){
	printf("its inside the loop!\n");
        int n = read(0, buff, n_length);
        if(n == 0){
            break;
        }
	printf("%c", *buff);
    }*/
    

    (void) pclose(fp);
    return 0;
}
