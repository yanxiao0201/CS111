//
//  server.c
//  lab1b
//
//  Created by Xiao Yan on 10/10/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <mcrypt.h>

#define BUFSIZE 50

//a buffer for input data to shell (read by shell)
char readbuf[BUFSIZE];
int nread;

//a buffer for output buffer from shell (write by shell)
char writebuf[BUFSIZE];
int nwrite;

//find shell pid and handle the signal
pid_t shellpid;

void pipe_handler(int);

//threadfrom shell: direct output from shell to terminal
void* threadfromshell(void *);

//create pipe
int pipe_to_shell[2];
int shell_to_pipe[2];

int sockfd, newsockfd, portno;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

MCRYPT td;
int po;
int enc;

int main(int argc, char * argv[]) {
    
    
    //check to see exist of --port and --encrypt
    static struct option long_options[] = {
        {"port", 1, 0, 'p'},
        {"encrypt", 0, 0, 'e'}
    };
    
    po = -1;
    enc = -1;
    
    char * port;
    
    while(1){
        int opt;
        int opt_index = 0;
        opt = getopt_long(argc, argv, "p:e", long_options, &opt_index);
        if (opt <= 0){
            break;
        }
        switch (opt){
            case 'p':
                po = 1;
                port = optarg;
                break;
            case 'e':
                enc = 1;
                break;
            default:
                printf("Usage: server --port={portaddress} [--encrypt]\n\n");
                exit(-1);
        }
    }
    
    if (po < 0){
        printf("Usage: server --port={portaddress} [--encrypt]\n\n");
        exit(-1);
    }
    portno = atoi(port);
    
    if (portno <= 0){
        printf("Usage: server --port={portaddress} [--encrypt]\n\n");
        exit(-1);
    }
    
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(-1);
    }
    
    /* Initialize socket structure */
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(-1);
    }
    
    /* Now start listening for the clients, here process will
     * go in sleep mode and will wait for the incoming connection
     */
    
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    /* Accept actual connection from the client */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    
    if (newsockfd < 0) {
        perror("ERROR on accept");
        exit(-1);
    }
    pipe(pipe_to_shell);
    pipe(shell_to_pipe);
    
    
    shellpid = fork();
    if (shellpid < 0){
        fprintf(stderr,"fork failed\n");
        exit(-1);
    }
    
    else if (shellpid == 0){
        close(0);
        dup(pipe_to_shell[0]);
        close(1);
        dup(shell_to_pipe[1]);
        close(2);
        dup(shell_to_pipe[1]);
        
        close(pipe_to_shell[0]);
        close(pipe_to_shell[1]);
        close(shell_to_pipe[0]);
        close(shell_to_pipe[1]);
        
        
        execlp("/bin/bash", "/bin/bash", NULL);
        //printf("child run bash error");
    }
    
    else {
        close(pipe_to_shell[0]);
        close(shell_to_pipe[1]);
        
        //i/o redirection
        close(0);
        dup(newsockfd);
        close(1);
        dup(newsockfd);
        close(2);
        dup(newsockfd);
        
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
        
        
        pthread_t tid2;
        
        //this thread reads input from socket to shell
        
        pthread_create(&tid2, NULL, threadfromshell, &shell_to_pipe[0]);
        
        while (1){
            nread = read(0,readbuf,sizeof(readbuf));
            if (nread <= 0){
                close(newsockfd);
                kill(shellpid,SIGKILL);
                exit(1);
            }
            else {
                int i;
                for (i = 0; i < nread; i++){
	            if (enc > 0){
		         mdecrypt_generic(td,&readbuf[i],1);
                    } 	
                    write(pipe_to_shell[1], &readbuf[i],1);
                }
                
            }
        }
    }
    if (enc>0){
        mcrypt_generic_deinit (td);
        mcrypt_module_close(td);
    }
    return 0;
}




void *threadfromshell(void *output){
    //signal(SIGPIPE,pipe_handler);
    int* fromshell = (int*)output;
    while (1){
        nwrite = read(*fromshell, writebuf ,sizeof(writebuf));
        if (nwrite <= 0){
            close(newsockfd);
            kill(shellpid,SIGKILL);
            exit(2);
        }
        else {
            //write(1,writebuf,sizeof(writebuf));
            int j;
            for (j = 0; j < nwrite; j++){
		          if (enc > 0){
		             mcrypt_generic(td, &writebuf[j],1);
                  }
                  write(1,&writebuf[j],1);
            }
            
        }
        
    }
    return NULL;
}

 
void pipe_handler(int sigerr){
    close(newsockfd);
    kill(shellpid,SIGKILL);
    exit(2);
}


