#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROCESSES 10
#define TIME_QUANTUM 2

// Process structure
typedef struct {
    int pid;
    char name[20];      
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int start_time;
} Process;

// Gantt chart entry
typedef struct {
    int pid;
    char name[20];       
    int start;
    int end;
} GanttEntry;

GanttEntry gantt[100];
int gantt_index = 0;

// Deadlock simulation mutexes
pthread_mutex_t resource1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resource2 = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void fcfs_scheduling(Process proc[], int n);
void sjf_scheduling(Process proc[], int n);
void priority_scheduling(Process proc[], int n);
void round_robin_scheduling(Process proc[], int n, int quantum);
void calculate_times(Process proc[], int n);
void print_gantt_chart();
void *draw_gantt_thread(void *arg);
void demonstrate_deadlock();
void *deadlock_thread1(void *arg);
void *deadlock_thread2(void *arg);
void add_gantt_entry(Process p, int start, int end);

// Add entry to Gantt chart
void add_gantt_entry(Process p, int start, int end) {
    gantt[gantt_index].pid = p.pid;
    strcpy(gantt[gantt_index].name, p.name);
    gantt[gantt_index].start = start;
    gantt[gantt_index].end = end;
    gantt_index++;
}

// FCFS Scheduling
void fcfs_scheduling(Process proc[], int n) {
    int current_time = 0;
    for (int i = 0; i < n; i++) {
        if (current_time < proc[i].arrival_time)
            current_time = proc[i].arrival_time;

        proc[i].start_time = current_time;
        add_gantt_entry(proc[i], current_time, current_time + proc[i].burst_time);

        current_time += proc[i].burst_time;
        proc[i].completion_time = current_time;
        proc[i].turnaround_time = proc[i].completion_time - proc[i].arrival_time;
        proc[i].waiting_time = proc[i].turnaround_time - proc[i].burst_time;
    }
}

// SJF Scheduling
void sjf_scheduling(Process proc[], int n) {
    int current_time = 0, completed = 0;
    bool done[MAX_PROCESSES] = {0};

    while (completed != n) {
        int idx = -1;
        int min_burst = 9999;
        for (int i = 0; i < n; i++) {
            if (!done[i] && proc[i].arrival_time <= current_time) {
                if (proc[i].burst_time < min_burst) {
                    min_burst = proc[i].burst_time;
                    idx = i;
                }
                if (proc[i].burst_time == min_burst && proc[i].arrival_time < proc[idx].arrival_time)
                    idx = i;
            }
        }
        if (idx != -1) {
            proc[idx].start_time = current_time;
            add_gantt_entry(proc[idx], current_time, current_time + proc[idx].burst_time);

            current_time += proc[idx].burst_time;
            proc[idx].completion_time = current_time;
            proc[idx].turnaround_time = proc[idx].completion_time - proc[idx].arrival_time;
            proc[idx].waiting_time = proc[idx].turnaround_time - proc[idx].burst_time;
            done[idx] = true;
            completed++;
        } else
            current_time++;
    }
}

// Priority Scheduling
void priority_scheduling(Process proc[], int n) {
    int current_time = 0, completed = 0;
    bool done[MAX_PROCESSES] = {0};

    while (completed != n) {
        int idx = -1;
        int highest_priority = 9999;
        for (int i = 0; i < n; i++) {
            if (!done[i] && proc[i].arrival_time <= current_time) {
                if (proc[i].priority < highest_priority) {
                    highest_priority = proc[i].priority;
                    idx = i;
                }
                if (proc[i].priority == highest_priority && proc[i].arrival_time < proc[idx].arrival_time)
                    idx = i;
            }
        }
        if (idx != -1) {
            proc[idx].start_time = current_time;
            add_gantt_entry(proc[idx], current_time, current_time + proc[idx].burst_time);

            current_time += proc[idx].burst_time;
            proc[idx].completion_time = current_time;
            proc[idx].turnaround_time = proc[idx].completion_time - proc[idx].arrival_time;
            proc[idx].waiting_time = proc[idx].turnaround_time - proc[idx].burst_time;
            done[idx] = true;
            completed++;
        } else
            current_time++;
    }
}

// Round Robin Scheduling
void round_robin_scheduling(Process proc[], int n, int quantum) {
    int current_time = 0, completed = 0;
    Process temp[MAX_PROCESSES];
    for (int i = 0; i < n; i++) {
        temp[i] = proc[i];
        temp[i].remaining_time = temp[i].burst_time;
        temp[i].start_time = -1;
    }

    int queue[MAX_PROCESSES], front = 0, rear = 0, visited[MAX_PROCESSES] = {0};
    queue[rear++] = 0; visited[0] = 1;

    while (completed != n) {
        if (front == rear) {
            for (int i = 0; i < n; i++)
                if (temp[i].remaining_time > 0 && temp[i].arrival_time <= current_time && !visited[i])
                    queue[rear++] = i, visited[i] = 1;
            if (front == rear) { current_time++; continue; }
        }

        int idx = queue[front++];
        if (temp[idx].start_time == -1) temp[idx].start_time = current_time;

        int exec = (temp[idx].remaining_time < quantum) ? temp[idx].remaining_time : quantum;
        add_gantt_entry(temp[idx], current_time, current_time + exec);
        current_time += exec;
        temp[idx].remaining_time -= exec;

        for (int i = 0; i < n; i++)
            if (temp[i].remaining_time > 0 && temp[i].arrival_time <= current_time && !visited[i])
                queue[rear++] = i, visited[i] = 1;

        if (temp[idx].remaining_time > 0) queue[rear++] = idx;
        else {
            temp[idx].completion_time = current_time;
            temp[idx].turnaround_time = temp[idx].completion_time - temp[idx].arrival_time;
            temp[idx].waiting_time = temp[idx].turnaround_time - temp[idx].burst_time;
            completed++;
        }
    }

    for (int i = 0; i < n; i++) proc[i] = temp[i];
}

// Print table & averages
void calculate_times(Process proc[], int n) {
    float avg_wt = 0, avg_tat = 0;
    printf("\nPID\tName\tArrival\tBurst\tPriority\tCompletion\tTurnaround\tWaiting\n");
    printf("--------------------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%s\t%d\t%d\t%d\t\t%d\t\t%d\t\t%d\n",
               proc[i].pid, proc[i].name, proc[i].arrival_time, proc[i].burst_time,
               proc[i].priority, proc[i].completion_time,
               proc[i].turnaround_time, proc[i].waiting_time);
        avg_wt += proc[i].waiting_time;
        avg_tat += proc[i].turnaround_time;
    }
    printf("\nAverage Waiting Time: %.2f\n", avg_wt / n);
    printf("Average Turnaround Time: %.2f\n", avg_tat / n);
}

// Gantt chart printing
void print_gantt_chart() {
    printf("\n--- GANTT CHART ---\n|");
    for (int i = 0; i < gantt_index; i++) {
        int len = strlen(gantt[i].name);
        for (int j = 0; j < len; j++) printf("-");
        printf("%s", gantt[i].name);
        for (int j = 0; j < len; j++) printf("-");
        printf("|");
    }
    printf("\n%d", gantt[0].start);
    for (int i = 0; i < gantt_index; i++) {
        int len = strlen(gantt[i].name) * 2 + 2;
        for (int j = 0; j < len; j++) printf(" ");
        printf("%d", gantt[i].end);
    }
    printf("\n");
}

// Thread to display Gantt chart
void *draw_gantt_thread(void *arg) {
    sleep(1);
    print_gantt_chart();
    return NULL;
}

// Deadlock demonstration
void *deadlock_thread1(void *arg) {
    printf("\n[Deadlock Thread1] Lock Resource1\n");
    pthread_mutex_lock(&resource1); sleep(1);
    printf("[Deadlock Thread1] Lock Resource2\n");
    pthread_mutex_lock(&resource2);
    pthread_mutex_unlock(&resource2); pthread_mutex_unlock(&resource1);
    printf("[Deadlock Thread1] Released Resources\n");
    return NULL;
}
void *deadlock_thread2(void *arg) {
    printf("[Deadlock Thread2] Lock Resource2\n");
    pthread_mutex_lock(&resource2); sleep(1);
    printf("[Deadlock Thread2] Lock Resource1\n");
    pthread_mutex_lock(&resource1);
    pthread_mutex_unlock(&resource1); pthread_mutex_unlock(&resource2);
    printf("[Deadlock Thread2] Released Resources\n");
    return NULL;
}
void demonstrate_deadlock() {
    pthread_t t1, t2;
    printf("\n=== DEADLOCK DEMO ===\n");
    pthread_create(&t1,NULL,deadlock_thread1,NULL);
    sleep(1);
    pthread_create(&t2,NULL,deadlock_thread2,NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
}

// Main
int main() {
    Process processes[MAX_PROCESSES];
    int n, choice;
    printf("========================================================\n");
    printf("     CPU SCHEDULING VISUALIZER - OS PROJECT \n");
    printf("========================================================\n");
    printf("Enter number of processes (1-%d): ", MAX_PROCESSES);
    while (scanf("%d",&n)!=1 || n<1 || n>MAX_PROCESSES) {
        printf("Invalid input! Enter 1-%d: ", MAX_PROCESSES);
        while(getchar()!='\n');
    }

    // Fork child for input
    int pipe_fd[2]; pipe(pipe_fd);
    pid_t pid=fork();
    if(pid==0){
        close(pipe_fd[0]);
        for(int i=0;i<n;i++){
            processes[i].pid=i+1;
            printf("\nProcess %d Name: ", i+1);
            while(scanf("%s", processes[i].name)!=1){
        printf("Invalid! Enter name: ");
        while(getchar()!='\n');
    }
            printf("Arrival Time: ");
            while(scanf("%d",&processes[i].arrival_time)!=1 || processes[i].arrival_time<0){
                printf("Invalid! Enter >=0: ");
                while(getchar()!='\n');
            }
            printf("Burst Time: ");
            while(scanf("%d",&processes[i].burst_time)!=1 || processes[i].burst_time<=0){
                printf("Invalid! Enter >0: ");
                while(getchar()!='\n');
            }
            printf("Priority (1=high): ");
            while(scanf("%d",&processes[i].priority)!=1 || processes[i].priority<=0){
                printf("Invalid! Enter >0: ");
                while(getchar()!='\n');
            }
            processes[i].remaining_time=processes[i].burst_time;
        }
        write(pipe_fd[1],processes,sizeof(Process)*n);
        close(pipe_fd[1]); exit(0);
    }else{
        close(pipe_fd[1]); wait(NULL);
        read(pipe_fd[0],processes,sizeof(Process)*n); close(pipe_fd[0]);

        printf("\nSelect Scheduling:\n1.FCFS 2.SJF 3.Priority 4.Round Robin 5.Deadlock\nChoice: ");
        while(scanf("%d",&choice)!=1 || choice<1 || choice>5){
            printf("Invalid! Enter 1-5: ");
            while(getchar()!='\n');
        }

        gantt_index=0;
        Process temp[MAX_PROCESSES]; for(int i=0;i<n;i++) temp[i]=processes[i];

        pthread_t gantt_thread;
        switch(choice){
            case 1: fcfs_scheduling(temp,n); break;
            case 2: sjf_scheduling(temp,n); break;
            case 3: priority_scheduling(temp,n); break;
            case 4: round_robin_scheduling(temp,n,TIME_QUANTUM); break;
            case 5: demonstrate_deadlock(); return 0;
        }

        calculate_times(temp,n);
        pthread_create(&gantt_thread,NULL,draw_gantt_thread,NULL);
        pthread_join(gantt_thread,NULL);
    }
    return 0;
}