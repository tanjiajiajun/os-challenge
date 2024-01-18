// built upon example code from here: https://groups.google.com/g/mailing.openssl.users/c/QjC9p14dOGI?pli=1
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/sha.h>

int main(){
    /*
    Pseudo code for next steps to add to this code:
    - iterate through a start/end value
    - take put it through the SHA256 to generate a hash
    - compare hash values
    - if hash value not equal, move to next number
    - if entire hash value is equal stop and return this number found
    */
    unsigned char ibuf[] = "102";
    unsigned char obuf[SHA256_DIGEST_LENGTH];

    printf("%s","testing buffer 1\n");

    SHA256(ibuf, strlen(ibuf), obuf);

    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x ", obuf[i]);
    }
    printf("\n");

    int start_num = 100;
    int end_num = 104;
    for(int test_num = start_num; test_num<=end_num; test_num++){
        //unsigned char ibuf2[] = "102";
        unsigned char ibuf2[5]; //placeholder size for now
        unsigned char obuf2[SHA256_DIGEST_LENGTH];

        sprintf(ibuf2, "%d", test_num);

        printf("%s","testing buffer 2\n");

        SHA256(ibuf2, strlen(ibuf2), obuf2);
        int j;
        for (j = 0; j < SHA256_DIGEST_LENGTH; j++) {
            printf("%02x ", obuf2[j]);
        }
        printf("\n");
        bool same = true;
        for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            if(obuf[i]!=obuf2[i]){
                same = false;
                break;
            }

        }
        printf("%d\n", same);
        if(same){
            printf("%d\n",test_num);
            printf("%s", "found the number\n");
            break;
        }
    }

    return 0;
}