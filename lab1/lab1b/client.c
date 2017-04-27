//
//  lab1b
//  client.c
//
//  Created by Xiao Yan on 10/9/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//

#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mcrypt.h>

#define BUFSIZE 50

//a buffer for input data to shell (read by shell)
char readbuf[BUFSIZE];
int nread;

//a buffer for output buffer from shell (write by shell)
char writebuf[BUFSIZE];
int nwrite;


//threadfrom shell: direct output from shell to terminal
void* threadfromsocket(void* output);

int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;

//for getting and setting terminal mode
struct termios attr1, attr2;

int logfd; //for log

//for encrypt
MCRYPT td;

int enc;
int po;
int logflag;

int main(int argc, char * argv[]) {
 
    //check argc <2
    if (argc < 2){
        printf("Usage: client --port={portaddress} [--log] [--encrypt]\n\n");
        exit(-1);
    }
    
    //check the option of log and encrypt
    static struct option long_options[] = {
        {"log", 1, 0, 'l'},
        {"encrypt", 0, 0, 'e'},
        {"port", 1, 0, 'p'}
        
    };
    
    enc = -1;
    po = -1;
    logflag = -1;
    
    char * port;
    char * logname;
   
    
    while (1){
        int opt;
        int opt_index = 0;
        opt = getopt_long(argc, argv, "l:ep:", long_options, &opt_index);
        if (opt <= 0){
            break;
        }
        switch (opt){
            case 'l':
                logflag = 1;
                logname = optarg;
                break;
            case 'e':
                enc = 1;
                break;
            case 'p':
                po = 1;
                port = optarg;
                break;
            default:
                printf("Usage: client --port={portaddress} [--log] [--encrypt]\n\n");
                exit(-1);
                
        }
    }
    
    if (po < 0){
        printf("Usage: client --port={portaddress} [--log] [--encrypt]\n\n");
        exit(-1);
    }
    

    portno = atoi(port);
    if (portno <= 0){
        printf("Usage: client --port={portaddress} [--log] [--encrypt]\n\n");
        exit(-1);
    }
    
    //establish socket
    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(-1);
    }
    
    server = gethostbyname("localhost");
    
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }
    
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    memcpy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    
    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(-1);
    }
    //set terminal into non-echo mode
    tcgetattr(STDIN_FILENO, &attr1);
    
    attr2 = attr1;
    attr1.c_lflag &= ~(ICANON|ECHO);
    attr1.c_cc[VMIN] = 1;
    attr1.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr1);
    



    if (enc > 0){
        srand(1);
        
        int a;
      
        char * Iv;
        td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
        Iv = malloc(mcrypt_enc_get_iv_size(td));

        for (a=0; a < mcrypt_enc_get_iv_size(td); a++) {
            Iv[a]=rand();
        }

        int fdkey = open("my.key", O_RDONLY);
        char key[20];
        read(fdkey, key, 20);
        close(fdkey);
        
        mcrypt_generic_init(td, key, strlen(key), Iv);
    }
    
    if (logflag > 0){
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        logfd = creat(logname,mode);
    }

    
    //create thread
    pthread_t tid2;
    

    //this thread reads input from terminal to socket
    pthread_create(&tid2, NULL, threadfromsocket, &sockfd);

    while (1){
        nread = read(0, readbuf, sizeof(readbuf));
        if (nread <= 0){
            close(sockfd);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr2);
            exit(1);
        }
        else {
            int i;
            for (i = 0; i < nread; i++){
                        
      //deal with ctrl+d
                if (readbuf[i] == 4){
                    close(sockfd);
                    tcsetattr(STDIN_FILENO, TCSANOW, &attr2);
                    exit(0);
                }
                else {
                    write(1, &readbuf[i],1);

                    if (enc > 0){
                         mcrypt_generic(td, &readbuf[i], 1);
                    }
                    if (logflag > 0){
                        write(logfd,"SENT 1 byte:",12);
                        write(logfd,&readbuf[i],1);
                        write(logfd,"\n",1);
                    }
                    write(sockfd, &readbuf[i],1);
                }
                    
            }
        }
        
    }
    if (enc > 0){
        mcrypt_generic_deinit (td);
        mcrypt_module_close(td);
    }
    return 0;
}


void* threadfromsocket(void* output){
    int * fromsocket = (int*)output;
    while (1){
        nwrite = read(*fromsocket, writebuf ,sizeof(writebuf));
        if (nwrite <= 0){
            close(sockfd);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr2);
            exit(1);
        }
        else{
            //write(1,writebuf,sizeof(writebuf));
            int j;
            for (j = 0; j < nwrite; j++){
                if (logflag > 0){
                    write(logfd,"RECEIVED 1 byte:",16);
                    write(logfd,&writebuf[j],1);
                    write(logfd,"\n",1);
                }
                if (enc > 0){
                    mdecrypt_generic(td, &writebuf[j], 1);
                }

                write(1,&writebuf[j],1);
            }

            
        }
    }
        
    return NULL;
}


