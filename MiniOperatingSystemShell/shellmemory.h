#include<stdlib.h>
#include<stdio.h>
#include "pcb.h"
#include <stdbool.h>

void mem_init(void);
char *var_get_value(char *var);
void var_set_value(char *var, char *value);
void clean_mem(void);
char *get_frame(int framenumber);
int put_frame(char *data, PCB *pcb, bool pagefault);
void resetmem(void);
void resetmempcb(PCB *pcb);
char* path_to_backing_store(char *filename);