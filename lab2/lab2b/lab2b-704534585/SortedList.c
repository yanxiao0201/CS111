//
//  SortedList.c
//  lab2a
//
//  Created by Xiao Yan on 10/19/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "SortedList.h"


extern int yield;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
    SortedListElement_t * temp = list;
    if (yield == 1 || yield == 3 || yield == 5 || yield == 7){
        pthread_yield();
    }
    //not inserting at the end
    while (temp->next!= NULL){
            if (strcmp(element->key, temp->next->key)>0){
                element->next = temp->next;
                element->prev = temp;
                temp->next->prev = element;
                temp->next = element;
                break;
            }
            else {
                temp = temp->next;
            }
        }
        //inserting at the end
    if (temp->next==NULL){
            temp->next = element;
            element->prev = temp;
            element->next = NULL;
        }
    
}


int SortedList_delete( SortedListElement_t *element){
    SortedListElement_t * tempprev;
    if (yield == 2 || yield == 3 || yield == 6 || yield == 7){
        pthread_yield();
    }

    tempprev = element->prev; 
    if (tempprev->next != element){
	return 1;
        }
    else if(element->next == NULL){
	//delete at the end
	tempprev->next = NULL;
	return 0;
	}
    else if(element->next->prev != element){
	return 1;
	}
    else {
	element->next->prev = tempprev;
	tempprev->next = element->next;
	return 0;
	}
    
    return -1;
    
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
    SortedListElement_t * temp = list;
    if (yield == 4||yield == 5 || yield == 6 || yield ==7){
        pthread_yield();
    }
    while (temp->next != NULL){
        temp = temp->next;
        if (strcmp(temp->key,key)==0){
            return temp;
        }
    }
    return NULL;
    
}


int SortedList_length(SortedList_t *list){
    int pos = 0;
    SortedListElement_t * temp = list;
    SortedListElement_t * tempprev;
    if (temp -> prev!=NULL){
        return -1;
    }
    if (yield == 4 || yield == 5 || yield == 6 || yield ==7){
        pthread_yield();
    }
    while (temp->next!=NULL){
        tempprev = temp;
        temp = temp->next;
        if (temp->prev != tempprev){
            return -1;
        }
        pos = pos + 1;
    }
    
    return pos;

}





























