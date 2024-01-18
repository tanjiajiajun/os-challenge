#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/sha.h>

int main(){
    // Binary representation with CTX
    // Input
    uint64_t number_to_hash_ctx = 256;

    // Output
    unsigned char hash_of_number_ctx[SHA256_DIGEST_LENGTH];
    bzero(hash_of_number_ctx, sizeof(hash_of_number_ctx));

    // Hashing  
    SHA256_CTX sha256_num; 
    SHA256_Init(&sha256_num);
    SHA256_Update(&sha256_num, &number_to_hash_ctx, sizeof(number_to_hash_ctx));
    SHA256_Final(hash_of_number_ctx, &sha256_num);

    // Print the hash
    printf("First method using binary representation of the number and CTX\n");
    printf("Hash of the number: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        printf("%02x ", hash_of_number_ctx[i]);
    }
    printf("\n");

    // Binary representation with SH256
    // Input
    uint64_t number_to_hash_sh256 = 256;

    // Output
    unsigned char hash_of_number_sh256[SHA256_DIGEST_LENGTH];
    bzero(hash_of_number_sh256, sizeof(hash_of_number_sh256));

    // Hashing
    SHA256(&number_to_hash_sh256, sizeof(number_to_hash_sh256), hash_of_number_sh256);

    // Print the hash
    printf("Second method using binary representation of the number and SH256\n");
    printf("Hash of the number: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        printf("%02x ", hash_of_number_sh256[i]);
    }
    printf("\n");

    // String representation with CTX
    // Input
    unsigned char string_to_hash_ctx[] = "256";

    // Output    
    unsigned char hash_of_string_ctx[SHA256_DIGEST_LENGTH];
    bzero(hash_of_string_ctx, sizeof(hash_of_string_ctx));

    // Hashing
    SHA256_CTX sha256_str;
    SHA256_Init(&sha256_str);
    SHA256_Update(&sha256_str, string_to_hash_ctx, strlen(string_to_hash_ctx));
    SHA256_Final(hash_of_string_ctx, &sha256_str);

    // Print the hash
    printf("Third method using string representation of the number and CTX\n");
    printf("Hash of the number: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        printf("%02x ", hash_of_string_ctx[i]);
    }
    printf("\n");

    // String representation with SH256
    // Input
    unsigned char string_to_hash_sh256[] = "256";

    // Output
    unsigned char hash_of_string_sh256[SHA256_DIGEST_LENGTH];
    bzero(hash_of_string_sh256, sizeof(hash_of_string_sh256));

    // Hashing
    SHA256(string_to_hash_sh256, strlen(string_to_hash_sh256), hash_of_string_sh256);

    // Print the hash
    printf("Fourth method using string representation of the number and SH256\n");
    printf("Hash of the number: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        printf("%02x ", hash_of_string_sh256[i]);
    }
    printf("\n");

    return 0;
}