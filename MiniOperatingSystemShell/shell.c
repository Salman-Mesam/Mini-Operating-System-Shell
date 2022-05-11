#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "interpreter.h"
#include "kernel.h"
#include "shellmemory.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 2
#endif

#ifndef VARIABLE_STORE_SIZE
#define VARIABLE_STORE_SIZE 10
#endif

int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);

int main(int argc, char *argv[]) {

  printf("%s\n", "Shell version 1.1 Created January 2022");
  help();

  // based on https://stackoverflow.com/a/4204758
  DIR *d;
  struct dirent *dir;
  d = opendir("./backing_store");
  char pathbuffer[sizeof("./backing_store/") + sizeof(dir->d_name) + 1];
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      memset(pathbuffer, 0, sizeof(pathbuffer));
      strcat(pathbuffer, "./backing_store/");
      strcat(pathbuffer, dir->d_name);
      remove(pathbuffer);
    }
    closedir(d);
  }
  rmdir("./backing_store");
  mkdir("./backing_store", 0777);
  printf("Frame Store Size = %d; Variable Store Size = %d\n", FRAME_STORE_SIZE,
         VARIABLE_STORE_SIZE);

  char prompt = '$';              // Shell prompt
  char userInput[MAX_USER_INPUT]; // user's input stored here
  int errorCode = 0;              // zero means no error, default

  // init user input
  for (int i = 0; i < MAX_USER_INPUT; i++)
    userInput[i] = '\0';

  // init shell memory
  mem_init();

  ready_queue_initialize();

  // initialize random number generator, for fileID and pid generation
  srand(time(NULL));

  while (1) {
    printf("%c ", prompt);
    fgets(userInput, MAX_USER_INPUT - 1, stdin);

    if (feof(stdin)) {
      freopen("/dev/tty", "r", stdin);
    }

    errorCode = parseInput(userInput);
    if (errorCode == -1)
      exit(99); // ignore all other errors
    memset(userInput, 0, sizeof(userInput));
  }

  return 0;
}

int parseInput(char ui[]) {
  char tmp[200];
  char *words[100];
  int a = 0;
  int b;
  int w = 0; // wordID
  int errorCode;
  for (a = 0; ui[a] == ' ' && a < 1000; a++)
    ; // skip white spaces

  while (ui[a] != '\n' && ui[a] != '\0' && a < 1000) {
    for (b = 0; ui[a] != ';' && ui[a] != '\0' && ui[a] != '\n' &&
                ui[a] != ' ' && a < 1000;
         a++, b++)
      tmp[b] = ui[a]; // extract a word
    tmp[b] = '\0';

    words[w] = strdup(tmp);

    if (ui[a] == ';') {
      w++;

      errorCode = interpreter(words, w);
      if (errorCode == -1) {
        return errorCode;
      }

      a++;
      w = 0;
      for (; ui[a] == ' ' && a < 1000; a++)
        ; // skip white spaces
      continue;
    }

    w++;
    if (ui[a] == '\0') {
      break;
    }
    a++;
  }
  errorCode = interpreter(words, w);

  return errorCode;
}