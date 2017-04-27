//
//  lab0.c
//  lab0
//
//  Created by Xiao Yan on 9/24/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//

#include <stdio.h>//BUFSIZ 512
#include <fcntl.h>//for open,creat,read,write,
#include <stdlib.h>//for exit
#include <signal.h> //for signal
#include <getopt.h> //getopt_long



void sighandler(int);

int main(int argc, char * argv[]){
    
	if(argc < 2){
		printf("Usage: lab0 --input={inputFile} --output={outputFile} [--segfault] [--catch]\n\n");
		exit(-1);
	}

    
    // get char * input, output, segfault, catch
    // if no input, exit(-1)
    static struct option long_options[] = {
        {"input", 1, 0,'i'},
        {"output", 1, 0, 'o'},
        {"segfault", 0, 0, 's'},
        {"catch", 0, 0, 'c'}
    };
    int opt;
    
    
    //indicators for options
    int fd0 = -1;
    int fd1 = -1;
    int seg = -1;
    int cat = -1;
    char* input;
    char* output = NULL;
    
    char buff[BUFSIZ];
    
    
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    
    //printf("%s",optarg);
    //take arg first before read/write
    while (1){
        int opt_index = 0;
        
        opt = getopt_long(argc, argv,"i:o:sc",long_options,&opt_index);
        //printf("%s",optarg);
        if (opt < 0){
            break;
        }//finish taking args
        switch(opt){
            case 'i':
            	input = optarg;
                break;                
            case 'o':
            	output = optarg;                              
                break;                
            case 's':
                seg = 1;
                break;                
            case 'c':
                cat = 1;
                break;               
            default:
                break;
                
        }
        
    }
    
   
   
    if (cat > 0){//set catch
        signal(SIGSEGV,sighandler);       
    }

    if (seg > 0){//set segfault
       char * c = NULL;
       c[0] = 'a';
    }
    
 
   fd0 = open(input,O_RDONLY); 
   if (fd0 < 0){
   		fprintf(stderr, "fail to open %s\n",input);
        perror(input);
        exit(1);
    }
    fd1 = creat(output,mode);  
    if (fd1 < 0){
    	fprintf(stderr, "fail to create %s\n",output);
        perror(output);
        exit(2);
    }
    
   
    // read until EOF
    int n;
    while((n = read(fd0,buff,BUFSIZ)) > 0){
        write(fd1,buff,n);
    }
    
    
    if (fd0 >=0){
        close(fd0);
    }
    if (fd1 >=0){
        close(fd1);
    }
    return 0;
}


void sighandler(int sigerr){
    perror("Caught segfault signal error");
    exit(3);
}

