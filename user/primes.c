#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int p[2];
  int np[2];

  int finished = 1;

  pipe(p);
  char num;
  char prime;
  for (char i = 2; i < 36; ++i) {
    write(p[1], &i, 1);
  }
  close(p[1]);

  int pid = fork();
  /* fork failed */
  if (pid < 0) {
    fprintf(2, "Fork failed!");

  } else if (pid == 0) {
    /* if it's not generator, filter primes in loop */
    while (1) {
      /* create pipe for communicating with next process */
      pipe(np);
      read(p[0], &prime, 1);
      printf("prime %d\n", prime);

      /* send filtered num to next process */
      while (read(p[0], &num, 1)) {
        if (num % prime != 0) {
          write(np[1], &num, 1);
          /* there is num for next process */
          finished = 0;
        }
      }
      close(np[1]);

      /* if not finished, then fork();
       * and if it's not child, continue to loop */
      if ((finished == 0) && ((pid = fork()) == 0)) {
        p[0] = np[0];
        finished = 1;
        continue;
      }

      if (pid < 0) {
        fprintf(2, "Fork failed!");
      }
      break;
    }
  } else {
    close(p[0]);
    close(p[1]);
  }

  /* wait for childs to stop */
  wait(); /* maybe wait(0) in future */

  return 0;
}
