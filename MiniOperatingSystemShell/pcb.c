#include "pcb.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//In this implementation, Pid is the same as file ID 
PCB* makePCB(int length, char* pid){
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = pid;
    newPCB->PC = 0;
    newPCB->job_length_score = length;
    newPCB->length = length;
    memset(&newPCB->pagetable, -1, sizeof(newPCB->pagetable));
    return newPCB;
}