#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>

#include "shellmemory.h"

#define SHELL_MEM_LENGTH 1000

#define SHELL_LINE_LENGTH 100
#define VARIABLE_NAME_LENGTH 10

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 2
#endif

#ifndef VARIABLE_STORE_SIZE
#define VARIABLE_STORE_SIZE 10
#endif

struct memory_struct{
	PCB *pcbs[FRAME_STORE_SIZE/3];
	char frames[FRAME_STORE_SIZE/3][3][SHELL_LINE_LENGTH];
	char vars[VARIABLE_STORE_SIZE][VARIABLE_NAME_LENGTH];
	char *values[VARIABLE_STORE_SIZE];
	int counter[FRAME_STORE_SIZE/3];
};

struct memory_struct shellmemory;

// Shell memory functions

void mem_init(void){
	memset(&shellmemory, 0, sizeof(shellmemory));
}

// Set key value pair
void var_set_value(char *var_in, char *value_in) {
	for (int i = 0; i < VARIABLE_STORE_SIZE; i++) {
		if (strcmp(var_in, shellmemory.vars[i]) == 0) {
			// strncpy(shellmemory.vars[i], var_in, sizeof(shellmemory.vars[i]));
			shellmemory.values[i] = strdup(value_in);
			return;
		}
	}

	for (int i = 0; i < VARIABLE_STORE_SIZE; i++) {
		if (shellmemory.vars[i][0] == 0) {
			strncpy(shellmemory.vars[i], var_in, sizeof(shellmemory.vars[i]));
			shellmemory.values[i] = strdup(value_in);
			return;
		}
	}
}

//get value based on input key
char *var_get_value(char *var_in) {
	for (int i = 0; i < VARIABLE_STORE_SIZE; i++) {
		if (strcmp(var_in, shellmemory.vars[i]) == 0) {
			return shellmemory.values[i];
		}
	}

	return NULL;
}

int next_count() {
	int max = 0;
	for (int i = 0; i < FRAME_STORE_SIZE/3; i++) {
		if (shellmemory.counter[i] > max) {
			max = shellmemory.counter[i];
		}
	}

	return max + 1;
}

char *get_frame(int framenumber) {
	shellmemory.counter[framenumber] = next_count();
	return &shellmemory.frames[framenumber][0][0];
}

int lru_victim() {
	int min = 2147483627;
	int index = -1;
	for (int i = 0; i < FRAME_STORE_SIZE/3; i++) {
		if (shellmemory.counter[i] < min) {
			min = shellmemory.counter[i];
			index = i;
		}
	}

	return index;
}

int put_frame(char *data, PCB *pcb, bool pagefault) {
	// find free spot
	int framesize = FRAME_STORE_SIZE/3;
	for (int i = 0; i < FRAME_STORE_SIZE/3; i++) {
		if (shellmemory.frames[i][0][0] == 0) {
			memcpy(&shellmemory.frames[i][0][0], data, 3 * SHELL_LINE_LENGTH);
			shellmemory.pcbs[i] = pcb;
			shellmemory.counter[i] = next_count();
			return i;
		}
	}

	// page fault

	// find victim
	int victim_index = lru_victim();

	// might be unnecessary
	if (pagefault) {
		printf("Page fault! Victim page contents:\n");
		for (int i = 0; i < 3; i++) {
			char *victim = &shellmemory.frames[victim_index][i][0];
			if (victim[0] != 0) {
				int len = strlen(victim);
				printf("%s", victim);
				if (victim[len - 1] != '\n') {
					printf("\n");
				}
			}
		}
		printf("End of victim page contents.\n");
	}

	// overwrite frame store
	memcpy(&shellmemory.frames[victim_index][0][0], data, sizeof(shellmemory.frames[victim_index]));
	
	// page eviction
	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
		if (shellmemory.pcbs[victim_index]->pagetable[i] == victim_index) {
			shellmemory.pcbs[victim_index]->pagetable[i] = -1;
		}
	}
	
	// new pcb owns the frame
	shellmemory.pcbs[victim_index] = pcb;
	shellmemory.counter[victim_index] = next_count();

	return victim_index;
}

/*
* remove all variables
*/
void resetmem(void) {
	for (int i = 0; i < VARIABLE_STORE_SIZE; i++) {
		memset(shellmemory.vars, 0, sizeof(shellmemory.vars));
		memset(shellmemory.values, 0, sizeof(shellmemory.values));
	}
}

/*
* remove pcb related data like frame store
*/
void resetmempcb(PCB *pcb) {
	int framesize = FRAME_STORE_SIZE/3;
	for (int i = 0; i < FRAME_STORE_SIZE/3; i++) {
		if (shellmemory.pcbs[i] && strcmp(shellmemory.pcbs[i]->pid, pcb->pid) == 0) {
			memset(shellmemory.frames[i], 0, sizeof(shellmemory.frames[i]));
			shellmemory.pcbs[i] = NULL;
		}
	}
}

void clean_mem(void){
    memset(&shellmemory, 0, sizeof(shellmemory));
}

char* path_to_backing_store(char *filename) {
	char path[sizeof("./backing_store/") + 32];
	memset(path, 0, sizeof(path));
	strcat(path, "./backing_store/");
	strcat(path, filename);
	// remove(path);
	return strdup(path);
}