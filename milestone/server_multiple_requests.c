#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <endian.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>

#include "messages.h"

#define MAX_QUEUE_SIZE 100
#define MESSAGE_SIZE 49

// Structure to store the message's fields from the client
struct parsedMessage{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint64_t start;
    uint64_t end;
    uint8_t priority;
};

// Function to handle multiple requests from the client
void doprocessing(int sock);

// Function to parse the message from the client
struct parsedMessage parcing_msg(unsigned char *message);

// Function to reverse the hash by brute force
uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message);

uint8_t prio_score;

int main(char argc, char *argv[]){
    int serverSocket, clientSocket, portno;
    int bytesRead, pid; //n
    unsigned char whole_message[PACKET_REQUEST_SIZE];

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

    // n = bind(serverSocket, (struct sockaddr *)&serverAdrr, sizeof(serverAdrr));
    if(bind(serverSocket, (struct sockaddr *)&serverAdrr, sizeof(serverAdrr)) == -1){
        perror("Error binding socket");
        exit(1);
    }

    listen(serverSocket, 5);
    printf("Server is listening on port %d\n", portno);
    while(1){
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if(clientSocket == -1){
            perror("Error accepting connection");
            exit(1);
        }

        /* Create child process*/
        pid = fork();
        if(pid == -1){
            perror("Error on fork");
            exit(1);
        }

        if(pid == 0){ // This is the child process
            printf("prio score is %d/,", prio_score);
            int cue = setpriority(PRIO_PROCESS, 0, -prio_score);
            if (cue == -1) {
                perror("Error setting priority");
                exit(1);
            }
            close(serverSocket);
            doprocessing(clientSocket);
            exit(0);
        } else{
            close(clientSocket);
        }
    }
    return 0;
}

void doprocessing(int sock){
    int n, bytesRead;
    unsigned char whole_message[MESSAGE_SIZE];
    bzero(whole_message, MESSAGE_SIZE);

    bytesRead = read(sock, whole_message, sizeof(whole_message));

    if(bytesRead != MESSAGE_SIZE){
        perror("Error reading message");
        exit(1);
    }    

    struct parsedMessage parsed_message = parcing_msg(whole_message);
    prio_score = parsed_message.priority;
    uint64_t reversed_hash;
    reversed_hash = reverse_hash(parsed_message.start, parsed_message.end, parsed_message.hash);

    uint64_t reversed_hash_nbo = htobe64(reversed_hash);
    n = write(sock, &reversed_hash_nbo, sizeof(reversed_hash_nbo));
    if(n == -1){
        perror("Error writing to socket");
        exit(1);
    }
    
    close(sock);
}

struct parsedMessage parcing_msg(unsigned char *message){
    struct parsedMessage parsed_message;
    bzero(&parsed_message, sizeof(parsed_message));

    memcpy(parsed_message.hash, message, SHA256_DIGEST_LENGTH);
    memcpy(&parsed_message.start, message + SHA256_DIGEST_LENGTH, sizeof(uint64_t));
    memcpy(&parsed_message.end, message + SHA256_DIGEST_LENGTH + sizeof(uint64_t), sizeof(uint64_t));
    memcpy(&parsed_message.priority, message + SHA256_DIGEST_LENGTH + sizeof(uint64_t) + sizeof(uint64_t), sizeof(uint8_t));

    parsed_message.start = be64toh(parsed_message.start);
    parsed_message.end = be64toh(parsed_message.end);

    return parsed_message;
}

uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message){
    unsigned char temp_hash[SHA256_DIGEST_LENGTH];

    bzero(temp_hash, sizeof(temp_hash));

    uint64_t test_num = 0;
    for(test_num = start; test_num < end; test_num++){
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, &test_num, sizeof(test_num));
        SHA256_Final(temp_hash, &sha256);

        bool same = true;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
            if(temp_hash[i] != hash_message[i]){
                same = false;
                break;
            }
        }

        if(same){
            return test_num;
        }
    }
    return test_num;
}

