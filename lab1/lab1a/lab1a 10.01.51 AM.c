//
//  lab1.c
//  lab1
//
//  Created by Xiao Yan on 10/3/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//

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

#define BUFSIZE 50

//a buffer for input data to shell (read by shell)
char readbuf[BUFSIZE];
int nread;

//a buffer for output buffer from shell (write by shell)
char writebuf[BUFSIZE];
int nwrite;

//find shell pid and handle the signal
pid_t shellpid;
void int_handler(int);
void pipe_handler(int);

//threadfrom shell: direct output from shell to terminal

void *threadfromshell(void *output);

//create pipe
int pipe_to_shell[2];
int shell_to_pipe[2];

//for exit
int status;
int my_exit(int);

//for getting and setting terminal mode
struct termios attr1, attr2;

int main(int argc, char * argv[]) {
    
    //check argc <2
    if (argc > 2){
        printf("Usage: lab1a [--shell]\n\n");
        exit(-1);
    }
    
    //check to see exist of --shell
    static struct option long_options[] = {
        {"shell", 0, 0, 's'}
    };
    int sh = -1;
    if (argc == 2){
        int opt;
        int opt_index = 0;
        opt = getopt_long(argc, argv, "s", long_options, &opt_index);
        
        switch (opt){
            case 's':
                sh = 1;
                break;
            default:
                break;
        }
    }

    tcgetattr(STDIN_FILENO, &attr1);
    
    //store attr1
    attr2 = attr1;
    attr1.c_lflag &= ~(ICANON|ECHO);
    attr1.c_cc[VMIN] = 1;
    attr1.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr1);

    //set up pipe for input output
    pipe(pipe_to_shell);
    pipe(shell_to_pipe);
    

        
    
    //if has --shell
    if (sh > 0){
        pid_t rc;
        rc = fork();

        if (rc < 0){
            fprintf(stderr,"fork failed\n");
            exit(-1);
        }
        else if (rc == 0){
        //in child
            //link pipes to shell
            
  
            close(0);
        
            dup(pipe_to_shell[0]);
   
            
            close(1);
            dup(shell_to_pipe[1]);
            close(2);
            dup(shell_to_pipe[1]);
        
            execlp("/bin/bash", "/bin/bash", NULL);
            //printf("child run bash error");

        }
        else {
 
            
            shellpid = rc;
            //signal(SIGINT, int_handler);
            
            signal(SIGPIPE, pipe_handler);
     
            
            
            //in parent
            //create thread
            pthread_t tid2;
       
            

            //this thread reads input from terminal to shell
           
            pthread_create(&tid2, NULL, threadfromshell, &shell_to_pipe[0]);

            while (1){
                
                nread = read(0,readbuf,sizeof(readbuf));
                if (nread <= 0){
                    break;
                }
                else {
                    int i;
                    for (i = 0; i < nread; i++){
                        if (readbuf[i] == '\n'|| readbuf[i] == '\r'){
                            write(1,"\r",1);
                            write(1,"\n",1);
                            write(pipe_to_shell[1],"\n",1);
                        }
                        
                        //if input ^C send a signal to shell
                    
                        else if (readbuf[i] == 3){
                            kill(shellpid, SIGINT);
                         }
                        
                        else if (readbuf[i] == 4){
                            close(pipe_to_shell[0]);
                            close(pipe_to_shell[1]);
                            close(shell_to_pipe[0]);
                            close(shell_to_pipe[1]);
                            kill(shellpid, SIGHUP);
                            tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr2);

                            exit(0);
                        }
                        else {
                            write(1, &readbuf[i],1);
                            write(pipe_to_shell[1], &readbuf[i],1);
                        }
                    }
                    
                }
            }

            
   
           
            
            
        }
    }
    //if there is no --shell
    else {
        while ((nread = read(STDIN_FILENO, readbuf, sizeof(readbuf)))){
            for (int k = 0; k < nread; ++k){
                if (readbuf[k] == 10 || readbuf[k] == 13){
                    write(STDOUT_FILENO, "\r",1);
                    write(STDOUT_FILENO, "\n", 1);
                }
                else if(readbuf[k] == 4){
                    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr2);
                    exit(0);
                }
                else {
                    write(STDOUT_FILENO, &readbuf[k], 1);
                }
            }
        }
        
    }
    
    return 0;
}




void *threadfromshell(void *output){
    
  
    
    int* fromshell = (int*)output;
    while (1){
        nwrite = read(*fromshell, writebuf ,sizeof(writebuf));
        if (nwrite < 0){
            exit(-1);
        }
        else if (nwrite == 0){//EOF
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr2);
            waitpid(shellpid,&status,0);
            fprintf(stdout,"exit, status %d\n", WEXITSTATUS(status));
            exit(1);
        }
        else {
            int j;
            for (j = 0; j < nwrite; j++){
                write(1,&writebuf[j],1);
            }
            
        }
        
    }
    return NULL;
}

void int_handler(int sigerr){
    kill(shellpid,SIGINT);
    
}
void pipe_handler(int sigerr){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr2);
    waitpid(shellpid,&status,0);
    fprintf(stdout,"exit, status %d\n", WEXITSTATUS(status));
    exit(1);
}

/*int my_exit(int n){
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr2);
    waitpid(shellpid,&status,0);
    fprintf(stdout,"exit, status %d\n", WEXITSTATUS(status));
   

    printf("return code : %d\n",n);
    
    exit(n);
    
}
*/