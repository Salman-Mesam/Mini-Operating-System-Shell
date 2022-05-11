#include "kernel.h"
#include "cpu.h"
#include "interpreter.h"
#include "pcb.h"
#include "shell.h"
#include "shellmemory.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

#define QUEUE_LENGTH 10
#define MAX_INT 2147483646
struct pcbhead head;
struct pcbhead finished;
bool is_first = false; // is first scheduled process like run or exec

void ready_queue_initialize() {
  TAILQ_INIT(&head);
  TAILQ_INIT(&finished);
}

void ready_queue_destory() {
  struct PCB *first = TAILQ_FIRST(&head);
  while (first != NULL) {
      struct PCB *next = TAILQ_NEXT(first, entries);
      TAILQ_REMOVE(&head, first, entries);
      free(first);
      first = next;
  }
}

PCB* ready_queue_pop() {
  struct PCB *first = TAILQ_FIRST(&head);
  if (first)
    TAILQ_REMOVE(&head, first, entries);
  return first;
}

void ready_queue_add_to_end(PCB *pPCB) {
  TAILQ_INSERT_TAIL(&head, pPCB, entries);
}

void ready_queue_add_to_front(PCB *pPCB) {
  TAILQ_INSERT_HEAD(&head, pPCB, entries);
}

bool is_ready_empty() {
  return TAILQ_EMPTY(&head);
}

int myinit(const char *filename) {
  FILE *fp;
  int error_code = 0;
  int *start = (int *)malloc(sizeof(int));
  int *end = (int *)malloc(sizeof(int));

  fp = fopen(filename, "rt");
  if (fp == NULL) {
    error_code = 11; // 11 is the error code for file does not exist
    return error_code;
  }

  // generate a random ID as file ID
  char *fileID = (char *)malloc(32);
  sprintf(fileID, "%d", rand());

  // create new file in backing_store
  char path[sizeof("./backing_store/") + 32];
  memset(path, 0, sizeof(path));
  strcat(path, "./backing_store/");
  strcat(path, fileID);
  // printf("%s\n", path);
  FILE *bak = fopen(path, "w+");

  // copy file into backing_store
  char ch = 0;
  while ((ch = fgetc(fp)) != EOF)
    fputc(ch, bak);
  fclose(fp);
  rewind(bak);

  // calculate length of file in terms of line
  int length = 0;
  char buffer[100];
  while(fgets(buffer, sizeof(buffer), bak)) {
    length++;
  }
  rewind(bak);

  PCB *newPCB = makePCB(length, fileID);

  // load the first 2 page into frame store
  bool stop = false;
  for (int j = 0; j < 2 && !stop; j++) {
    char buffer[3][100];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < 3 && !stop; i++) {
      size_t len = 0;
      if (!fgets(buffer[i], sizeof(buffer[i]), bak)) {
        stop = true;
        break;
      }
    }
    int framenumber = put_frame((char *)buffer, newPCB, false);
    newPCB->pagetable[j] = framenumber;
  }

  ready_queue_add_to_end(newPCB);

  fclose(bak);

  return error_code;
}

int get_scheduling_policy_number(char *policy) {
  if (strcmp("FCFS", policy) == 0) {
    return 0;
  } else if (strcmp("SJF", policy) == 0) {
    return 1;
  } else if (strcmp("RR", policy) == 0) {
    return 2;
  } else if (strcmp("AGING", policy) == 0) {
    return 3;
  } else {
    // error code 15
    return 15;
  }
}

/*
 * Variable:  schedulingPolicy
 * --------------------
 * 0: FCFS
 * 1: SJF
 * 2: RR
 * 3: AGING
 */
int scheduler(int policyNumber) {
  if (is_first) {
    return 0;
  }
  is_first = true;

  int error_code = 0;

  int cpu_quanta_per_program = 2;

  // FCFS and SJF: running program will stop when it finishes
  if (policyNumber == 0 || policyNumber == 1) {
    cpu_quanta_per_program = MAX_INT;
  } else if (policyNumber == 3) {
    cpu_quanta_per_program = 1;
  }

  // scheduling logic for 0: FCFS and 2: RR
  if (policyNumber == 0 || policyNumber == 2) {
    // keep running programs while ready queue is not empty
    PCB *firstPCB = NULL;
    while ((firstPCB = ready_queue_pop())) {
      load_PCB_TO_CPU(firstPCB);

      int error_code_load_PCB_TO_CPU =
          cpu_run(cpu_quanta_per_program);
      

      if (error_code_load_PCB_TO_CPU == 2) {
        // the head PCB program has been done, time to reclaim the shell mem

        // remove file from backing_store
        char *path = path_to_backing_store(firstPCB->pid);
        remove(path);
        free(path);
        if (is_first) {
          // the current process is either exec or run

          resetmempcb(firstPCB);
          free(firstPCB);
          is_first = false;
        } else {
          // the current process is other kinds of process
          TAILQ_INSERT_TAIL(&finished, firstPCB, entries);
        }
      }
      if (error_code_load_PCB_TO_CPU == 0) {
        // the head PCB program has finished its quanta, it need to be put to
        // the end of ready queue
        ready_queue_add_to_end(firstPCB);
      }
    }

    // remove all process from frame store
    struct PCB *first = TAILQ_FIRST(&finished);
    while (first != NULL) {
        struct PCB *next = TAILQ_NEXT(first, entries);
        TAILQ_REMOVE(&finished, first, entries);
        resetmempcb(first);
        free(first);
        first = next;
    }
  }

  // // scheduling policy for 1: SJF
  // if (policyNumber == 1) {
  //   while (!is_ready_empty()) {
  //     // task with the lowest lines of codes runs first
  //     int task_index_with_the_least_lines;
  //     int task_lines = MAX_INT;
  //     // get the lowest job length
  //     for (int i = 0; i < QUEUE_LENGTH; i++) {
  //       if ((*readyQueue[i]).pid != NULL &&
  //            (readyQueue[i]->PC) < task_lines) {
  //         task_lines = readyQueue[i]->PC;
  //         task_index_with_the_least_lines = i;
  //       }
  //     }

  //     PCB current_task_PCB = (*readyQueue[task_index_with_the_least_lines]);
  //     load_PCB_TO_CPU(&current_task_PCB);

  //     is_cpu_busy = true;
  //     int error_code_load_PCB_TO_CPU =
  //         cpu_run(cpu_quanta_per_program);
  //     is_cpu_busy = false;

  //     // the head PCB program has been done, time to reclaim the shell mem
  //     char *path = path_to_backing_store(current_task_PCB.pid);
  //     remove(path);
  //     free(path);
  //     // put the current PCB into invalid mode
  //     terminate_task_in_queue_by_index(task_index_with_the_least_lines);
  //   }
  // }

  // // scheduling policy for 3: Aging
  // if (policyNumber == 3) {
  //   int task_index_least_job_length_score;
  //   int task_job_length_score = MAX_INT;

  //   // find job with the lowest job score
  //   for (int i = 0; i < QUEUE_LENGTH; i++) {
  //     // get the lowest job length score
  //     if ((*readyQueue[i]).pid != NULL &&
  //         (*readyQueue[i]).job_length_score < task_job_length_score) {
  //       task_job_length_score = (*readyQueue[i]).job_length_score;
  //       task_index_least_job_length_score = i;
  //     }
  //   }
  //   // move the task with the lowest job score to the front of the queue
  //   PCB job_with_lowest_job_score =
  //       ready_queue_pop(task_index_least_job_length_score, true);
  //   ready_queue_add_to_front(&job_with_lowest_job_score);

  //   while (!is_ready_empty()) {
  //     // task with the lowest job length score runs first
  //     // in this case, the task with the lowest job length score is the first
  //     // task in queue
  //     task_job_length_score = (*readyQueue[0]).job_length_score;
  //     task_index_least_job_length_score = 0;

  //     PCB current_task_PCB = (*readyQueue[task_index_least_job_length_score]);
  //     load_PCB_TO_CPU(&current_task_PCB);

  //     is_cpu_busy = true;
  //     int error_code_load_PCB_TO_CPU =
  //         cpu_run(cpu_quanta_per_program);
  //     is_cpu_busy = false;

  //     if (error_code_load_PCB_TO_CPU == 2) {
  //       // the head PCB program has been done, time to reclaim the shell mem
        
  //       char *path = path_to_backing_store(readyQueue[task_index_least_job_length_score]->pid);
  //       remove(path);
  //       free(path);

  //       ready_queue_pop(task_index_least_job_length_score, true);
  //       task_job_length_score = MAX_INT;
  //     }

  //     if (error_code_load_PCB_TO_CPU == 0) {
  //       // Age all the tasks (other than the current executing task) in queue by
  //       // 1
  //       for (int i = 0; i < QUEUE_LENGTH; i++) {
  //         // get the lowest job length score
  //         if ((*readyQueue[i]).pid != NULL &&
  //             (*readyQueue[i]).job_length_score > 0 &&
  //             i != task_index_least_job_length_score) {
  //           (*readyQueue[i]).job_length_score -= 1;
  //         }
  //       }
  //     }

  //     // if the first task job score is not the lowest,
  //     // then move the frst task to the end
  //     // and the lowest job score task to the front
  //     for (int i = 0; i < QUEUE_LENGTH; i++) {
  //       // get the lowest job length score
  //       if ((*readyQueue[i]).pid != NULL &&
  //           (*readyQueue[i]).job_length_score < task_job_length_score) {
  //         task_job_length_score = (*readyQueue[i]).job_length_score;
  //         task_index_least_job_length_score = i;
  //       }
  //     }
  //     if (task_index_least_job_length_score != 0) {
  //       // pop the task with the lowest job score
  //       PCB lowest_job_score_task =
  //           ready_queue_pop(task_index_least_job_length_score, true);
  //       // move the frst task to the end
  //       PCB first_pcb = ready_queue_pop(0, true);
  //       ready_queue_add_to_end(&first_pcb);
  //       // move the lowest job score task to the front
  //       ready_queue_add_to_front(&lowest_job_score_task);
  //     }
  //   }
  // }

  // clean up
  ready_queue_destory();
  cpu_empty();
  is_first = false;

  return error_code;
}