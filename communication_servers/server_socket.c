#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>

int main(int argc, char *argv[]){
    /*sockfd: file descriptor for the server socket*/
    /*newsockfd: file descriptor for the accepted clien socket (in accept())*/
    /*portno: server socket port number*/
    /*clilen: length of the structure sockaddr_in of the client*/
    int sockfd, newsockfd, portno, clilen;
    char buffer[256]; // buffer for storing message

    /*Structures for server and client address*/ 
    /*IMPORTANT: structure sockaddr_in is protocol specific for IPv4 address family*/
    /*IMPORTANT: structure sockaddr is the general structure valid for any protocol*/
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    /******************** Get a file descriptor for the specific socket that will be used ********************/
    /*Family: AF_INET (IPv4 protocol)*/
    /*Type: SOCK_STREAM (Stream socket)*/
    /*Protocol: 0 (selects the system's default protocol for the given combination of the family and type)*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        perror("ERROR opening socket");
        exit(1);
    }

    /*********************************** Initialize socket structure ***************************************/
    /*Set all bytes of serv_addr structure to 0 (bzero is for a string therefore the cast)*/
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]); // Get the port number from the user
    // portno = 5003; // port number

    serv_addr.sin_family = AF_INET; // Address family
    serv_addr.sin_addr.s_addr = INADDR_ANY; //32-bit IP address in NBO (if sin_addr.s_addr = INADDR_ANY means server's IP address will be assigned automatically)
    serv_addr.sin_port = htons(portno); // 16-bit port number in NBO (if port = 0, then system will choose a random port)

    /*********************** Assign a local protocol address to the socket (bind) **************************/
    /*Combine IPv4 address along with 16-bit TCP port number*/
    /*The socket address must be a struct sockaddr, so it must be cast*/
    n = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if(n == -1){
        perror("ERROR on binding");
        exit(1);
    }
    
    /******************************** Start listening for the clients **************************************/
    /*Convert an unconnected socket into a passive one (kernel should accept incoming connection requests directed to this socket)*/
    /*Second argument specifies the maximum # of connections that the kernel should queue for this specific socket*/
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    /*Process will go in sleep mode and will wait for the incoming connection*/

        
    /***************************** Accept actual connection from the client *********************************/
    /*sockfd: file descriptor of the server socket*/
    /*cli_addr: pointer to the structure socketaddr for the client socket (will contain client IP and port)*/
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if(newsockfd == -1){
        perror("ERROR on accept");
        exit(1);
    }
    /************************ Start communication after establishing connection *************************/
    bzero(buffer, sizeof(buffer));
    /*newsockfd: client socket file descriptor*/
    /*buffer: buffer where the data will be put into*/
    /*255: number of bytes to be read */
    n = read(newsockfd, buffer, 255);

    if(n == -1){
        perror("ERROR reading socket");
        exit(1);
    }

    printf("Here is the message: %s \n", buffer);
    bzero(buffer, sizeof(buffer));

    /******************************** Write response to the client *****************************************/
    /*newsockfd: client file descriptor to which the message will be send*/
    uint8_t i = 1;

    n = write(newsockfd, &i, sizeof(buffer));

    if(n == -1){
        perror("ERROR writing socket");
    }
    
    return 0;
}