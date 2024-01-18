#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <endian.h>

#include <netdb.h>
#include <netinet/in.h>

#include <openssl/sha.h>

/*
uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message, unsigned char *str_message){
    unsigned char temp_hash[SHA256_DIGEST_LENGTH];
    
    bzero(temp_hash, sizeof(temp_hash));
    bzero(str_message, sizeof(str_message));
    
    uint64_t test_num;
    for(test_num = start; test_num <= end; test_num++){
        snprintf(str_message, sizeof(str_message), "%lu", test_num);
        SHA256(str_message, strlen(str_message), temp_hash);

        bool same = true;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
            if(temp_hash[i] != hash_message[i]){
                same = false;
                break;
            }
        }
        if(same){
            for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
                printf("%02x ", temp_hash[i]);
            }
            printf("\n");
            break;
        }
    }
    return test_num;    
}
*/

uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message, unsigned char *str_message){
    unsigned char temp_hash[SHA256_DIGEST_LENGTH];

    bzero(temp_hash, sizeof(temp_hash));    
    bzero(str_message, sizeof(str_message));
    
    uint64_t test_num = 0;

    for(test_num = start; test_num < end; test_num++){
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, &test_num, sizeof(test_num));
        SHA256_Final(temp_hash, &sha256);

        printf("Test num: uint64_t %lu\n", test_num);
        printf("Hash in hex: ");
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
            printf("%02x ", temp_hash[i]);
        }
        printf("\n");

        bool same = true;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
            if(temp_hash[i] != hash_message[i]){
                same = false;
                break;
            }
        }

        if(same){
            snprintf(str_message, sizeof(str_message), "%lu", test_num);
            return test_num;
        }
    }
}

int main(char argc, char *argv[]){
    int serverSocket, clientSocket, portno;
    int n;
    unsigned char rcv_hash_msg[SHA256_DIGEST_LENGTH]; // buffer for storing message

    struct sockaddr_in serverAdrr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1){
        perror("Error creating socket");
        exit(1);
    }

    bzero((char *)&serverAdrr, sizeof(serverAdrr));
    portno = atoi(argv[1]);

    serverAdrr.sin_family = AF_INET;
    serverAdrr.sin_addr.s_addr = INADDR_ANY;
    serverAdrr.sin_port = htons(portno);

    n = bind(serverSocket, (struct sockaddr *)&serverAdrr, sizeof(serverAdrr));
    if(n == -1){
        perror("Error binding");
        exit(1);
    }

    listen(serverSocket, 5);
    printf("Server is listening on port %d\n", portno);

    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
    if(clientSocket == -1){
        perror("Error accepting connection");
        exit(1);
    }
    printf("Client connected\n");

    // Receiving message from client   
    bzero(rcv_hash_msg, SHA256_DIGEST_LENGTH);
    n = read(clientSocket, rcv_hash_msg, sizeof(rcv_hash_msg));

    printf("Here is the received hash: %s\n", rcv_hash_msg);
    printf("And here is the hex format: ");
    for(int i = 0; i < 32; i++){
        printf("%02x ", rcv_hash_msg[i]);
    }
    printf("\n");

    // Reversing the hash
    unsigned char rcv_str_msg[33]; // To store the string output
    uint64_t reversed_hash;

    reversed_hash = reverse_hash(0, 12, rcv_hash_msg, rcv_str_msg);

    printf("Here is the reverse hash: %s\n", rcv_str_msg);
    printf("And here is the uint64_t number: %lu \n", reversed_hash);

    // Send the result to the client in uint64_t format
    // Pass from host byte order to network byte order (le to be)
    uint64_t reversed_hash_nbo = htobe64(reversed_hash);
    printf("And here is the uint64_t number in network byte order (be): %lu \n", reversed_hash_nbo);

    n = write(clientSocket, &reversed_hash_nbo, sizeof(reversed_hash_nbo));
    if(n == -1){
        perror("Error sending result to client");
        exit(1);
    }

    close(clientSocket);

    return 0;
}