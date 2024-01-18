#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <endian.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include <netdb.h>
#include <netinet/in.h>

#include <openssl/sha.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include "messages.h"

/********************************General parameters***********************************/
#define MESSAGE_SIZE 49
#define MAX_QUEUE_SIZE 100
#define THREAD_POOL_SIZE 8
#define QUEUE_NUMBER 2
#define THRESHOLD 4

/********************************** Structures ***************************************/
// Structure to store request's fields from the client
struct parsedMessage{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint64_t start;
    uint64_t end;
    uint8_t priority;
    int sock;
};

// Structure for priority queues
struct PriorityQueue {
    struct parsedMessage requests[MAX_QUEUE_SIZE];
    int size;
    sem_t sem;
    int number;
};

/**************************** Functions for priority queue ****************************/
// Initialize the priority queue
void initPriorityQueue(struct PriorityQueue *queue, int i);

// Check if the priority queue is empty
int isEmpty(struct PriorityQueue *queue);

// Swap two elements in the priority queue
void swap(struct parsedMessage *a, struct parsedMessage *b);

// Enqueue an element into the priority queue
void enqueue(struct PriorityQueue *queue, struct parsedMessage request);

// Dequeue an element from the priority queue
struct parsedMessage dequeue(struct PriorityQueue *queue);

/**************************** Functions to handle requests ***************************/
// Read the message from the client
struct parsedMessage read_msg(int sock);

// Parse the message from the client
struct parsedMessage parsing_msg(unsigned char *message, int sock);

// Reverse hash by brute force
uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message);

// Write back the reversed hash to the client
void write_msg(struct parsedMessage parsed_message);

/**************************** Function to for threads ********************************/
// Handler for low priority requests from the client
void* low_priority_handler(void *queuePtr);

// Handler for high priority requests from the client
void* high_priority_handler(void *queuePtr);

// Handler for reading requests from the client
void* thread_reader(void *arg);

/********************************** Global variables *********************************/
// Declare the thread pool
pthread_t thread_pool[THREAD_POOL_SIZE];

// Declare the priority queues
struct PriorityQueue queues[QUEUE_NUMBER];

// Declare and initialize mutex and condition variable for high priority requests
pthread_mutex_t high_priority_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t high_priority_cond = PTHREAD_COND_INITIALIZER;
uint8_t high_priority_request = 0;

/*********************************** Main function ************************************/
int main(char argc, char *argv[]){

    // Initialize the priority queues
    for(int i = 0; i < QUEUE_NUMBER; i++){
        sem_init(&queues[i].sem, 0, 1);
        initPriorityQueue(&queues[i], i);
    }
    
    // Four threads to handle low priority requests
    for(int i = 0; i < THREAD_POOL_SIZE/2; i++){
        pthread_create(&thread_pool[i], NULL, &low_priority_handler, &queues[0]);
    }

    // Four threads to handle medium priority requests
    for(int i = THREAD_POOL_SIZE/2; i < THREAD_POOL_SIZE; i++){
        pthread_create(&thread_pool[i], NULL, &high_priority_handler, &queues[1]);
    }

    // Thread to read requests from the client
    pthread_t thread_reader_id;
    int portno = atoi(argv[1]);
    pthread_create(&thread_reader_id, NULL, &thread_reader, &portno);

    for(int i = 0; i < THREAD_POOL_SIZE; i++){
        pthread_join(thread_pool[i], NULL);
    }

    pthread_join(thread_reader_id, NULL);

    return 0;
}

/*
    Function: initPriorityQueue
    Description: Initialize an empty priority queue
    Input: 
        pointer to a priority queue (struct PriorityQueue *queue)
        number to identify the priority queue (int i)
    Output: 
        void
*/
void initPriorityQueue(struct PriorityQueue *queue, int i){
    queue->size = 0;
    queue->number = i;
}

/*
    Function: thread_reader
    Description: Thread to read requests from the client
    Input: 
        port number (void *arg)
    Output: 
        void
*/
void* thread_reader(void *arg){
    int serverSocket, clientSocket, portno;
    int bytesRead, n;
    unsigned char whole_message[PACKET_REQUEST_SIZE];

    struct sockaddr_in serverAdrr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1){
        perror("Error creating socket");
        exit(1);
    }

    bzero((char *)&serverAdrr, sizeof(serverAdrr));
    portno = *((int *)arg);

    serverAdrr.sin_family = AF_INET;
    serverAdrr.sin_addr.s_addr = INADDR_ANY;
    serverAdrr.sin_port = htons(portno);

    if(bind(serverSocket, (struct sockaddr *)&serverAdrr, sizeof(serverAdrr)) == -1){
        perror("Error binding socket");
        exit(1);
    }

    listen(serverSocket, 500);
    printf("Server is listening on port %d\n", portno);

    while(1){
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if(clientSocket == -1){
            perror("Error accepting connection");
            exit(1);
        }

        struct parsedMessage message = read_msg(clientSocket);

        if(message.priority <= THRESHOLD){
            enqueue(&queues[0], message);
        }else{
            enqueue(&queues[1], message);
            if(pthread_mutex_lock(&high_priority_mutex) != 0) {
            // Handle error here, e.g. by continuing to the next iteration
                perror("Error locking mutex");
                continue;
            }
            high_priority_request = 1;
            pthread_cond_signal(&high_priority_cond); // Signal the condition variable
            pthread_mutex_unlock(&high_priority_mutex); // Unlock mutex
        }
    }
}

/*
    Function: isEmpty
    Description: Check if the priority queue is empty
    Input: 
        pointer to a priority queue (struct PriorityQueue *queue)
    Output: 
        1 if the queue is empty
        0 if the queue is not empty
*/
int isEmpty(struct PriorityQueue *queue){
    return (queue->size == 0);
}

/*
    Function: swap
    Description: Swap two elements in the priority queue
    Input: 
        pointer to a parsed message (struct parsedMessage *a)
        pointer to a parsed message (struct parsedMessage *b)
    Output: 
        void
*/
void swap(struct parsedMessage *a, struct parsedMessage *b){
    struct parsedMessage temp = *a;
    *a = *b;
    *b = temp;
}

/*
    Function: enqueue
    Description: Enqueue an element into the priority queue
    Input: 
        pointer to a priority queue (struct PriorityQueue *queue)
        parsed message (struct parsedMessage request)
    Output: 
        void
*/
void enqueue(struct PriorityQueue *queue, struct parsedMessage request){
    
    // Taking sempahore
    if(sem_wait(&queue->sem) != 0){
        perror("Error waiting for semaphore");
        exit(1);
    }

    if(queue->size < MAX_QUEUE_SIZE){ // If the queue is not full
        // Add the element to the end of the queue
        queue->requests[queue->size] = request;
        int currentIndex = queue->size;

        // Bubble up to maintain the priority order (smallest priority at the top)
        while (currentIndex > 0 && queue->requests[currentIndex].priority > queue->requests[(currentIndex - 1) / 2].priority) {
            swap(&queue->requests[currentIndex], &queue->requests[(currentIndex - 1) / 2]);
            currentIndex = (currentIndex - 1) / 2;
        }
        queue->size++;
    } else{
        printf("Priority queue is full.\n");
        return;
    }

    // Releasing semaphore
    if(sem_post(&queue->sem) != 0){
        perror("Error posting semaphore");
        exit(1);
    }
}

struct parsedMessage dequeue(struct PriorityQueue *queue){
    // Taking semaphore
    if(sem_wait(&queue->sem) != 0){
        perror("Error waiting for semaphore");
        exit(1);
    }

    // Element to be dequeued
    struct parsedMessage maxElement;

    if(!isEmpty(queue)){ // If the queue is not empty
        maxElement = queue->requests[0];
        queue->size--;
        queue->requests[0] = queue->requests[queue->size];

        // Heapify to maintain the priority order (highgest priority at the top)
        int currentIndex = 0;
        while (1) {
            int leftChildIndex = 2 * currentIndex + 1;
            int rightChildIndex = 2 * currentIndex + 2;
            int largestIndex = currentIndex;

            if (leftChildIndex < queue->size && queue->requests[leftChildIndex].priority > queue->requests[largestIndex].priority) {
                largestIndex = leftChildIndex;
            }

            if (rightChildIndex < queue->size && queue->requests[rightChildIndex].priority > queue->requests[largestIndex].priority) {
                largestIndex = rightChildIndex;
            }

            if (largestIndex == currentIndex) {
                break;
            }

            swap(&queue->requests[currentIndex], &queue->requests[largestIndex]);
            currentIndex = largestIndex;
        }    
    } else{
        // If the queue is empty, return an empty parsed message
        memset(&maxElement, 0, sizeof(struct parsedMessage));
    }

    // Posting semaphore
    if(sem_post(&queue->sem) != 0){
        perror("Error waiting for semaphore");
        exit(1);
    }
    return maxElement;
}

/*
    Function: read_msg
    Description: Read the message from the client
    Input: 
        socket (int sock)
    Output: 
        parsed message (struct parsedMessage)
*/
struct parsedMessage read_msg(int sock) {
    int n, bytesRead;
    unsigned char whole_message[MESSAGE_SIZE];
    bzero(whole_message, MESSAGE_SIZE);

    bytesRead = read(sock, whole_message, sizeof(whole_message));

    if(bytesRead != MESSAGE_SIZE){
        perror("Error reading message");
        exit(1);
    }    

    struct parsedMessage parsed_message = parsing_msg(whole_message, sock);
    return parsed_message;
}

/*
    Function: parsing_msg
    Description: Parse the message from the client
    Input: 
        message to be parsed (unsigned char *message)
        socket (int sock)
    Output: 
        parsed message (struct parsedMessage)
*/
struct parsedMessage parsing_msg(unsigned char *message, int sock){
    struct parsedMessage parsed_message;
    bzero(&parsed_message, sizeof(parsed_message));

    memcpy(parsed_message.hash, message, SHA256_DIGEST_LENGTH);
    memcpy(&parsed_message.start, message + SHA256_DIGEST_LENGTH, sizeof(uint64_t));
    memcpy(&parsed_message.end, message + SHA256_DIGEST_LENGTH + sizeof(uint64_t), sizeof(uint64_t));
    memcpy(&parsed_message.priority, message + SHA256_DIGEST_LENGTH + sizeof(uint64_t) + sizeof(uint64_t), sizeof(uint8_t));
    memcpy(&parsed_message.sock, &sock, sizeof(int));

    parsed_message.start = be64toh(parsed_message.start);
    parsed_message.end = be64toh(parsed_message.end);

    return parsed_message;
}

/*
    Function: reverse_hash
    Description: Reverse hash by brute force
    Input: 
        start number for the loop (uint64_t start)
        end number for the loop (uint64_t end)
        hash message to be processed (unsigned char *hash_message)
    Output: 
        reversed hash (uint64_t)
*/
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

/*
    Function: write_msg
    Description: Write back the reversed hash to the client
    Input: 
        parsed message (struct parsedMessage parsed_message)
    Output: 
        void
*/
void write_msg(struct parsedMessage parsed_message){
    if(parsed_message.start != NULL){
        uint64_t reversed_hash;
        reversed_hash = reverse_hash(parsed_message.start, parsed_message.end, parsed_message.hash);
        uint64_t reversed_hash_nbo = htobe64(reversed_hash);
        int n;
        n = write(parsed_message.sock, &reversed_hash_nbo, sizeof(reversed_hash_nbo));
        if(n == -1){
            perror("Error writing to socket");
            exit(1);
        }
        close(parsed_message.sock);
    }
}

/*
    Function: low_priority_handler
    Description: Handler for low priority requests from the client
    Input: 
        pointer to a priority queue (void *queuePtr)
    Output: 
        void
*/
void* low_priority_handler(void *queuePtr){
    struct PriorityQueue *queue = (struct PriorityQueue *)queuePtr;
    while(1){        
        struct parsedMessage parsed_message = dequeue(queue);
        write_msg(parsed_message);    
    }
    pthread_exit(NULL);
}

/*
    Function: high_priority_handler
    Description: Handler for high priority requests from the client
    Input: 
        pointer to a priority queue (void *queuePtr)
    Output: 
        void
*/
void* high_priority_handler(void *queuePtr){
    struct PriorityQueue *queue = (struct PriorityQueue *)queuePtr;
    while(1){
        if(pthread_mutex_lock(&high_priority_mutex) != 0) {
            perror("Error locking mutex");
            exit(1);
        }
        while(!high_priority_request){ // If there is no high priority request
            pthread_cond_wait(&high_priority_cond, &high_priority_mutex); // Wait in the condition variable
        }
        high_priority_request = 0;
        pthread_mutex_unlock(&high_priority_mutex);
        struct parsedMessage parsed_message = dequeue(queue);
        write_msg(parsed_message);
    }   
    pthread_exit(NULL);
}