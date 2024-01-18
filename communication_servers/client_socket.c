#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>

#define OPTIONS "hs:i:"


void display_help(void){
        printf("A client that allows for communicatino with server client\n"
                "\nUSAGE\n ./server [-h] [s]\nOPTIONS\n  -h\tProgram Usage and help"
                "\n  -s\tSocker Number(defualt = 5002)\n");
}

void forwardHash(uint64_t number, unsigned char *hash) {
    bzero(hash, SHA256_DIGEST_LENGTH);

    SHA256_CTX sha256_num;
    SHA256_Init(&sha256_num);
    SHA256_Update(&sha256_num, &number, sizeof(number));
    SHA256_Final(hash, &sha256_num);
}

int main(int argc, char *argv[]){
    /*sockfd: file descriptor for the client socket*/
    /*portno: client socket port number*/
    int sockfd, portno, seed, diff, rep, delay, lambda; //, n;

    /*Setup parameters from argument*/
    seed = atoi(argv[3]);
    srand(seed);

    /*
    uint64_t number_to_send = 8;
    unsigned char hash_number [SHA256_DIGEST_LENGTH];

    forwardHash(number_to_send, hash_number);
    printf("Number to send: %lu\n", number_to_send);

    printf("Hash of the number: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        printf("%02x ", hash_number[i]);
    }
    printf("\n");
    */

    /*Structure for server address*/
    struct sockaddr_in serv_addr;

    /*Structure to keep information related to the host(server)*/
    struct hostent *server;

    char buffer[256]; // buffer for storing message

    // if(argc < 3){ // Check if the user provided the necessary arguments
    //     fprintf(stderr, "usage %s hostname port\n", argv[0]);
    //     exit(1);
    // }

    portno = atoi(argv[2]); // Get the port number from the user

    /************************** Create a file descriptor for the socket that will be used********************************/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        perror("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname(argv[1]); // Get the host(server) name from the user

    if(server == NULL){
        fprintf(stderr, "ERROR no such host\n");
        exit(1);
    }

    /*Set all bytes of serv_addr structure to 0 (bzero is for a string therefore the cast)*/
    bzero((char *)&serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; // Address family
    /*copies the nbytes(server->h_length) from server->h_addr to serv_addr.sin_addr.s_addr*/
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length); // Copy the server address to the serv_addr structure
    serv_addr.sin_port = htons(portno); // 16-bit port number in NBO

    /************************* Connect to the server ***********************/
    /*Used by TCP client to establisha connecton with a TCP server*/
    /*sockfd: socket client file descriptor */
    /*serv_addr: pointer to the strcture sockaddr that contains the destination port and address of the server*/
    /*addrlen: length of the structure pointed by serv_addr*/
    
    // n = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        perror("ERROR connecting");
        exit(1);
    }
 
    
    /************************* Get the user message ************************/
    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    /************************* Send message to the server ******************/
    /*sockfd: socket client file descriptor */
    /*buffer: pointer to the buffer containing the message*/
    /*strlen: length of the message*/
    // n = write(sockfd, buffer, strlen(buffer));

    if(write(sockfd, buffer, strlen(buffer)) == -1){
        perror("ERROR writing to socket");
        exit(1);
    }

    /************************* Read server response *************************/
    bzero(buffer, 256);
    // n = read(sockfd, buffer, 255);

    if(read(sockfd, buffer, 255) == -1){
        perror("ERROR reading from socket");
        exit(1);
    }

    printf("Message back: %s\n", buffer);
    
    return 0;
}
