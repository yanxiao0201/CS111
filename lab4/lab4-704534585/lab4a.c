//
//  main.c
//  lab4
//
//  Created by Xiao Yan on 12/8/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <mraa/aio.h>

#include <time.h>
#include <math.h>
#include <signal.h>
#include "lcd.h"


int main() {
    // insert code here...
    int tempdata;
    time_t rawtime;
    struct tm* local;
    char finaltime[20];
    char message[50];
    float r;
    float temperature;
    mraa_aio_context tempsensor;
    tempsensor = mraa_aio_init(0);

    begin(16, 2, LCD_5x8DOTS);
    
    FILE * fp;
    fp = fopen("lab4a.log","w+");
    while(1){
        
        clear();
        tempdata = mraa_aio_read(tempsensor);
        
        time(&rawtime);
        local = localtime(&rawtime);
        strftime(finaltime, 20, "%H:%M:%S", local);
        
        r = 1023.0/((float)tempdata) - 1.0;
        r = 100000.0 * r;
        temperature = 1.0/(log(r/100000.0)/4275 + 1/298.15) - 273.15;
        
        temperature = temperature * 1.8 + 32;
        printf("%s %.1f\n",finaltime, temperature);
        
        fprintf(fp,"%s %.1f\n",finaltime, temperature);
        fflush(fp);
        
        setCursor(0, 0);
        printString(finaltime);
        print(' ');
        printFloat(temperature);

        
        sleep(1);
        
    }
    
    fclose(fp);
    mraa_aio_close(tempsensor);
    return 0;
}
