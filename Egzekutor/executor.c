#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "err.h"
#include "utils.h"

#define MAX_TASKS 4096
#define BUFFER_SIZE 1024
#define BUFFER_INPUT 512

typedef struct {
    pthread_t thread_out;
    pthread_t thread_err;
    pthread_t thread_wait;
    pthread_mutex_t mutex_out;
    pthread_mutex_t mutex_err;
    char last_out[BUFFER_SIZE];
    char last_err[BUFFER_SIZE];
    int pipe_out[2];
    int pipe_err[2];
    pid_t childs_pid;
    pid_t pid;
    int status;
} task;

typedef struct {
    int queue[MAX_TASKS];
    int head;
    int tail;
    pthread_mutex_t mutex;
} queue_t;

queue_t thread_queue;

task tasks[MAX_TASKS];

pthread_mutex_t mutex;

int num_tasks = 0;
bool if_task_runs = false;

void write_end(int id){
    if(tasks[id].thread_out != 0){
        pthread_join(tasks[id].thread_out, NULL);
        tasks[id].thread_out = 0;
    }
    if(tasks[id].thread_err != 0){
        pthread_join(tasks[id].thread_err, NULL);
        tasks[id].thread_err = 0;
    }
    if(WIFEXITED(tasks[id].status)){
        printf("Task %d ended: status %d.\n", id, WEXITSTATUS(tasks[id].status));
    } else {
        printf("Task %d ended: signalled.\n", id);
    }
}


void queue_init() {
    thread_queue.head = 0;
    thread_queue.tail = 0;
    pthread_mutex_init(&thread_queue.mutex, NULL);
}

void queue_destroy() {
    pthread_mutex_destroy(&thread_queue.mutex);
}

void queue_put(int id) {
    pthread_mutex_lock(&thread_queue.mutex);
    thread_queue.queue[thread_queue.tail] = id;
    thread_queue.tail++;
    pthread_mutex_unlock(&thread_queue.mutex);
}

void queue_clear() {
    pthread_mutex_lock(&thread_queue.mutex);
    while(thread_queue.head < thread_queue.tail){
        int id;
        id = thread_queue.queue[thread_queue.head];
        thread_queue.head++;
        write_end(id);
    }
    pthread_mutex_unlock(&thread_queue.mutex);
}

void add_or_write(int id){
    pthread_mutex_lock(&mutex);
    if(if_task_runs == true){
        queue_put(id);
    } else {
        write_end(id);
    }
    pthread_mutex_unlock(&mutex);
}

void *waiting(void *arg){
    int id = *((int *) arg);

    waitpid(tasks[id].childs_pid, &tasks[id].status, 0);
    add_or_write(id);

    free(arg);
    return 0;
}

void *reading_out(void *arg){
    int id = *((int *) arg);
    FILE* file = fdopen(tasks[id].pipe_out[0], "r");
    char buffer[BUFFER_SIZE];

    while(true){
        bool check = read_line(buffer, BUFFER_SIZE, file);
        if(!check){
            free(arg);
            fclose(file);
            return 0;
        }
        pthread_mutex_lock(&tasks[id].mutex_out);
        memcpy(tasks[id].last_out, buffer, BUFFER_SIZE);
        pthread_mutex_unlock(&tasks[id].mutex_out);
    }
}

void *reading_err(void *arg){
    int id = *((int *) arg);
    FILE* file = fdopen(tasks[id].pipe_err[0], "r");
    char buffer[BUFFER_SIZE];

    while(true){
        bool check = read_line(buffer, BUFFER_SIZE, file);
        if(!check){
            free(arg);
            fclose(file);
            return 0;
        }
        pthread_mutex_lock(&tasks[id].mutex_err);
        memcpy(tasks[id].last_err, buffer, BUFFER_SIZE);
        pthread_mutex_unlock(&tasks[id].mutex_err);
    }

}

void run(char* program, char** args) {

    int id = num_tasks++;

    ASSERT_SYS_OK(pipe(tasks[id].pipe_out));
    ASSERT_SYS_OK(pipe(tasks[id].pipe_err));

    set_close_on_exec(tasks[id].pipe_out[0], true);
    set_close_on_exec(tasks[id].pipe_out[1], true);
    set_close_on_exec(tasks[id].pipe_err[0], true);
    set_close_on_exec(tasks[id].pipe_err[1], true);

    pid_t pid = fork();

    ASSERT_SYS_OK(pid);

    if(!pid){
        // DZIECKO PIJANE
        ASSERT_SYS_OK(close(tasks[id].pipe_out[0]));
        ASSERT_SYS_OK(close(tasks[id].pipe_err[0]));

        ASSERT_SYS_OK(dup2(tasks[id].pipe_out[1], STDOUT_FILENO));
        ASSERT_SYS_OK(dup2(tasks[id].pipe_err[1], STDERR_FILENO));

        ASSERT_SYS_OK(close(tasks[id].pipe_out[1]));
        ASSERT_SYS_OK(close(tasks[id].pipe_err[1]));

        ASSERT_SYS_OK(execvp(program, args));
        exit(1);
    } else {
        // STARY PIJANY
        printf("Task %d started: pid %d.\n", id, pid);
        tasks[id].childs_pid = pid;
        tasks[id].pid = getppid();

        ASSERT_SYS_OK(close(tasks[id].pipe_out[1]));
        ASSERT_SYS_OK(close(tasks[id].pipe_err[1]));

        int *arg1 = malloc(sizeof(*arg1));
        int *arg2 = malloc(sizeof(*arg2));
        int *arg3 = malloc(sizeof(*arg3));
        *arg1 = id;
        *arg2 = id;
        *arg3 = id;

        ASSERT_ZERO(pthread_create(&tasks[id].thread_out, NULL, reading_out, arg2));
        ASSERT_ZERO(pthread_create(&tasks[id].thread_err, NULL, reading_err, arg3));
        ASSERT_ZERO(pthread_create(&tasks[id].thread_wait, NULL, waiting, arg1));
    }
}


void out(int id) {
    pthread_mutex_lock(&tasks[id].mutex_out);
    printf("Task %d stdout: '%s'.\n", id, tasks[id].last_out);
    pthread_mutex_unlock(&tasks[id].mutex_out);
}

void err(int id){
    pthread_mutex_lock(&tasks[id].mutex_err);
    printf("Task %d stderr: '%s'.\n", id, tasks[id].last_err);
    pthread_mutex_unlock(&tasks[id].mutex_err);
}

void kill_task(int id){
    if(tasks[id].childs_pid){
        kill(tasks[id].childs_pid, SIGINT);
    }
}

void kill_all(){
    for(int i = 0;i < MAX_TASKS;i++){
        if(tasks[i].childs_pid){
            kill(tasks[i].childs_pid, SIGKILL);
        }
        if(tasks[i].thread_wait != 0){
            pthread_join(tasks[i].thread_wait, NULL);
            tasks[i].thread_wait = 0;
        }
        if(tasks[i].thread_out != 0){
            pthread_join(tasks[i].thread_out, NULL);
            tasks[i].thread_out = 0;
        }
        if(tasks[i].thread_err != 0){
            pthread_join(tasks[i].thread_err, NULL);
            tasks[i].thread_err = 0;
        }
        pthread_mutex_destroy(&tasks[i].mutex_out);
        pthread_mutex_destroy(&tasks[i].mutex_err);
    }
}

void init(){
    queue_init();
    pthread_mutex_init(&mutex, NULL);
    for(int i = 0;i < MAX_TASKS;i++){
        tasks[i].status = 0;
        pthread_mutex_init(&tasks[i].mutex_out, NULL);
        pthread_mutex_init(&tasks[i].mutex_err, NULL);
        tasks[i].thread_wait = 0;
        tasks[i].thread_out = 0;
        tasks[i].thread_err = 0;
    }
}

void quit_job(){
    kill_all();
    pthread_mutex_destroy(&mutex);
    queue_destroy();
    exit(0);
}


int main(){
    init();
    char buffer[BUFFER_INPUT];

    while(true){

        queue_clear();

        bool check = read_line(buffer, BUFFER_INPUT, stdin);
        if(!check){quit_job();}
        char** partitioned = split_string(buffer);

        pthread_mutex_lock(&mutex);
        if_task_runs = true;
        pthread_mutex_unlock(&mutex);

        switch(partitioned[0][0]) {
            case 'r':
                run(partitioned[1], &partitioned[1]);
                break;
            case 'o':
                out(atoi(partitioned[1]));
                break;
            case 's':
                usleep(atoi(partitioned[1]) * 1000);
                break;
            case 'e':
                err(atoi(partitioned[1]));
                break;
            case 'k':
                kill_task(atoi(partitioned[1]));
                break;
            case 'q':
                free_split_string(partitioned);
                quit_job();
        }

        pthread_mutex_lock(&mutex);
        if_task_runs = false;
        pthread_mutex_unlock(&mutex);

        free_split_string(partitioned);
    }
}
