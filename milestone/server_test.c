#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <endian.h>

#include <netdb.h>
#include <netinet/in.h>

#include <openssl/sha.h>

#define MESSAGE_SIZE 49


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

int main(char argc, char *argv[]){
    int serverSocket, clientSocket, portno;
    int n, bytesRead;
    unsigned char whole_message[MESSAGE_SIZE]; // buffer for storing message

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

    // Receiving the message
    bzero(whole_message, MESSAGE_SIZE);
    bytesRead = read(clientSocket, whole_message, sizeof(whole_message));
    if(bytesRead != MESSAGE_SIZE){
        perror("Error reading message");
        exit(1);
    }

    printf("Here is the whole message: %s\n", whole_message);
    printf("Here is the hex format: ");
    for(int i = 0; i < MESSAGE_SIZE; i++){
        printf("%02x ", whole_message[i]);
    }
    printf("\n");

    // Parsing the message
    uint8_t hash_number[SHA256_DIGEST_LENGTH];
    uint64_t start_nbo, end_nbo;
    uint64_t start, end;
    uint8_t priority;

    memcpy(hash_number, whole_message, SHA256_DIGEST_LENGTH);
    memcpy(&start_nbo, whole_message + SHA256_DIGEST_LENGTH, sizeof(start_nbo));
    memcpy(&end_nbo, whole_message + SHA256_DIGEST_LENGTH + sizeof(start_nbo), sizeof(end_nbo));
    memcpy(&priority, whole_message + SHA256_DIGEST_LENGTH + sizeof(start_nbo) + sizeof(end_nbo), sizeof(priority));

    start = be64toh(start_nbo);
    end = be64toh(end_nbo);

    printf("Here is the message after parsing:\n");
    printf("Hash: %s\n", hash_number);
    printf("Hash in hex: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        printf("%02x ", hash_number[i]);
    }
    printf("\n");
    printf("Start in nbo: %02lx\n", start_nbo);
    printf("Start in hbo: %02lx\n", start);
    printf("End in nbo: %02lx\n", end_nbo);
    printf("End in hbo: %02lx\n", end);
    printf("Priority: %d\n", priority);

    // Reverse hashing
    uint64_t reversed_hash;
    reversed_hash = reverse_hash(start, end, hash_number);

    printf("Here is the uint64_t number: %lu \n", reversed_hash);

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