// This experiment test the use threads

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


#include <unistd.h> 
#include <pthread.h> 

#include <time.h>

//#include messages.h

#define MAX_P_ARRAY_SIZE 500 //increase later

struct parsedMessage{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint64_t start;
    uint64_t end;
    uint8_t priority;
};

int g = 0; 
int p_queue[MAX_P_ARRAY_SIZE];
int queue_start = 0;

int start_ind = 0;

pthread_mutex_t lock; 
pthread_mutex_t lock2; //this is lock for running_threads

int running_threads = 0; //this is the counter that keeps track of number of running threads


/*
peusdo code for later:
lock
- get value from start_ind
- start_ind++
unlock

do reverse hashing

*/

// Function to reverse the hash by brute force
uint64_t reverse_hash(uint64_t start, uint64_t end, unsigned char *hash_message);

// The function to be executed by all threads 
void *myThreadFun(void *vargp) 
{
    // Store the value argument passed to this thread 
    int *myid = (int *)vargp; 
  
    // Let us create a static variable to observe its changes 
    static int s = 0; 
  
    // Change static and global variables 
    ++s; ++g; 

    // pthread_mutex_lock(&lock); 
    // int val = p_queue[start_ind];
    // ++start_ind;
    // pthread_mutex_unlock(&lock); 

    // printf("value is %d \n", val);
    // //do the reverse hashing here and then send response

    //printf("running threads: %d \n", num_threads);
    // Print the argument, static and global variables 
    //printf("Thread ID: %d, Static: %d, Global: %d\n", *myid, ++s, ++g); 
    //need to lock this

    // test conditions
    uint64_t start = 0;
    uint64_t end = 300000;  // final diffuculty conditions 30000000

    unsigned char temp_hash[SHA256_DIGEST_LENGTH];
    uint64_t test_num = end-1; //this is to test worst case scenario run time

    // store temporary hash 
    bzero(temp_hash, sizeof(temp_hash));
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, &test_num, sizeof(test_num));
    SHA256_Final(temp_hash, &sha256);

    // solve reverse hash the brute force method
    reverse_hash(start, end, temp_hash);
    
    //thread has finished, decrement counter
    pthread_mutex_lock(&lock2);
    int num_threads = running_threads--;
    pthread_mutex_unlock(&lock2);
} 

//
void *priorityThread(void *vargp) 
{
    for(int i=0; i<MAX_P_ARRAY_SIZE; i++){
        //todo: organize by priority here
        p_queue[i]=i;
        printf("Adding to queue position %d \n", i);

        //Note: would need to set a limit to size and wrap around
    }
}

int main(char argc, char *argv[]) 
{ 
    struct parsedMessage record[2];

    int i; 
    pthread_t tid; 

    //idea: experiment with a limit on # of threads 

    //pthread_create(&tid, NULL, priorityThread, (void *)&tid); 
    
    // Create threads
    int counter = 0; 
    bool make_new_thread = true;

    // experiment parameters: goal is to see how limiting the number of threads affects 
    // run time
    // modify these values to test different conditions
    int thread_limit =  2;
    int num_requests = 1000;

    clock_t start, stop;
    start = clock(); //use this to time

    while(counter<num_requests){
        //printf("counter: %d\n",counter);
        pthread_mutex_lock(&lock2);
        if(running_threads<thread_limit){
            make_new_thread = true;
        }else{
            make_new_thread = false;
        }
        pthread_mutex_unlock(&lock2);
        //make_new_thread = true;

        if(make_new_thread){
            pthread_mutex_lock(&lock2);
            running_threads++;
            pthread_mutex_unlock(&lock2);
            pthread_create(&tid, NULL, myThreadFun, (void *)&tid); 
            counter++;
        }
    }
    stop = clock();
 
    printf("Approx seconds, tenths, hundredths and milliseconds: %.3f\n", 
        ((double)(stop - start) / CLOCKS_PER_SEC));
  
    pthread_exit(NULL); 
    return 0; 
} 


// function to reverse the hash
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


//misc notes:
//experiment determined that limiting number of threads slowed down runtime 50 vs 500. 129 second vs 73 seconds
// Parameters used: difficulty: 300000, request: 1000

// void doprocessing(int sock){
//     int n, bytesRead;
//     unsigned char whole_message[MESSAGE_SIZE];
//     bzero(whole_message, MESSAGE_SIZE);

//     bytesRead = read(sock, whole_message, sizeof(whole_message));

//     if(bytesRead != MESSAGE_SIZE){
//         perror("Error reading message");
//         exit(1);
//     }    

//     struct parsedMessage parsed_message = parcing_msg(whole_message);
//     prio_score = parsed_message.priority;
//     uint64_t reversed_hash;
//     reversed_hash = reverse_hash(parsed_message.start, parsed_message.end, parsed_message.hash);

//     uint64_t reversed_hash_nbo = htobe64(reversed_hash);
//     n = write(sock, &reversed_hash_nbo, sizeof(reversed_hash_nbo));
//     if(n == -1){
//         perror("Error writing to socket");
//         exit(1);
//     }
    
//     close(sock);
// }