/**
 * Austin Van Braeckel
 * 2021-02-22
 * CIS*3110 Assignment 2
 * A CPU Scheduler simulation that takes input from a given file,
 * having the following usage: "./simcpu [-d] [-v] [-r quantum] < input_file"
 * - where the d flag is for detailed information
 * - where the v flag is for verbose mode
 * - where the r flag indicates round robin scheduling with the given quantum
 * The input file format is specified in the Assignment 2 Description, and only that format
 * is supported with this program.
 * 
 * SOURCES:
 * - Code for Heap inspired by https://gist.github.com/sudhanshuptl/d86da25da46aa3d060e7be876bbdb343
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SUCCESS 1
#define FAILURE 0
#define MAX_LEN 500

#define STATE_NEW 0
#define STATE_READY 1
#define STATE_RUNNING 2
#define STATE_BLOCKED 3
#define STATE_TERMINATED 4

// turnaround time is time finished executing - time of start //TODO HERE

/* --------------------------------- PROTOTYPES ---------------------------------*/

void get_data();
int set_flags(bool* d, bool *v, bool *r, int argc, char *argv[]);

typedef struct thread_struct {
    int thread_num;
    int arrival_time;
    int *burst_time[2];
} Thread;

typedef struct heap_struct {
    Thread **arr;
    int count;
    int capacity;
} PriorityQueue;

PriorityQueue *CreateHeap(int capacity);
void insert(PriorityQueue *h, Thread *key);
void print(PriorityQueue *h);
void heapify_bottom_top(PriorityQueue *h,int index);
void heapify_top_bottom(PriorityQueue *h, int parent_node);
Thread* PopMin(PriorityQueue *h);

/*-------------------------------------------------------------------------------*/

/* ----------------- MAIN ----------------- */
int main (int argc, char *argv[]) {

    if (argc > 5) { // invalid number of arguments
        fprintf(stderr, "Usage: ./simcpu [-d] [-v] [-r quantum] < input_file\n");
        exit(-1);
    }

    bool *d_flag;
    bool *v_flag;
    bool *r_flag;
    int quantum = -1;
    char line[MAX_LEN];

    PriorityQueue *pq = CreateHeap(10);
    Thread t1;
    t1.thread_num = 1;
    t1.arrival_time = 20;
    insert(pq, &t1);
    t1.thread_num = 2;
    t1.arrival_time = 10;
    insert(pq, &t1);
    t1.thread_num = 3;
    t1.arrival_time = 30;
    insert(pq, &t1);
    t1.thread_num = 4;
    t1.arrival_time = 5;
    insert(pq, &t1);
    print(pq);

    printf("REMOVING\n\n");
    Thread *popped = PopMin(pq);
    printf("POPPED: Num: %d, Arrive: %d\n", popped->thread_num, popped->arrival_time);
    free(popped);
    popped = PopMin(pq);
    printf("POPPED: Num: %d, Arrive: %d\n", popped->thread_num, popped->arrival_time);
    free(popped);
    popped = PopMin(pq);
    printf("POPPED: Num: %d, Arrive: %d\n", popped->thread_num, popped->arrival_time);
    free(popped);
    popped = PopMin(pq);
    printf("POPPED: Num: %d, Arrive: %d\n", popped->thread_num, popped->arrival_time);
    free(popped);

    exit(0);

    set_flags(d_flag, v_flag, r_flag, argc, argv);
    if (*r_flag == true && quantum <= 0) {
        fprintf(stderr, "Usage: ./simcpu [-d] [-v] [-r quantum] < input_file\n");
        exit(-1);
    }

    // read input
    int num_processes; 
    int units_same_switch; // switch to new thread in same process
    int units_diff_switch; // switch to new thread in different process
    fgets(line, MAX_LEN - 1, stdin);
    sscanf(line, "%d %d %d", &num_processes, &units_same_switch, &units_diff_switch); // first line always starts with this
    if (num_processes <= 0) return 0;
    if (units_same_switch < 0 || units_diff_switch < 0) {
        fprintf(stderr, "ERROR: Invalid first line in input\n");
        exit(-1);
    }
    get_data(line);



    return 0;
}

/*--------------------------------- HELPER FUNCTIONS ---------------------------------*/

// Gets the data from the input file (from stdin)
void get_data(char *line) {
    while (line != NULL) {
        int num_threads;
        fgets(line, MAX_LEN - 1, stdin);
        sscanf(line, "%d %d %d");
    }
}

// returns the quantum and sets the flags to true if they exist
int set_flags(bool* d, bool *v, bool *r, int argc, char *argv[]) {
    int i;
    int quantum = 0;
    if (argc < 1 || argc > 5) return -1; // invalid number of arguments
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) *d = true;
        else if (strcmp(argv[i], "-v") == 0) *v = true;
        else if (strcmp(argv[i], "-r") == 0) {
            *r = true;
            if (argc > i + 1) {
                quantum = atoi(argv[i + 1]);
                if (quantum <= 0) return -1;
                i++; // skip that argument on next iteration
            } else return -1;
        }
    }

    return quantum;
}

/* ---------------------------------- HEAP FUNCTIONS ---------------------------------- */

PriorityQueue *CreateHeap(int capacity){
    PriorityQueue *h = (PriorityQueue*) malloc(sizeof(PriorityQueue)); //one is number of heap

    //check if memory allocation is fails
    if(h == NULL){
        printf("Memory Error!");
        return NULL;
    }
    h->count = 0;
    h->capacity = capacity;
    h->arr = malloc(capacity * sizeof(Thread*)); //size in bytes

    //check if allocation succeed
    if ( h->arr == NULL){
        printf("Memory Error!");
        return NULL;
    }
    return h;
}

void insert(PriorityQueue *h, Thread *key){
    if( h->count < h->capacity){
        h->arr[h->count] = malloc(sizeof(Thread));
        if (h->arr[h->count] == NULL) {
            fprintf(stderr, "Malloc() failed for inserting Thread to Priority Queue.\n");
            exit(-1);
        }
        h->arr[h->count]->arrival_time = key->arrival_time;
        h->arr[h->count]->thread_num = key->thread_num;
        //h->arr[h->count]->burst_time[0] = key->burst_time[0]; //TODO UNCOMMENT
        heapify_bottom_top(h, h->count);
        h->count++;
    }
}

void heapify_bottom_top(PriorityQueue *h,int index){
    Thread *temp;
    int parent_node = (index-1)/2;

    if(h->arr[parent_node]->arrival_time > h->arr[index]->arrival_time){
        //swap and recursive call
        temp = h->arr[parent_node];
        h->arr[parent_node] = h->arr[index];
        h->arr[index] = temp;
        heapify_bottom_top(h,parent_node);
    }
}

// parent_node is the index of the parent node
void heapify_top_bottom(PriorityQueue *h, int parent_node){
    int left = parent_node*2+1;
    int right = parent_node*2+2;
    int min;
    Thread *temp;

    if(left >= h->count || left <0)
        left = -1;
    if(right >= h->count || right <0)
        right = -1;

    if(left != -1 && h->arr[left]->arrival_time < h->arr[parent_node]->arrival_time)
        min=left;
    else
        min = parent_node;
    if(right != -1 && h->arr[right]->arrival_time < h->arr[min]->arrival_time)
        min = right;

    if(min != parent_node){
        temp = h->arr[min];
        h->arr[min] = h->arr[parent_node];
        h->arr[parent_node] = temp;

        // recursive  call
        heapify_top_bottom(h, min);
    }
}

Thread* PopMin(PriorityQueue *h){
    Thread* pop;
    if(h->count == 0){
        printf("\n__Heap is Empty__\n");
        return NULL;
    }
    // replace first node by last and delete last
    pop = h->arr[0];
    h->arr[0] = h->arr[h->count-1];
    h->count--;
    heapify_top_bottom(h, 0);
    return pop;
}

void print(PriorityQueue *h){
    int i;
    printf("____________Print Heap_____________\n");
    for(i=0;i< h->count;i++){
        printf("-> THREAD: %d, ARRIVE: %d, BURST: N/A\n", h->arr[i]->thread_num, h->arr[i]->arrival_time); //TODO ADD BURST
    }
    printf("->__/\\__\n");
}
/* -------------------------------------------------------------------- */