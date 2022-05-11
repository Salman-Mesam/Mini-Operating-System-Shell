#ifndef PCB_H
#define PCB_H

#include <stdbool.h>

#include <sys/queue.h>

#define PAGE_TABLE_SIZE 100

/*
 * Struct:  PCB 
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 */
typedef struct PCB
{
    char* pid;
    int PC;
    int job_length_score;
    int length;
    int pagetable[PAGE_TABLE_SIZE];
    TAILQ_ENTRY(PCB) entries;
}PCB;
TAILQ_HEAD(pcbhead, PCB);

PCB * makePCB(int length, char* pid);
#endif
