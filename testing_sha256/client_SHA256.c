#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>

int main(char argc, char *argv[]){
    int clientSocket, portno;
    int n;

    struct sockaddr_in serverAddr;
    struct hostent *server;

    server = gethostbyname(argv[1]);
    portno = atoi(argv[2]);

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket == -1){
        perror("Error creating socket");
        exit(1);
    }

    server = gethostbyname(argv[1]);
    if(server == NULL){
        fprintf(stderr, "Error no such host");
        exit(1);
    }

    bzero((char *)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
    serverAddr.sin_port = htons(portno);

    n = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if(n == -1){
        perror("Error connecting to server");
        exit(1);
    }

    // Hash process through binary representation and CTX
    uint64_t number_to_send = 8;
    unsigned char hash_number [SHA256_DIGEST_LENGTH];
    bzero(hash_number, sizeof(hash_number));

    SHA256_CTX sha256_num;
    SHA256_Init(&sha256_num);
    SHA256_Update(&sha256_num, &number_to_send, sizeof(number_to_send));
    SHA256_Final(hash_number, &sha256_num);

    printf("Number to send: %lu\n", number_to_send);
    printf("Hash of the number: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        printf("%02x ", hash_number[i]);
    }
    printf("\n");

    n = write(clientSocket, hash_number, SHA256_DIGEST_LENGTH);
    if(n == -1){
        perror("Error sending hash to server");
        exit(1);
    }

    // Receiving the result from the server
    uint64_t result_nbo;
    uint64_t result_hbo;
    bzero((char *)&result_nbo, sizeof(result_hbo));

    n = read(clientSocket, &result_nbo, sizeof(result_hbo));
    if(n == -1){
        perror("Error receiving result from server");
        exit(1);
    }

    result_hbo = be64toh(result_nbo);
    printf("Result received from server in network byte order (be): %lu\n", result_nbo);
    printf("Result received from server in host byte order (le): %lu\n", result_hbo);

    return 0;
}