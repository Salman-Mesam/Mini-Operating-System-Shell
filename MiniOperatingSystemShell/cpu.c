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

/*
 * Struct:  CPU
 * --------------------
 * IP: serve as a pointer to shell memory. Ex. IP = 101 means CPU is executing
 * the 101th line in shellmemory IR: stores the line of code that CPU is
 * executing quanta: how many lines of code could the current task run until it
 * finishes or being switched to another task
 */
struct CPU {
  char IR[1000];
  int quanta;
  PCB *pcb;
};
struct CPU aCPU = {.quanta = 0};

void cpu_empty() { aCPU.quanta = 2; }

void load_PCB_TO_CPU(PCB *pcb) { aCPU.pcb = pcb; }

/*
 * Function:  cpu_run
 * 	Added in A2
 * --------------------
 * run "quanta"(first input parameter) lines of code, starting from the cpu.IP
 *
 * quanta: number of lines that the CPU will run before it switches task or the
 * task ends end: the last line of the task in shell memory returns: error code,
 * 2: file reaches an end, 0: no error
 */
int cpu_run(int quanta) {
  aCPU.quanta = quanta;
  int error_code;
  PCB *pcb = aCPU.pcb;

  while (aCPU.quanta != 0 && aCPU.pcb->PC < aCPU.pcb->length) {
    int pageindex = (pcb->PC / 3);

    if (pcb->pagetable[pageindex] == -1) {
      // start page fault

      // open file
      char *path = path_to_backing_store(pcb->pid);
      FILE *fp = fopen(path, "r");

      // buffer 3 lines
      char buffer[3][100];
      memset(buffer, 0, sizeof(buffer));
      for (int i = 0; i < pcb->PC; i++) {
        fgets(buffer[0], sizeof(buffer), fp);
      }
      for (int i = 0; i < 3; i++) {
        fgets(buffer[i], sizeof(buffer), fp);
      }

      // update/evict frame
      pcb->pagetable[pageindex] = put_frame(buffer[0], pcb, true);

      // close file
      fclose(fp);

      return 0;
    }
    int framenumber = pcb->pagetable[pageindex];
    char *buffer = get_frame(framenumber);

    int pc = pcb->PC;
    pcb->PC = pcb->PC + 1;
    strncpy(aCPU.IR, &buffer[(pc % 3) * 100], 1000);
    parseInput(aCPU.IR);
    aCPU.quanta -= 1;

    if (pcb->PC >= pcb->length) {
      error_code = 2;
      return error_code;
    }
  }

  error_code = 0;
  return error_code;
}