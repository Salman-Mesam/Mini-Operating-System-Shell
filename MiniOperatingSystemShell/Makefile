mysh: shell.c interpreter.c shellmemory.c pcb.c kernel.c cpu.c 
	gcc -g -D FRAME_STORE_SIZE=$(framesize) -D VARIABLE_STORE_SIZE=$(varmemsize) -c shell.c interpreter.c shellmemory.c pcb.c kernel.c cpu.c
	gcc -g -o  mysh shell.o interpreter.o shellmemory.o pcb.o cpu.o kernel.o

clean: 
	rm mysh; rm *.o
