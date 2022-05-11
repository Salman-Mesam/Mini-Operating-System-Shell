#ifndef CPU_H
#define CPU_H

#include "pcb.h"

int cpu_get_ip();
void cpu_empty();
void load_PCB_TO_CPU(PCB* pcb);
int cpu_run(int quanta);
#endif
