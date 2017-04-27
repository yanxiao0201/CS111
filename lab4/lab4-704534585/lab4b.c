//
//
//
//
//  Copyright © 2016 Xiao Yan. All rights reserved.
//

#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <mraa/aio.h>
#include <math.h>
#include <time.h>

#define BUFSIZE 50


char buf[BUFSIZE];
int nread;


void* threadfun(void* output);

int sockfd,sockfd2, portno, portno2;

struct sockaddr_in serv_addr;
struct hostent *server;

int interval = 3;
int stop = 0;
int tempisF = 1;
int invalid = 0;

pthread_t tid2;
FILE * fp;


int main() {
    
    portno = 16000;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(-1);
    }
    
    server = gethostbyname("lever.cs.ucla.edu");
    
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    
    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting to server");
        exit(-1);
    }
    
    write(sockfd, "Port request 704534585",22);
    
    nread = read(sockfd, &portno2, sizeof(int));
    if (nread <=0 ){
        fprintf(stderr,"Error assigning new port.\n");
        exit(-1);
    }
    

    close(sockfd);
    
    
    sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd2 < 0) {
        perror("ERROR opening new socket");
        exit(-1);
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno2);
    
    /* Now connect to the server */
    if (connect(sockfd2, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting to server with new port");
        exit(-1);
    }
    
    //create thread

    fp = fopen("lab4b.log","w+");

    //this thread reads input from terminal to socket
    pthread_create(&tid2, NULL, threadfun, &sockfd2);
    
    int tempdata;

    float r;
    float temperature;
    
    time_t rawtime;
    struct tm* local;
    char finaltime[20];
    
    char temp;
    
    mraa_aio_context tempsensor;
    tempsensor = mraa_aio_init(0);
    
    char message [100];
    

    while(1){
        
        //if (strcmp(buf,"OFF") == 0){
          //  exit(-1);
        //}
        
        if (stop == 0){
            tempdata = mraa_aio_read(tempsensor);
            time(&rawtime);
            local = localtime(&rawtime);
            memset(finaltime,0,20);
            strftime(finaltime, 20, "%H:%M:%S", local);
            
            r = 1023.0/((float)tempdata) - 1.0;
            r = 100000.0 * r;
            temperature = 1.0/(log(r/100000.0)/4275 + 1/298.15) - 273.15;
            
            if(tempisF){
                temperature = temperature * 1.8 + 32;
                temp = 'F';
            }
            else{
                temp = 'C';
            }
            
            memset(message, 0, 100);
            sprintf(message,"[704534585] TEMP=%.1f\n",temperature);
            write(sockfd2, message, 100);
            
            printf("%s",message);
            fprintf(fp,"%s %.1f %c\n",finaltime, temperature, temp);
            fflush(fp);
        }
        sleep(interval);
    }
    
    
    close(sockfd2);
    fclose(fp);
    mraa_aio_close(tempsensor);
    return 0;
}


void* threadfun(void* output){
    int* socket2 = (int*)output;
    memset(buf,0,sizeof(buf));
    while ((nread = read(*socket2, buf, sizeof(buf))) > 0){
        if (strcmp(buf,"OFF") == 0){
            fprintf(fp,"%s\n",buf);
            fflush(fp);
            printf("%s\n",buf);
            close(sockfd2);
            fclose(fp);
            exit(-1);
        }
        else if (strcmp(buf, "STOP") == 0){
            stop = 1;
        }
        else if (strcmp(buf, "START") == 0){
            stop = 0;
        }
        else if (strncmp(buf, "SCALE=", 6) == 0){
            char option = buf[6];
            if (option == 'F'){
                tempisF = 1;
            }
            else if (option == 'C'){
                tempisF = 0;
            }
            else {
                invalid = 1;
            }
        }
        else if (strncmp(buf,"FREQ=",5) == 0){
            char frequency[50];
            memcpy(frequency, buf + 5, sizeof(buf) -5);
            int freq = atoi(frequency);
            if (freq > 0 && freq < 3601){
                interval = freq;
            }
            else{
                invalid = 1;
            }
        }
        else{
            invalid = 1;
        }
        
        if (invalid == 1){
            invalid = 0;
            fprintf(fp,"%s I\n",buf);
            fflush(fp);
            printf("%s I\n",buf);
        }
        else{
            fprintf(fp,"%s\n",buf);
            fflush(fp);
            printf("%s\n",buf);
        }
        memset(buf,0,sizeof(buf));
    };
    
    return NULL;
}


