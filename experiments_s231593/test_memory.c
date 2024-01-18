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
#include <pthread.h>

#include "messages.h"

#define MESSAGE_SIZE 49
#define MAX_QUEUE_SIZE 1000
#define THREAD_POOL_SIZE 8
#define SECOND_CHANCE_PAGE 50

// Structure to store the message's fields from the client
struct parsedMessage{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint64_t start;
    uint64_t end;
    uint8_t priority;
    int sock;
};

// Define the priority queue
struct PriorityQueue {
    struct parsedMessage elements[MAX_QUEUE_SIZE];
    int size;
};

// Define Struct for second chance array
struct SecondChance {
    unsigned char *hash;
    uint64_t answer;
    bool second_chance;
};

// Functions for priority queue
void initPriorityQueue(struct PriorityQueue *queue);
int isEmpty(struct PriorityQueue *queue);
void swap(struct parsedMessage *a, struct parsedMessage *b);
void enqueue(struct PriorityQueue *queue, struct parsedMessage element);
struct parsedMessage dequeue(struct PriorityQueue *queue);



// Function for each thread to execute
void* thread_function(void *queuePtr);

// Function to parse the message from the client
struct parsedMessage parcing_msg(unsigned char *message, int sock);
struct parsedMessage unpack_msg(int sock);

// Function to reverse the hash by brute force
uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message);

// Creating thread pool and
pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Init second chance arrays
struct SecondChance* frames[SECOND_CHANCE_PAGE];
int counter = 0;

int main(char argc, char *argv[]){
    int serverSocket, clientSocket, portno;
    int bytesRead; //n
    unsigned char whole_message[PACKET_REQUEST_SIZE];

    // Init priority queue to store requests.
    struct PriorityQueue queue;
    initPriorityQueue(&queue);

    for (int i=0; i<THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, &thread_function, &queue);
    }

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
        // unpacking the message and enqueueing to pq
        struct parsedMessage message = unpack_msg(clientSocket);

        pthread_mutex_lock(&mutex);
        enqueue(&queue, message);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}

struct parsedMessage unpack_msg(int sock) {
    int n, bytesRead;
    unsigned char whole_message[MESSAGE_SIZE];
    bzero(whole_message, MESSAGE_SIZE);

    bytesRead = read(sock, whole_message, sizeof(whole_message));

    if(bytesRead != MESSAGE_SIZE){
        perror("Error reading message");
        exit(1);
    }    

    struct parsedMessage parsed_message = parcing_msg(whole_message, sock);
    return parsed_message;
}

void* thread_function(void *queuePtr){
    struct PriorityQueue *queue = (struct PriorityQueue *)queuePtr;
    while(true) {
        // Include mutex for race condition
        pthread_mutex_lock(&mutex);
        // Dequeue from priority queue
        struct parsedMessage parsed_message = dequeue(queue);
        pthread_mutex_unlock(&mutex);
        if (parsed_message.start != NULL) {
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
    pthread_exit(NULL);
}

struct parsedMessage parcing_msg(unsigned char *message, int sock){
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

    // Check head of queue and match the hash_message.
    // If hash_message does not match, check boolean. If true, change to false and put it to the back of the array
    while (counter > 1) {
        struct SecondChance* head = frames[0];
        bool isEqual = (memcmp(head->hash, hash_message, 32) == 0);
        if (isEqual) {
            return head->answer;
            break;
        } else {
            if (head->second_chance) {
                for (int i=1; i<counter; i++) {
                    frames[i - 1] = frames[i];
                }
                head->second_chance = false;
                frames[9] = head;
            } else {
                //remove and add new block to the end
                for (int i=1; i<counter; i++) {
                    frames[i - 1] = frames[i];
                }
                counter--;
                break;
            }
        }
        break;
    }
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
            // Add the first 50 elements into the SecondChance array
            if (counter < SECOND_CHANCE_PAGE - 1) {

            frames[counter] = (struct SecondChance*)malloc(sizeof(struct SecondChance));
            if (frames[counter] != NULL) {
                frames[counter]->hash = (unsigned char*)malloc(32); // Assuming a 32-byte message
                memcpy(frames[counter]->hash, hash_message, 32);
                frames[counter]->answer = test_num; // Initialize answer
                frames[counter]->second_chance = true;
                }
                counter++;
            }
            return test_num;
        }
    }
    return test_num;
}


/*
The following is code for the PriorityQueue to store requests, ranked on their
priority level. Hopes to move this code to a separate file in the future.
*/

// Function to initialize the priority queue
void initPriorityQueue(struct PriorityQueue *queue) {
    queue->size = 0;
}

// Function to check if the priority queue is empty
int isEmpty(struct PriorityQueue *queue) {
    return (queue->size == 0);
}

// Function to swap two elements
void swap(struct parsedMessage *a, struct parsedMessage *b) {
    struct parsedMessage temp = *a;
    *a = *b;
    *b = temp;
}

// Function to enqueue an element into the priority queue
void enqueue(struct PriorityQueue *queue, struct parsedMessage element) {
    if (queue->size >= MAX_QUEUE_SIZE) {
        printf("Priority queue is full.\n");
        return;
    }

    // Add the element to the end of the queue
    queue->elements[queue->size] = element;
    int currentIndex = queue->size;

    // Bubble up to maintain the priority order (smallest priority at the top)
    while (currentIndex > 0 && queue->elements[currentIndex].priority > queue->elements[(currentIndex - 1) / 2].priority) {
        swap(&queue->elements[currentIndex], &queue->elements[(currentIndex - 1) / 2]);
        currentIndex = (currentIndex - 1) / 2;
    }

    queue->size++;
}

// Function to dequeue an element from the priority queue
struct parsedMessage dequeue(struct PriorityQueue *queue) {
    if (isEmpty(queue)) {
        // printf("Priority queue is empty.\n");
        struct parsedMessage message;
        memset(&message, 0, sizeof(struct parsedMessage));
        return message;
    }

    struct parsedMessage maxElement = queue->elements[0];
    queue->size--;
    queue->elements[0] = queue->elements[queue->size];

    // Heapify to maintain the priority order (smallest priority at the top)
    int currentIndex = 0;
    while (1) {
        int leftChildIndex = 2 * currentIndex + 1;
        int rightChildIndex = 2 * currentIndex + 2;
        int largestIndex = currentIndex;

        if (leftChildIndex < queue->size && queue->elements[leftChildIndex].priority > queue->elements[largestIndex].priority) {
            largestIndex = leftChildIndex;
        }

        if (rightChildIndex < queue->size && queue->elements[rightChildIndex].priority > queue->elements[largestIndex].priority) {
            largestIndex = rightChildIndex;
        }

        if (largestIndex == currentIndex) {
            break;
        }

        swap(&queue->elements[currentIndex], &queue->elements[largestIndex]);
        currentIndex = largestIndex;
    }

    return maxElement;
}
