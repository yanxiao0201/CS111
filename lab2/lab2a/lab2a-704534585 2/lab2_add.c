//
//  main.c
//  lab2a
//
//  Created by Xiao Yan on 10/17/16.
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

void add(long long *, long long);
void *add_none(void *);
void *add_m(void *);
void *add_s(void *);
void *add_c(void *);

int thread;
int iter;
int opt;
char* sy = "none";

void * status;

int opt_yield;

//set up the mutex
pthread_mutex_t mutexadd;

//set up spin_lock
int spin_lock;

//for getting time
struct timespec time1, time2, runtime;
struct timespec diff(struct timespec start, struct timespec end);

int main(int argc, char * argv[]) {
    

    static struct option long_options[] = {
        {"threads", 1, 0,'t'},
        {"iterations", 1, 0, 'i'},
	    {"yield", 0, 0, 'y'},
        {"sync", 1, 0, 's'}
        
    };
    
    thread = 1;
    iter = 1;

    while (1){
        int opt_index = 0;
        
        opt = getopt_long(argc, argv,"t:i:y",long_options,&opt_index);
        
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
		        break;
            case 's':
                sy = optarg;
                break;
            default:
                exit(-1);
                
        }
        
    }

    long long counter = 0;
    int i;
    pthread_t threadlist[thread];
    
    void* (*fun)(void*);
    
    if(strcmp(sy,"m")==0){
        fun = &add_m;
        pthread_mutex_init(&mutexadd,NULL);
    }
    else if (strcmp(sy,"s")==0){
        spin_lock = 0;
        fun = &add_s;
    }
    else if (strcmp(sy, "c")==0){
        fun = &add_c;
        
    }
    else {
        fun = &add_none;
    }
        
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for (i = 0; i < thread; i++){
        pthread_create(&threadlist[i], NULL, *fun, (void*)&counter);
    }
    
    for (i = 0; i < thread; i++){
        pthread_join(threadlist[i],&status);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &time2);
    
    runtime = diff(time1, time2);
    
    long run_nsec = 1000000000 * runtime.tv_sec + runtime.tv_nsec;
    
    long ope = thread * iter * 2;
    
    int ave_time = run_nsec/ope;
    
    char * info = "add-";
    if (opt_yield){
	info = "add-yield-";
    }
    
    printf("%s%s,%d,%d,%ld,%ld,%d,%lld\n",info,sy,thread,iter,ope,run_nsec,ave_time,counter);
    
    if(strcmp(sy,"m")==0){
        pthread_mutex_destroy(&mutexadd);
    }
    
    return 0;
}


void add(long long *pointer, long long value){
    long long sum = *pointer + value;
    if (opt_yield){
	    pthread_yield();
    }
    *pointer = sum;
}

void *add_none(void *arg){
    long long * pcounter = (long long *)arg;
    int k;
    for (k = 0; k < iter; k++){
	add(pcounter, 1);
    }
    for (k = 0; k < iter; k++){
	 add(pcounter, -1);
    }
}

void *add_m(void *arg){
    long long * pcounter = (long long *)arg;
    int k;
    pthread_mutex_lock(&mutexadd);
    for (k = 0; k < iter; k++){
        add(pcounter, 1);
    }
    for (k = 0; k < iter; k++){
        add(pcounter, -1);
    }
    pthread_mutex_unlock(&mutexadd);
    
}


void *add_s(void *arg){
    long long * pcounter = (long long *)arg;
    int k;
    while (__sync_lock_test_and_set(&spin_lock,1));
    
    for (k = 0; k < iter; k++){
        add(pcounter, 1);
    }
    for (k = 0; k < iter; k++){
        add(pcounter, -1);
    }
        
    __sync_lock_release(&spin_lock);

}

void *add_c(void *arg){
    long long * pcounter = (long long *)arg;
    long long pres, i;
    for (i =0; i<iter; i++){
        do{
            pres = *pcounter;
            if (opt_yield){
                pthread_yield();
            }
        }while(__sync_val_compare_and_swap(pcounter,pres,pres+1)!=pres);
    }
    
    for (i =0; i<iter; i++){
        do{
            pres = *pcounter;
            if (opt_yield){
                pthread_yield();
            }
        }while(__sync_val_compare_and_swap(pcounter,pres,pres-1)!=pres);
    }
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






