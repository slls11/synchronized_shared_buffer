#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

#define ARRAY_SIZE 8
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_NAME_LENGTH 255 // this includes '\0'
#define MAX_HOSTS_FILE 50
//#define MAX_IP_LENGTH

/* alot of these are unused...*/
typedef struct{
    FILE* stack[MAX_INPUT_FILES];
    FILE* head;
    int top;
    int host_count;
    int host_max;
    int host_ip_count;
    int something_to_read;
    int write_thread_count;
    int read_thread_count;

    pthread_mutex_t mutex_file_stack_top;
    pthread_mutex_t mutex_check_top;
    pthread_mutex_t mutex_write_serviced;
    pthread_mutex_t mutex_write_results;

    pthread_mutex_t mutex_read_serviced;
    pthread_mutex_t mutex_read_results;


    sem_t avaliable_files_left;

    FILE* fptr_serviced; 
    FILE* fptr_results;

    char serviced_stack[MAX_INPUT_FILES * MAX_HOSTS_FILE * MAX_NAME_LENGTH];
    char* ptr_serviced_stack;
    int index_serviced_stack;

    char results_stack[MAX_INPUT_FILES * MAX_HOSTS_FILE * MAX_NAME_LENGTH];
    char* ptr_results_stack;
    //int index_serviced_stack;


    ////////////////////
    char buffer[ARRAY_SIZE * MAX_NAME_LENGTH]; char* buffer_ptr;
    int array_write_stack[ARRAY_SIZE]; int write_stack_top;
    int array_read_stack[ARRAY_SIZE]; int read_stack_top;

    sem_t sem_write_space;
    sem_t sem_read_space;
    pthread_mutex_t mutex_producer_update;
    pthread_mutex_t mutex_consumer_update;

    int write_threads_done;

} File_Stack;
