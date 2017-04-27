//
//  lab2_list.c
//  lab2a
//
//  Created by Xiao Yan on 10/20/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//


#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

#include "SortedList.h"

void rand_str(char *, size_t);
void *listop (void *);
void *list_mutex (void *);
void *list_spin_lock(void *);

int thread;
int iter;
int yield = 0;
int opt_yield;

int opt;
char* yie = "none";
char* sy = "none";
int lis = 1;

//set up the mutex
pthread_mutex_t mutexlist;

//set up spin_lock
int spin_lock;


SortedList_t* headlist;

struct timespec time1, time2, runtime;
struct timespec diff(struct timespec start, struct timespec end);

long threadwait=0;

int main(int argc, char * argv[]) {
    
    
    static struct option long_options[] = {
        {"threads", 1, 0,'t'},
        {"iterations", 1, 0, 'i'},
        {"yield", 1, 0, 'y'},
        {"sync", 1, 0,'s'},
        {"lists", 1, 0, 'l'}
    };
    
    thread = 1;
    iter = 1;
    
    while (1){
        int opt_index = 0;
        opt = getopt_long(argc, argv,"t:i:y:s:l:",long_options,&opt_index);
        
        if (opt < 0){
            break;
        }//finish taking args
        switch(opt){
            case 't':
                thread = atoi(optarg);
                break;
            case 'i':
                iter = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                yie = optarg;
                break;
            case 's':
                sy = optarg;
                break;
            case 'l':
                lis = atoi(optarg);
                break;
            default:
                exit(-1);
                
        }
        
    }
    
    if(opt_yield == 1){
	if (strcmp(yie, "i")==0){
		yield = 1;
	}
	else if (strcmp(yie, "d")==0){
		yield = 2;
	}
	else if (strcmp(yie, "l")==0){
		yield = 4;
	}
	else if (strcmp(yie, "id")==0){
		yield = 3;
	}
	else if (strcmp(yie, "il")==0){
		yield = 5;
	}
	else if (strcmp(yie, "dl")==0){
		yield = 6;
	}
	else if (strcmp(yie, "idl")==0){
		yield = 7;
	}
    }
    
    if (lis > iter) {
        perror("list number too high");
        exit(-1);
    }
    

    int i,offset;
    SortedList_t element[thread*iter];
      

    for (i=0; i < thread*iter; i++){
        char * key = malloc(4 * sizeof(char));
        element[i].key = malloc(4 * sizeof(char));
        rand_str(key,4);
        element[i].key = key;
    }
    
    pthread_t threadlist[thread];
    
    void* (*fun)(void*);
    
    if(strcmp(sy,"m")==0){
        fun = &list_mutex;
        pthread_mutex_init(&mutexlist,NULL);
    }
    else if (strcmp(sy,"s")==0){
        spin_lock = 0;
        fun = &list_spin_lock;
    }
    else {
        fun = &listop;
    }

    headlist = (SortedList_t*)malloc(sizeof(SortedList_t)*lis);
    int count;
    for (count = 0; count < lis; count++){
        (headlist+count)->prev = NULL;
        (headlist+count)->next = NULL;
        (headlist+count)->key = NULL;
    }
  
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for (i = 0; i < thread; i++){
        offset = i * iter;

        pthread_create(&threadlist[i], NULL, fun, (void*)(element+offset));
    }
    
    for (i = 0; i < thread; i++){
        pthread_join(threadlist[i],NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &time2);
    
    int length;
    for (count = 0; count < lis; count++){
        length = SortedList_length(headlist+count);
        if (length!=0){
            printf("Error building/deleting list\n");
            exit(-1);
        }
    }
    
    runtime = diff(time1, time2);
    
    long run_nsec = 1000000000 * runtime.tv_sec + runtime.tv_nsec;
    
    long ope = thread * iter * 3;
    
    long ave_time = run_nsec/ope;
    
    long ave_waitforlock = threadwait/thread;
    
    
    printf("list-%s-%s,%d,%d,%d,%ld,%ld,%ld,%ld\n",yie,sy,thread,iter,lis,ope,run_nsec,ave_time,ave_waitforlock);
    
    if(strcmp(sy,"m")==0){
        pthread_mutex_destroy(&mutexlist);
    }
   
    return 0;
    
}

void rand_str(char *dest, size_t length) {
    char charset[] = "abcdefghijklmnopqrstuvwxyz";
    
    while (length-- > 0) {
        size_t index = rand() % 26;
        *dest++ = charset[index];
    }
    *dest = '\0';
}

void *listop(void* start){
    SortedList_t* first = (SortedList_t*)start;
    SortedList_t* to_del;
    int hashkey;
    int k;
    for (k = 0; k < iter; k++){
        hashkey = atoi((first+k)->key)%lis;
        SortedList_insert((headlist+hashkey), first+k);
    }
    int length = 0;
    int count;
    for (count = 0; count < lis; count++){
        length = length + SortedList_length(headlist+count);
    }
    for (k = 0; k < iter; k++){
        hashkey = atoi((first+k)->key)%lis;
        to_del = SortedList_lookup((headlist+hashkey), (first+k)->key);
        SortedList_delete(to_del);
    }
    
    return NULL;
}

void *list_mutex(void* start){
    SortedList_t* first = (SortedList_t*)start;
    SortedList_t* to_del;
    int hashkey;
    int k;
    
    struct timespec waitlock,getlock,locktime;
    clock_gettime(CLOCK_MONOTONIC, &waitlock);
    pthread_mutex_lock(&mutexlist);
    clock_gettime(CLOCK_MONOTONIC, &getlock);
    locktime = diff(waitlock,getlock);
    threadwait = threadwait + 1000000000 * locktime.tv_sec + locktime.tv_nsec;
    
    for (k = 0; k < iter; k++){
        hashkey = atoi((first+k)->key)%lis;
        SortedList_insert((headlist+hashkey), first+k);
    }
    int length = 0;
    int count;
    for (count = 0; count < lis; count++){
        length = length + SortedList_length(headlist+count);
    }
    for (k = 0; k < iter; k++){
        hashkey = atoi((first+k)->key)%lis;
        to_del = SortedList_lookup((headlist+hashkey), (first+k)->key);
        SortedList_delete(to_del);
    }
    pthread_mutex_unlock(&mutexlist);
    return NULL;
}

void *list_spin_lock(void* start){
    SortedList_t* first = (SortedList_t*)start;
    SortedList_t* to_del;
    int hashkey;
    int k;
    while (__sync_lock_test_and_set(&spin_lock,1));
    for (k = 0; k < iter; k++){
        hashkey = atoi((first+k)->key)%lis;
        SortedList_insert((headlist+hashkey), first+k);
    }
    int length = 0;
    int count;
    for (count = 0; count < lis; count++){
        length = length + SortedList_length(headlist+count);
    }
    for (k = 0; k < iter; k++){
        hashkey = atoi((first+k)->key)%lis;
        to_del = SortedList_lookup((headlist+hashkey), (first+k)->key);
        SortedList_delete(to_del);
    }
    __sync_lock_release(&spin_lock);
    return NULL;
}

struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}


