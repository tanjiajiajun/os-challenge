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

#define MESSAGE_SIZE 49
#define MAX_QUEUE_SIZE 100
#define THREAD_POOL_SIZE 8
#define QUEUE_NUMBER 2
// ********************************* Global variables ********************************
// Define the thread pool
pthread_t thread_pool[THREAD_POOL_SIZE];

// ********************************* Structures **************************************
// Structure to store request's fields from the client
struct parsedMessage{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint64_t start;
    uint64_t end;
    uint8_t priority;
    int sock;
};

// Define the priority queue
struct PriorityQueue {
    struct parsedMessage requests[MAX_QUEUE_SIZE];
    int size;
    sem_t sem;
    int number;
};

struct PriorityQueue queues[QUEUE_NUMBER];

// *************************** Functions for priority queue *************************** 
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

// *************************** Functions to handle requests **************************
// Read the message from the client
struct parsedMessage read_msg(int sock);

// Parse the message from the client
struct parsedMessage parsing_msg(unsigned char *message, int sock);

// Reverse hash by brute force
uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message);

// Write back the reversed hash to the client
void write_msg(struct parsedMessage parsed_message);

// ********************** Function to for threads ***********************
// Handle multiple requests from the client
void* low_priority_handler(void *queuePtr);

void* thread_reader(void *arg);

int main(char argc, char *argv[]){
    for(int i = 0; i < QUEUE_NUMBER; i++){
        sem_init(&queues[i].sem, 0, 1);
        initPriorityQueue(&queues[i], i);
    }

    for(int i = 0; i < 3; i++){
        pthread_create(&thread_pool[i], NULL, &low_priority_handler, &queues[0]);
        // printf("Thread %d created assign to queue %d\n", i, i);
    }

    for(int i = 3; i < 5; i++){
        pthread_create(&thread_pool[i], NULL, &low_priority_handler, &queues[1]);
        // printf("Thread %d created assign to queue %d\n", i, i-4);
    }

    for(int i = 5; i < 7; i++){
        pthread_create(&thread_pool[i], NULL, &low_priority_handler, &queues[2]);
        // printf("Thread %d created assign to queue %d\n", i, i-8);
    }

    pthread_create(&thread_pool[7], NULL, &low_priority_handler, &queues[3]);

    pthread_t thread_reader_id;
    int portno = atoi(argv[1]);
    pthread_create(&thread_reader_id, NULL, &thread_reader, &portno);

    for(int i = 0; i < THREAD_POOL_SIZE; i++){
        pthread_join(thread_pool[i], NULL);
    }

    pthread_join(thread_reader_id, NULL);

    return 0;
}

void initPriorityQueue(struct PriorityQueue *queue, int i){
    queue->size = 0;
    queue->number = i;
}

void* thread_reader(void *arg){
    // printf("Thread reader created\n");
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
    // portno = atoi(portno);

    serverAdrr.sin_family = AF_INET;
    serverAdrr.sin_addr.s_addr = INADDR_ANY;
    serverAdrr.sin_port = htons(portno);

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
        // unpacking the message and enqueueing to pq
        struct parsedMessage message = read_msg(clientSocket);

        if(message.priority <= 4){
            enqueue(&queues[0], message);
        }else if(message.priority <= 8){
            enqueue(&queues[1], message);
        }else if(message.priority <= 12){
            enqueue(&queues[2], message);
        }else if(message.priority <= 16){
            enqueue(&queues[3], message);
        }
    }
}

int isEmpty(struct PriorityQueue *queue){
    return (queue->size == 0);
}

void swap(struct parsedMessage *a, struct parsedMessage *b){
    struct parsedMessage temp = *a;
    *a = *b;
    *b = temp;
}

void enqueue(struct PriorityQueue *queue, struct parsedMessage request){
    
    // Taking sempahore
    // sem_wait(&queue->sem);
    int ret = sem_wait(&queue->sem);
    if(ret != 0){
        perror("Error waiting for semaphore");
    }

    if(queue->size < MAX_QUEUE_SIZE){ // If the queue is not full
        // printf("Enqueueing in %d queue\n", queue->number);

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

    // sem_post(&queue->sem);
    ret = sem_post(&queue->sem);
    if(ret != 0){
        perror("Error posting semaphore");
    }
    // printf("Enqueue semaphore posted\n");
}

struct parsedMessage dequeue(struct PriorityQueue *queue){
    // Taking semaphore
    // sem_wait(&queue->sem);
    int ret = sem_wait(&queue->sem);
    if(ret != 0){
        perror("Error waiting for semaphore");
    }

    struct parsedMessage maxElement;

    if(!isEmpty(queue)){ // If the queue is not empty
        // printf("Dequeuing in %d queueu\n", queue->number);

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
        // printf("Priority queue is empty.\n");
        memset(&maxElement, 0, sizeof(struct parsedMessage));
    }

    // Posting semaphore
    // sem_post(&queue->sem);
    ret = sem_post(&queue->sem);
    if(ret != 0){
        perror("Error waiting for semaphore");
        exit(1);
    }
    // printf("Dequeue semaphore posted\n");
    return maxElement;
}

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

void* low_priority_handler(void *queuePtr){
    struct PriorityQueue *queue = (struct PriorityQueue *)queuePtr;
    while(true){        
        struct parsedMessage parsed_message = dequeue(queue);
        write_msg(parsed_message);    
    }
    pthread_exit(NULL);
}
