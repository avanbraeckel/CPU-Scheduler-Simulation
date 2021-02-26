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
#define MAX_CAPACITY 1000

#define STATE_NEW 0
#define STATE_READY 1
#define STATE_RUNNING 2
#define STATE_BLOCKED 3
#define STATE_TERMINATED 4

// turnaround time is time finished executing - time of start //TODO HERE
//TODO MAKE SURE THAT IF ARRIVAL TIME IS THE SAME, THERE IS A CONSISTENT WAY TO ORDER THEM (DEFINE ANOTHER WAY OF ORDERING THEM - MAYBE PROCESS #)

/* --------------------------------- PROTOTYPES ---------------------------------*/

typedef struct thread_struct {
    int thread_num;
    int arrival_time;
    int burst_num;
    int process_num;
    int turnaround_time;
    int current_burst;
    int time_enters_cpu;
    int *cpu_burst_times;
    int *io_burst_times;
} Thread;

typedef struct heap_struct {
    Thread **arr;
    int count;
    int capacity;
} PriorityQueue;

void get_data(PriorityQueue *pq);
int set_flags(bool* d, bool *v, bool *r, int argc, char *argv[]);

PriorityQueue *CreateHeap(int capacity);
void insert(PriorityQueue *h, Thread *key);
void print(PriorityQueue *h);
void up_heap(PriorityQueue *h,int index);
void down_heap(PriorityQueue *h, int parent_node);
Thread* PopMin(PriorityQueue *h);
void free_thread(Thread *t);

/*-------------------------------------------------------------------------------*/

/* ----------------- MAIN ----------------- */
int main (int argc, char *argv[]) {

    if (argc > 5) { // invalid number of arguments
        fprintf(stderr, "Usage: ./simcpu [-d] [-v] [-r quantum] < input_file\n");
        exit(-1);
    }

    bool d_flag = false;
    bool v_flag = false;
    bool r_flag = false;
    int quantum = -1;
    char line[MAX_LEN];
    int i, j;

    PriorityQueue *pq = CreateHeap(MAX_CAPACITY);

    if (argc > 1) {
        set_flags(&d_flag, &v_flag, &r_flag, argc, argv);
        if (r_flag && quantum <= 0) {
            fprintf(stderr, "Usage: ./simcpu [-d] [-v] [-r quantum] < input_file\n");
            exit(-1);
        }
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
    get_data(pq);

    // PRINT ORIGINAL DATA FROM FILE
    for (i = 0; i < pq->count; i++) {
        printf("PROCESS: %d, THREAD: %d, ARRIVAL: %d\n", pq->arr[i]->process_num, pq->arr[i]->thread_num, pq->arr[i]->arrival_time);
        for (j = 0; j < pq->arr[i]->burst_num; j++) {
            printf("\t\t|%d| CPU: %d", j + 1, pq->arr[i]->cpu_burst_times[j]);
            if (j < pq->arr[i]->burst_num - 1) {
                printf(", IO: %d", pq->arr[i]->io_burst_times[j]);
            }
            printf("\n");
        }
    }
    
    int time_total = 0;
    int cpu_time_total = 0;
    int process_num = 0;
    //int num_iterations = 0; //TODO REMOVE LATER

    // loop while there are still threads in the ready queue
    while (pq->count > 0) {
        Thread *cur_thread = PopMin(pq);

        printf("ARRIVAL TIME of PROCESS %d, THREAD %d, BURST %d : %d\n",  cur_thread->process_num,  cur_thread->thread_num, cur_thread->current_burst + 1, cur_thread->arrival_time);
        
        // not first time through
        if (time_total != 0) {
            
            if (process_num == cur_thread->process_num) { // context switch - same process
                time_total += units_same_switch;
            } else { // context switch - different process
                time_total += units_diff_switch;
            }
            // set time entering CPU for this thread
            cur_thread->time_enters_cpu = time_total;
        }
        printf("\tTIME: %d\n", time_total);

        // update total times
        cpu_time_total += cur_thread->cpu_burst_times[cur_thread->current_burst];
        time_total += cur_thread->cpu_burst_times[cur_thread->current_burst];

        cur_thread->arrival_time = cur_thread->cpu_burst_times[cur_thread->current_burst]
                + cur_thread->io_burst_times[cur_thread->current_burst] + cur_thread->time_enters_cpu;


        process_num = cur_thread->process_num;
        //TODO       I/O TIME NEEDS TO BE ADDRESSED?

        cur_thread->current_burst++;
        
        // add thread back into queue unless it has finished
        if (cur_thread->current_burst < cur_thread->burst_num) {   
            insert(pq, cur_thread);
        } else {
            free_thread(cur_thread);
        }

        
    }

    return 0;
}

/*--------------------------------- HELPER FUNCTIONS ---------------------------------*/

// Gets the data from the input file (from stdin)
void get_data(PriorityQueue *pq) {
    char line[MAX_LEN];
    fgets(line, MAX_LEN - 1, stdin);
    // TODO CHANGE TO FOR LOOP USING NUM_PROCESSES FROM MAIN()??
    while (line != NULL) {
        int num_threads = 0, process_num = 0;
        int i, j;
        sscanf(line, "%d %d", &process_num, &num_threads);
        for (i = 0; i < num_threads; i++) {
            fgets(line, MAX_LEN - 1, stdin);
            Thread temp;
            temp.process_num = process_num;
            sscanf(line, "%d %d %d", &(temp.thread_num), &(temp.arrival_time), &(temp.burst_num));
            temp.cpu_burst_times = malloc(temp.burst_num * sizeof(int));
            temp.io_burst_times = malloc(temp.burst_num * sizeof(int));
            //temp.time_burst_enters_ready = malloc(temp.burst_num & sizeof(int));  //TODO REMOVE LATER
            temp.time_enters_cpu = 0;
            temp.turnaround_time = 0;
            temp.current_burst = 0;
            // get the bursts
            for (j = 0; j < temp.burst_num; j++) { //
                fgets(line, MAX_LEN - 1, stdin);
                if (line == NULL) break;
                int burst;
                if (j < temp.burst_num - 1) {
                    sscanf(line, "%d %d %d", &burst, &(temp.cpu_burst_times[j]), &(temp.io_burst_times[j]));
                } else {
                    sscanf(line, "%d %d", &burst, &(temp.cpu_burst_times[j]));
                    temp.io_burst_times[j] = 0;
                }
            }
            // add the Thread to the Priority Queue
            insert(pq, &temp);
            free(temp.cpu_burst_times);
            free(temp.io_burst_times);
        }
        if ( (fgets(line, MAX_LEN - 1, stdin)) == NULL) break;
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

void free_thread(Thread *t) {
    if (t == NULL) return;
    if (t->cpu_burst_times != NULL) free(t->cpu_burst_times);
    if (t->io_burst_times != NULL) free(t->io_burst_times);
    if (t != NULL) free(t);
    t = NULL;
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

void insert(PriorityQueue *pq, Thread *key){
    int i;
    if( pq->count < pq->capacity){
        pq->arr[pq->count] = malloc(sizeof(Thread));
        if (pq->arr[pq->count] == NULL) {
            fprintf(stderr, "malloc() failed for inserting Thread to Priority Queue.\n");
            exit(-1);
        }
        pq->arr[pq->count]->arrival_time = key->arrival_time;
        pq->arr[pq->count]->thread_num = key->thread_num;
        pq->arr[pq->count]->burst_num = key->burst_num;
        pq->arr[pq->count]->process_num = key->process_num;
        pq->arr[pq->count]->current_burst = key->current_burst;
        pq->arr[pq->count]->time_enters_cpu = key->time_enters_cpu;
        pq->arr[pq->count]->turnaround_time = key->turnaround_time;
        pq->arr[pq->count]->cpu_burst_times = malloc(key->burst_num * sizeof(int));
        pq->arr[pq->count]->io_burst_times = malloc((key->burst_num - 1) * sizeof(int));
        
        for (i = 0; i < key->burst_num; i++) {
            pq->arr[pq->count]->cpu_burst_times[i] = key->cpu_burst_times[i];
            if (i < key->burst_num - 1) {
                pq->arr[pq->count]->io_burst_times[i] = key->io_burst_times[i];
            }
        }
        up_heap(pq, pq->count);
        pq->count++;
    }
}

void up_heap(PriorityQueue *h,int index){
    Thread *temp;
    int parent_node = (index-1)/2;

    if(h->arr[parent_node]->arrival_time > h->arr[index]->arrival_time){
        //swap and recursive call
        temp = h->arr[parent_node];
        h->arr[parent_node] = h->arr[index];
        h->arr[index] = temp;
        up_heap(h,parent_node);
    } else if (h->arr[parent_node]->arrival_time == h->arr[index]->arrival_time
            && h->arr[parent_node]->process_num > h->arr[index]->process_num) { // compares by process number if arrival time is the same
        //swap and recursive call
        temp = h->arr[parent_node];
        h->arr[parent_node] = h->arr[index];
        h->arr[index] = temp;
        up_heap(h,parent_node);
    }
}

// parent_node is the index of the parent node
void down_heap(PriorityQueue *h, int parent_node){
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
        down_heap(h, min);
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
    down_heap(h, 0);
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
/* -------------------------------------------------------------------------------- */