#include "multi-lookup.h"
#include "util.h"

/* this is a really dumb and messy way to do this, new solution is much better... */

void free_data(File_Stack* fs_ptr){
    pthread_mutex_destroy(&(fs_ptr->mutex_file_stack_top));    
    pthread_mutex_destroy(&(fs_ptr->mutex_check_top));
    pthread_mutex_destroy(&(fs_ptr->mutex_write_serviced));
    pthread_mutex_destroy(&(fs_ptr->mutex_read_serviced));
    pthread_mutex_destroy(&(fs_ptr->mutex_producer_update));
    pthread_mutex_destroy(&(fs_ptr->mutex_consumer_update));
    pthread_mutex_destroy(&(fs_ptr->mutex_write_serviced));
    pthread_mutex_destroy(&(fs_ptr->mutex_write_results));

    sem_destroy(&(fs_ptr->sem_read_space));  
    sem_destroy(&(fs_ptr->sem_write_space));

    fclose(fs_ptr->fptr_serviced);
    fclose(fs_ptr->fptr_results);

    free(fs_ptr);
}

int length_str(const char* arr_start){
    int len = 0;
    while (*arr_start != '\0'){
        arr_start++; len++;
    }
return len;
}

int convert_to_int(const char* arr_start){
    int len = length_str(arr_start);   
    char str[len]; strncpy(str, arr_start, len);
return atoi(str);
}

int check_thread_input(const int argc, const char* argv[], File_Stack* fs_ptr){
    if (argc < 6){
        printf("Incorrect number of arguments! Program needs at least 6 arguments!\n");
        return -1;
    }
    int requester_arg = convert_to_int(argv[1]); 
    int resolver_arg = convert_to_int(argv[2]); 

    if (requester_arg > MAX_REQUESTER_THREADS || requester_arg < 1){
        fprintf(stderr, "Input not in range for requester thread count!\n");
        return -1;
    }
    else if (resolver_arg > MAX_RESOLVER_THREADS || resolver_arg < 1){
        fprintf(stderr, "Input not in range for resolver thread count!\n");
        return -1;
    }
    if (requester_arg == 1)requester_arg++;
    if (resolver_arg == 1)resolver_arg++;


    fs_ptr->write_thread_count = requester_arg;
    fs_ptr->read_thread_count = resolver_arg;

return 0;
}

int check_file_input(const char* argv[]){
    if (!!(strcmp(argv[3], "serviced.txt"))){
        fprintf(stderr, "Input for requester write file invalid! Must be 'serviced.txt'.\n");
        return -1;
    }else if (!!(strcmp(argv[4], "results.txt"))){
        fprintf(stderr, "Input for resolver write file invalid! Must be 'results.txt'\n");
        return -1;
    }
return 0; 
}

void insert_files_stack(const int argc, const char* file_names[], File_Stack* fs_ptr){
    int count = 5; fs_ptr->host_count = 0; fs_ptr->host_ip_count = 0;
    for (int i = 5; i < argc; i++){
        FILE* file;
        if ((file = fopen(file_names[count], "r"))){
            fs_ptr->stack[i-5] = fopen(file_names[count], "r");
            fs_ptr->top++;
        }
        else{
            fprintf(stderr, "invalid file %s\n", file_names[count]);
            //i--;
        }
        count++;
    }
    //printf("********number of hostname total: %d *****\n", fs_ptr->host_count);
}

File_Stack* init_files_stack(const int argc, const char* file_names[]){ 
    File_Stack* fs_ptr = malloc(sizeof(File_Stack));
    fs_ptr->top = -1;
    fs_ptr->fptr_serviced = fopen("serviced.txt", "w+");
    fs_ptr->fptr_results = fopen("results.txt", "w+");
    fs_ptr->ptr_serviced_stack = fs_ptr->serviced_stack;
    fs_ptr->ptr_results_stack = fs_ptr->results_stack;

    insert_files_stack(argc, file_names, fs_ptr);

    pthread_mutex_init(&(fs_ptr->mutex_file_stack_top), NULL);
    pthread_mutex_init(&(fs_ptr->mutex_check_top), NULL);
    pthread_mutex_init(&(fs_ptr->mutex_write_serviced), NULL);
    pthread_mutex_init(&(fs_ptr->mutex_read_serviced), NULL);

    sem_init(&(fs_ptr->sem_write_space), 0, ARRAY_SIZE);
    sem_init(&(fs_ptr->sem_read_space), 0, 0);
    pthread_mutex_init(&(fs_ptr->mutex_producer_update), NULL);
    pthread_mutex_init(&(fs_ptr->mutex_consumer_update), NULL);
    pthread_mutex_init(&(fs_ptr->mutex_write_serviced), NULL);
    pthread_mutex_init(&(fs_ptr->mutex_write_results), NULL);
    fs_ptr->write_stack_top = ARRAY_SIZE - 1; // entire buffer is empty
    fs_ptr->read_stack_top = -1; // nothing to read
    for (int i = 0; i < ARRAY_SIZE; i++) fs_ptr->array_write_stack[i] = i; // fill write stack   
    
return fs_ptr;
}

void array_put(const char* hostname, File_Stack* fs_ptr){    
    sem_wait(&(fs_ptr->sem_write_space));
        pthread_mutex_lock(&(fs_ptr->mutex_producer_update));
            int array_pos = fs_ptr->array_write_stack[fs_ptr->write_stack_top]; // write point in buffer
            fs_ptr->write_stack_top--;
            fs_ptr->read_stack_top++;
            fs_ptr->array_read_stack[fs_ptr->read_stack_top] = array_pos;        // place index to read in read stack;
            char* write_pos = fs_ptr->buffer+(array_pos*MAX_NAME_LENGTH); 
            strcpy(write_pos, hostname);
        pthread_mutex_unlock(&(fs_ptr->mutex_producer_update));
    sem_post(&(fs_ptr->sem_read_space));
}

void write_serviced(File_Stack* fs_ptr){
    for (int i = 0; i < MAX_INPUT_FILES * MAX_HOSTS_FILE * MAX_NAME_LENGTH ; i++){
        if (fs_ptr->serviced_stack[i] == '\0') {
            fputc('\n', fs_ptr->fptr_serviced);
            if (fs_ptr->serviced_stack+i == fs_ptr->ptr_serviced_stack) break; 
        }
        else fputc(fs_ptr->serviced_stack[i], fs_ptr->fptr_serviced);
    }
}

void write_results(File_Stack* fs_ptr){
    for (int i = 0; i < MAX_INPUT_FILES * MAX_HOSTS_FILE * MAX_NAME_LENGTH; i++){
    if (fs_ptr->results_stack[i] != '\0'){
        char host[MAX_NAME_LENGTH];
        int j = 0;
        while (fs_ptr->results_stack[i] != '\0'){
            host[j] = fs_ptr->results_stack[i];
            j++; i++;
        }
        char ip_address[MAX_NAME_LENGTH];
        if (!dnslookup(host, ip_address, MAX_NAME_LENGTH)){
            fputs(host, fs_ptr->fptr_results);
            fputs(", ", fs_ptr->fptr_results);
            fputs(ip_address, fs_ptr->fptr_results);
            fputc('\n', fs_ptr->fptr_results);
            printf("%s, %s\n",host, ip_address);

        }else{
            fputs(host, fs_ptr->fptr_results);
            fputs(", ", fs_ptr->fptr_results);
            fputs(" NOT_RESOLVED", fs_ptr->fptr_results);
            fputc('\n', fs_ptr->fptr_results);
            printf("%s, NOT_RESOLVED\n",host);

        }
        memset(host,0,sizeof(host));
    }
}  
}

int send_to_array_put(File_Stack* fs_ptr){
    pthread_mutex_lock(&(fs_ptr->mutex_check_top));
        int files_left;
        if (fs_ptr->top < 0) {
            pthread_mutex_unlock(&(fs_ptr->mutex_check_top));
            return 0;
        }
        files_left = fs_ptr->top;
        fs_ptr->top--;
    pthread_mutex_unlock(&(fs_ptr->mutex_check_top));

    if (files_left < 0) {
        printf("files_left ERROR\n");
        return 0;
    }

    char hostname[MAX_NAME_LENGTH];
    while (fscanf(fs_ptr->stack[files_left], "%s", hostname) == 1){
        char chr = '\0'; strncat(hostname, &chr, 1);
        array_put(hostname, fs_ptr);

        pthread_mutex_lock(&(fs_ptr->mutex_write_serviced));
            const char c = '\0';
            strcpy(fs_ptr->ptr_serviced_stack, hostname);
            fs_ptr->ptr_serviced_stack = strchr(fs_ptr->ptr_serviced_stack, c);
            fs_ptr->ptr_serviced_stack++;
        pthread_mutex_unlock(&(fs_ptr->mutex_write_serviced)); 
    }
return 1;
}

void requester_loop(void* args){
    File_Stack* fs_ptr = (File_Stack*)args;
    while (send_to_array_put(fs_ptr));
}

char* array_get(File_Stack* fs_ptr){    
    sem_wait(&(fs_ptr->sem_read_space));
        char str[5] = "*EXIT";
        char* ptr = str;
        if (fs_ptr->read_stack_top < 0) return ptr;
        if (fs_ptr->host_ip_count > fs_ptr->host_count && fs_ptr->something_to_read == 0) return ptr;

        pthread_mutex_lock(&(fs_ptr->mutex_producer_update));
        
            int array_pos = fs_ptr->array_read_stack[fs_ptr->read_stack_top];
            fs_ptr->read_stack_top--;
            fs_ptr->write_stack_top++;
            fs_ptr->array_write_stack[fs_ptr->write_stack_top] = array_pos;        // place index to read in read stack;        
            char* read_pos = fs_ptr->buffer+(array_pos*MAX_NAME_LENGTH);
            char* hostname = malloc(sizeof(char) * MAX_NAME_LENGTH);
            strcpy(hostname, read_pos); // hostname terminated by "\0"
            
            while(*read_pos != '\0'){
                *read_pos = '\0';
                read_pos++;
            }
        pthread_mutex_unlock(&(fs_ptr->mutex_producer_update));

    sem_post(&(fs_ptr->sem_write_space));

return hostname;
} 

int send_to_array_get(File_Stack* fs_ptr){
    if (fs_ptr->something_to_read || fs_ptr->read_stack_top > -1){
        if (fs_ptr->host_ip_count == fs_ptr->host_count && fs_ptr->something_to_read && fs_ptr->read_stack_top < 0) 
            return 0;

        char* hostname = array_get(fs_ptr);
        if (!strcmp(hostname, "*EXIT")) return 0;
        pthread_mutex_lock(&(fs_ptr->mutex_write_results));
            fs_ptr->host_ip_count++;
            //printf("ip_count:%d - %s\n", fs_ptr->host_ip_count, hostname);
            const char c = '\0';
            strcpy(fs_ptr->ptr_results_stack, hostname);
            fs_ptr->ptr_results_stack = strchr(fs_ptr->ptr_results_stack, c);
            fs_ptr->ptr_results_stack++;
        pthread_mutex_unlock(&(fs_ptr->mutex_write_results));
    return 1;
    }
return 0;
}

void resolver_loop(void* args){
    File_Stack* fs_ptr = (File_Stack*)args;
    while (send_to_array_get(fs_ptr));
}

void buffer_flush(File_Stack* fs_ptr){
    for (int i = 0; i < ARRAY_SIZE * MAX_NAME_LENGTH; i++){
        if (fs_ptr->buffer[i] != '\0'){
            char* hostname = malloc(sizeof(char) * MAX_NAME_LENGTH);
            int j = 0;
            while (fs_ptr->buffer[i] != '\0'){
                hostname[j] = fs_ptr->buffer[i];
                j++; i++;
            }
            hostname[j] = fs_ptr->buffer[i];
            //printf("Flushing: %s\n", hostname);
            pthread_mutex_lock(&(fs_ptr->mutex_write_results));
                fs_ptr->host_ip_count++;
                //printf("ip_count:%d |%s|\n", fs_ptr->host_ip_count, hostname);
                const char c = '\0';
                strcpy(fs_ptr->ptr_results_stack, hostname);
                fs_ptr->ptr_results_stack = strchr(fs_ptr->ptr_results_stack, c);
                fs_ptr->ptr_results_stack++;
                free(hostname);
            pthread_mutex_unlock(&(fs_ptr->mutex_write_results));
        }
    }
}

void init_threads(File_Stack* fs_ptr){
    pthread_t p_tid[fs_ptr->write_thread_count]; 
    pthread_t c_tid[fs_ptr->read_thread_count];

    for (int i = 0; i < (fs_ptr->write_thread_count); i++){
        if (pthread_create(&p_tid[i], NULL, (void*)&requester_loop, fs_ptr) != 0)
            fprintf(stderr, "Failed to create requester thread\n");   
    }
    fs_ptr->something_to_read = 1;

    for (int i = 0; i < (fs_ptr->read_thread_count); i++){
        if (pthread_create(&c_tid[i], NULL, (void*)&resolver_loop, fs_ptr) != 0)
            fprintf(stderr, "Failed to create resolver thread\n");
    }

    for (int i = 0; i < (fs_ptr->write_thread_count); i++){
        if (pthread_join(p_tid[i], NULL) != 0)
            fprintf(stderr, "Failed to join thread\n");
    }
    fs_ptr->something_to_read = 0;

    for (int i = 0; i < MAX_RESOLVER_THREADS; i++)
        sem_post(&(fs_ptr->sem_read_space));
    
    for (int i = 0; i < (fs_ptr->read_thread_count); i++){
        if (pthread_join(c_tid[i], NULL) != 0)
            fprintf(stderr, "Failed to join thread\n");
    }
}

int main(const int argc, const char* argv[]){
    if (check_file_input(argv) == -1) return -1;
    File_Stack* fs_ptr = init_files_stack(argc, argv);
    if (check_thread_input(argc, argv, fs_ptr) == -1) return -1;

    init_threads(fs_ptr);

    write_serviced(fs_ptr); 
    buffer_flush(fs_ptr); 
    write_results(fs_ptr);

    free(fs_ptr);
return 0;
}
