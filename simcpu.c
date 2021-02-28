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

#define STATE_NEW "NEW"
#define STATE_READY "READY"
#define STATE_RUNNING "RUNNING"
#define STATE_BLOCKED "BLOCKED"
#define STATE_TERMINATED "TERMINATED"

#define NEW_NUM 0
#define READY_NUM 1
#define RUNNING_NUM 2
#define BLOCKED_NUM 3
#define TERMINATED_NUM 4

/* --------------------------------- PROTOTYPES ---------------------------------*/

typedef struct thread_struct {
    int thread_num;
    int num_threads;
    int arrival_time;
    int burst_num;
    int process_num;
    int original_arrival_time;
    int time_finished;
    int current_burst;
    int time_enters_cpu;
    int service_time;
    int io_time;
    int *cpu_burst_times;
    int *io_burst_times;
} Thread;

typedef struct verbose_struct {
    int time;
    int thread_num;
    int process_num;
    int state1;
    int state2;
} VerboseLine;

typedef struct heap_struct {
    Thread **arr;
    int count;
    int capacity;
} PriorityQueue;

int get_data(PriorityQueue *pq);
int set_flags(bool* d, bool *v, bool *r, int argc, char *argv[]);

PriorityQueue *CreateHeap(int capacity);
void insert(PriorityQueue *h, Thread *key);
void print(PriorityQueue *h);
void up_heap(PriorityQueue *h,int index);
void down_heap(PriorityQueue *h, int parent_node);
Thread* PopMin(PriorityQueue *h);
void free_thread(Thread *t);

/* --------------------------------------- MAIN --------------------------------------- */
int main (int argc, char *argv[]) {
    char *states[] = {STATE_NEW, STATE_READY, STATE_RUNNING, STATE_BLOCKED, STATE_TERMINATED};
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
    int total_num_threads;

    PriorityQueue *pq = CreateHeap(MAX_CAPACITY);

    if (argc > 1) {
        quantum = set_flags(&d_flag, &v_flag, &r_flag, argc, argv);
        if (r_flag && quantum <= 0) {
            fprintf(stderr, "Usage: ./simcpu [-d] [-v] [-r quantum] < input_file\n");
            exit(-1);
        }
    }

    if (r_flag == true) {
        printf("Round Robin Scheduling (quantum = %d time units)\n", quantum);
    } else {
        printf("FCFS Scheduling\n");
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
    total_num_threads = get_data(pq);

    if (quantum <= 0) quantum = 1;
    VerboseLine verbose_output[MAX_CAPACITY * 3 / 2 * total_num_threads * quantum];
    int time_total = 0;
    int cpu_time_total = 0;
    int process_num = 0;
    int turnaround_total = 0;
    Thread *finished_threads[total_num_threads];
    int thread_index = 0;
    int thread_num = 0;
    int last_burst_num = 0;
    int verbose_counter = 0;
    bool finished_burst = true;

// --------------------------------------- MAIN SIMULATION LOOP ---------------------------------------
    // loop while there are still threads in the ready queue
    while (pq->count > 0) {
        Thread *cur_thread = PopMin(pq);

        finished_burst = true; // default true

        // Verbose Output for new to ready
        if (v_flag == true && cur_thread->arrival_time == cur_thread->original_arrival_time) { 
            VerboseLine new_verbose;
            new_verbose.process_num = cur_thread->process_num;
            new_verbose.thread_num = cur_thread->thread_num;
            new_verbose.time = cur_thread->arrival_time;
            new_verbose.state1 = NEW_NUM;
            new_verbose.state2 = READY_NUM;
            verbose_output[verbose_counter++] = new_verbose;
        }
        
        // not first time through
        if (time_total != 0) {
            if (process_num == cur_thread->process_num) { // context switch - same process
                if (thread_num != cur_thread->thread_num) { // different thread number
                    time_total += units_same_switch;
                } else if (last_burst_num != cur_thread->current_burst - 1) { // same thread number AND last burst isn't the same
                    time_total += cur_thread->io_burst_times[cur_thread->current_burst - 1];
                }
            } else { // context switch - different process
                time_total += units_diff_switch;
            }
            // set time entering CPU for this thread
            cur_thread->time_enters_cpu = time_total;
            last_burst_num = cur_thread->current_burst - 1;
        }
        // where time total matches "Time Enters CPU" 
        
        // Verbose Output for ready to running
        if (v_flag == true) { 
            VerboseLine new_verbose;
            new_verbose.process_num = cur_thread->process_num;
            new_verbose.thread_num = cur_thread->thread_num;
            new_verbose.time = time_total;
            new_verbose.state1 = READY_NUM;
            new_verbose.state2 = RUNNING_NUM;
            verbose_output[verbose_counter++] = new_verbose;
        }

        // update total times and the arrival time, as well as increment the current burst of the thread
        if (r_flag == true) { // Round Robin calculations
            int remaining_time = cur_thread->cpu_burst_times[cur_thread->current_burst] - quantum;
            if (remaining_time <= 0) { // burst finished
                remaining_time = 0; // reset to 0
                cpu_time_total += cur_thread->cpu_burst_times[cur_thread->current_burst];
                time_total += cur_thread->cpu_burst_times[cur_thread->current_burst];
                cur_thread->arrival_time = cur_thread->cpu_burst_times[cur_thread->current_burst] + 
                        cur_thread->io_burst_times[cur_thread->current_burst] + cur_thread->time_enters_cpu;
                cur_thread->current_burst++;
            } else { // otherwise burst didn't finish
                cpu_time_total += quantum;
                time_total += quantum;
                cur_thread->cpu_burst_times[cur_thread->current_burst] = remaining_time;
                cur_thread->arrival_time = quantum + cur_thread->time_enters_cpu;
                finished_burst = false;
            }
        } else { // FCFS calculations
            cpu_time_total += cur_thread->cpu_burst_times[cur_thread->current_burst];
            time_total += cur_thread->cpu_burst_times[cur_thread->current_burst];
            cur_thread->arrival_time = cur_thread->cpu_burst_times[cur_thread->current_burst]
                    + cur_thread->io_burst_times[cur_thread->current_burst] + cur_thread->time_enters_cpu;
            cur_thread->current_burst++;
        }
        process_num = cur_thread->process_num;
        thread_num = cur_thread->thread_num;
        
        // add thread back into queue unless it has finished
        if (cur_thread->current_burst < cur_thread->burst_num) {   
            insert(pq, cur_thread);

            // Verbose Output for running to blocked and blocked to ready
            if (v_flag == true) { 
                if (finished_burst) {
                    // Verbose Output from running to blocked
                    VerboseLine new_verbose;
                    new_verbose.process_num = cur_thread->process_num;
                    new_verbose.thread_num = cur_thread->thread_num;
                    new_verbose.time = cur_thread->arrival_time - cur_thread->io_burst_times[cur_thread->current_burst - 1];
                    new_verbose.state1 = RUNNING_NUM;
                    new_verbose.state2 = BLOCKED_NUM;
                    verbose_output[verbose_counter++] = new_verbose;
                    // Verbose Output for blocked to ready
                    VerboseLine new_verbose2;
                    new_verbose2.process_num = cur_thread->process_num;
                    new_verbose2.thread_num = cur_thread->thread_num;
                    new_verbose2.time = cur_thread->arrival_time;
                    new_verbose2.state1 = BLOCKED_NUM;
                    new_verbose2.state2 = READY_NUM;
                    verbose_output[verbose_counter++] = new_verbose2;
                } else { // didn't finish CPU burst
                    // Verbose Output from running to ready
                    VerboseLine new_verbose;
                    new_verbose.process_num = cur_thread->process_num;
                    new_verbose.thread_num = cur_thread->thread_num;
                    new_verbose.time = cur_thread->arrival_time;
                    new_verbose.state1 = RUNNING_NUM;
                    new_verbose.state2 = READY_NUM;
                    verbose_output[verbose_counter++] = new_verbose;
                }
            }
            free_thread(cur_thread);
        } else { // last burst finished
            // get thread turnaround time
            cur_thread->time_finished = time_total;
            finished_threads[thread_index++] = cur_thread;

            // Verbose Output for running to terminated
            if (v_flag == true) { 
                // printf("At time %d: Thread %d of Process %d moves from %s to %s\n", time_total,
                //         cur_thread->thread_num, cur_thread->process_num, STATE_RUNNING, STATE_TERMINATED);
                VerboseLine new_verbose;
                new_verbose.process_num = cur_thread->process_num;
                new_verbose.thread_num = cur_thread->thread_num;
                new_verbose.time = time_total;
                new_verbose.state1 = RUNNING_NUM;
                new_verbose.state2 = TERMINATED_NUM;
                verbose_output[verbose_counter++] = new_verbose;
            }
        }

    } // end while loop
// --------------------------------------- END OF MAIN SIMULATION LOOP ---------------------------------------
    // sort threads so they are in order for printing
    for(i = 0; i < total_num_threads - 1; i++) {
        for(j = i + 1; j < total_num_threads; j++) {
            if(finished_threads[i]->process_num > finished_threads[j]->process_num) {
                Thread *temp = finished_threads[i];
                finished_threads[i] = finished_threads[j];
                finished_threads[j] = temp;
            } else if (finished_threads[i]->process_num == finished_threads[j]->process_num && finished_threads[i]->thread_num > finished_threads[j]->thread_num) {
                Thread *temp = finished_threads[i];
                finished_threads[i] = finished_threads[j];
                finished_threads[j] = temp;
            }
        }
    }
    // sort verbose lines so they are in order for printing
    if (v_flag == true) {
        for(i = 0; i < verbose_counter - 1; i++) {
            for(j = i + 1; j < verbose_counter; j++) {
                if(verbose_output[i].time > verbose_output[j].time) {
                    VerboseLine temp = verbose_output[i];
                    verbose_output[i] = verbose_output[j];
                    verbose_output[j] = temp;
                } else if (verbose_output[i].time == verbose_output[j].time && (verbose_output[i].state1 == NEW_NUM || verbose_output[j].state1 == NEW_NUM)
                        && verbose_output[i].state1 > verbose_output[j].state1) {
                    VerboseLine temp = verbose_output[i];
                    verbose_output[i] = verbose_output[j];
                    verbose_output[j] = temp;
                }
            }
        }
    }

    // get the turnaround time total for the processes
    int current_process_num = finished_threads[0]->process_num;
    int highest_time = finished_threads[0]->time_finished;
    int lowest_arrival = finished_threads[0]->original_arrival_time;
    for (i = 0; i < total_num_threads; i++) {
        if (current_process_num == finished_threads[i]->process_num) {
            if (finished_threads[i]->time_finished > highest_time) {
                highest_time = finished_threads[i]->time_finished;
            }
            if (finished_threads[i]->original_arrival_time < lowest_arrival) {
                lowest_arrival = finished_threads[i]->original_arrival_time;
            } 
        } else {
            turnaround_total += highest_time - lowest_arrival; // add turnaround time of the process to the total
            highest_time = finished_threads[i]->time_finished; // new process, so reset highest time and lowest arrival
            lowest_arrival = finished_threads[i]->original_arrival_time;
        }
        current_process_num = finished_threads[i]->process_num;
    }
    // add turnaround time of last process
    turnaround_total += highest_time - lowest_arrival;

    // verbose output
    if (v_flag == true) {
        for (i = 0; i < verbose_counter; i++) {
            printf("At time %d: Thread %d of Process %d moves from %s to %s\n", verbose_output[i].time,
                    verbose_output[i].thread_num, verbose_output[i].process_num, states[verbose_output[i].state1], states[verbose_output[i].state2]);
            for (j = 0; j < total_num_threads; j++) {
                if (verbose_output[i].state2 == TERMINATED_NUM && verbose_output[i].process_num == finished_threads[j]->process_num 
                        && verbose_output[i].thread_num == finished_threads[j]->thread_num)
                    printf("Thread %d of Process %d:\n  arrival time: %d\n  service time: %d units,"
                        " I/O time: %d units, turnaround time: %d units, finish time: %d units\n", finished_threads[j]->thread_num, finished_threads[j]->process_num,
                        finished_threads[j]->original_arrival_time, finished_threads[j]->service_time, finished_threads[j]->io_time,
                        finished_threads[j]->time_finished - finished_threads[j]->original_arrival_time, finished_threads[j]->time_finished);
            }   
        }
    }

    // Default output
    printf("Total Time Required = %d units\nAverage Turnaround Time is %.1f units\nCPU Utilization is %2.1f%%\n", time_total,
            (double)turnaround_total / (double)num_processes, 100 * (double)cpu_time_total / (double)time_total);

    // Detailed Mode output
    if (d_flag == true) {
        for (i = 0; i < total_num_threads; i++) {
            // get service time and I/O time
            printf("Thread %d of Process %d:\n  arrival time: %d\n  service time: %d units,"
                    " I/O time: %d units, turnaround time: %d units, finish time: %d units\n", finished_threads[i]->thread_num, finished_threads[i]->process_num,
                    finished_threads[i]->original_arrival_time, finished_threads[i]->service_time, finished_threads[i]->io_time,
                    finished_threads[i]->time_finished - finished_threads[i]->original_arrival_time, finished_threads[i]->time_finished);
        }
    }

    // free threads 
    for (i = 0; i < total_num_threads; i++) {
        free_thread(finished_threads[i]);
    }
    // free priority queue
    if (pq != NULL && pq->arr != NULL) free(pq->arr);
    if (pq != NULL) free(pq);

    return 0;
}

/*--------------------------------- HELPER FUNCTIONS ---------------------------------*/

// Gets the data from the input file (from stdin) and returns number of threads across all processes
int get_data(PriorityQueue *pq) {
    char line[MAX_LEN];
    int total_threads = 0;
    fgets(line, MAX_LEN - 1, stdin);
    while (line != NULL) {
        int num_threads = 0, process_num = 0;
        int i, j;
        sscanf(line, "%d %d", &process_num, &num_threads);
        total_threads += num_threads;
        for (i = 0; i < num_threads; i++) {
            fgets(line, MAX_LEN - 1, stdin);
            // store the info in a Thread
            Thread temp;
            temp.process_num = process_num;
            temp.num_threads = num_threads;
            sscanf(line, "%d %d %d", &(temp.thread_num), &(temp.arrival_time), &(temp.burst_num));
            temp.cpu_burst_times = malloc(temp.burst_num * sizeof(int));
            temp.io_burst_times = malloc(temp.burst_num * sizeof(int));
            temp.time_enters_cpu = 0;
            temp.time_finished = 0;
            temp.current_burst = 0;
            temp.original_arrival_time = temp.arrival_time;
            temp.service_time = 0;
            temp.io_time = 0;
            // get the bursts
            for (j = 0; j < temp.burst_num; j++) {
                if ( (fgets(line, MAX_LEN - 1, stdin)) == NULL || line == NULL) break;
                int burst;
                if (j < temp.burst_num - 1) {
                    sscanf(line, "%d %d %d", &burst, &(temp.cpu_burst_times[j]), &(temp.io_burst_times[j]));
                    temp.service_time += temp.cpu_burst_times[j];
                    temp.io_time += temp.io_burst_times[j];
                } else {
                    sscanf(line, "%d %d", &burst, &(temp.cpu_burst_times[j]));
                    temp.io_burst_times[j] = 0;
                    temp.service_time += temp.cpu_burst_times[j];
                }
            }
            // add the Thread to the Priority Queue
            insert(pq, &temp);
            free(temp.cpu_burst_times);
            free(temp.io_burst_times);
        }
        if ( (fgets(line, MAX_LEN - 1, stdin)) == NULL) break;
    }
    return total_threads;
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
        // copy all data
        pq->arr[pq->count]->arrival_time = key->arrival_time;
        pq->arr[pq->count]->original_arrival_time = key->original_arrival_time;
        pq->arr[pq->count]->thread_num = key->thread_num;
        pq->arr[pq->count]->burst_num = key->burst_num;
        pq->arr[pq->count]->process_num = key->process_num;
        pq->arr[pq->count]->current_burst = key->current_burst;
        pq->arr[pq->count]->time_enters_cpu = key->time_enters_cpu;
        pq->arr[pq->count]->time_finished = key->time_finished;
        pq->arr[pq->count]->num_threads = key->num_threads;
        pq->arr[pq->count]->cpu_burst_times = malloc(key->burst_num * sizeof(int));
        pq->arr[pq->count]->io_burst_times = malloc(key->burst_num * sizeof(int));
        pq->arr[pq->count]->io_time = key->io_time;
        pq->arr[pq->count]->service_time = key->service_time;
        for (i = 0; i < key->burst_num; i++) { // copy burst times
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
    int left = parent_node * 2 + 1;
    int right = parent_node * 2 + 2;
    int min;
    Thread *temp;

    if(left >= h->count || left <0)
        left = -1;
    if(right >= h->count || right <0)
        right = -1;

    if(left != -1 && h->arr[left]->arrival_time < h->arr[parent_node]->arrival_time) // sort by arrival time
        min = left;
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
    if(h == NULL || h->arr == NULL || h->count == 0){ // return NULL if empty or invalid
        return NULL;
    }
    // replace first node by last and delete last
    pop = h->arr[0];
    h->arr[0] = h->arr[h->count-1];
    h->count--;
    down_heap(h, 0);
    return pop;
}