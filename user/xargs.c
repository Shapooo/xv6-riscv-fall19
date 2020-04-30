#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

#define MAXARG 32
#define BUFSIZE 128
#define ARGLEN 32

char *next_blank(char *point, char *limit) {
  while ((*point != ' ') && (*point != '\n') && (point < limit))
    point++;
  return point;
}

static int fill_argv(char *argv[], int argc, char buf[], int read_size) {
  char *nbp;                   /* next blank pointer */
  char *argp = buf;            /* arg pointer */
  char *end = buf + read_size; /* real end of buf */

  /*   argp   nbp
   *    v      v
   *|___xxxxxxx___xxxxxx */

  /* move argp to first non-blank char */
  while (*argp == ' ')
    argp++;

  while ((nbp = next_blank(argp, end))) {
    /* skip contiguous blank */
    if (nbp != argp) {
      argv[argc] = (char *)malloc(sizeof(char) * ARGLEN);
      memmove(argv[argc], argp, nbp - argp);
      argv[argc][nbp - argp] = '\0';
      argp = ++nbp;
      ++argc;
    } else if (*nbp == '\n' || nbp >= end) {
      break;
    } else
      argp = ++nbp;
  }
  return argc;
}

int main(int argc, char const *argv[]) {
  char buf[BUFSIZE];
  char read_size;
  int exec_argc;
  char *exec_argv[MAXARG];
  if (argc <= 1) { /* there's no command */
    while ((read_size = read(0, buf, BUFSIZE)) > 0) {
      /* get words to print */
      exec_argc = fill_argv(exec_argv, 0, buf, read_size);

      /* if there's empty input, continue */
      if (exec_argc == 0)
        continue;

      /* print args */
      for (int i = 0; i < exec_argc - 1; ++i) {
        printf("%s ", exec_argv[i]);
      }
      printf("%s\n", exec_argv[exec_argc - 1]);

      /* free argv[](s) */
      for (int i = 0; i < exec_argc; ++i) {
        free(exec_argv[i]);
      }
    }
  } else { /* there's command */
    int i;
    int pid;

    /* fill in from xargs' argv */
    for (i = 0; i < argc - 1; i++) {
      exec_argv[i] = malloc(sizeof(char) * ARGLEN);
      strcpy(exec_argv[i], argv[i + 1]);
    }

    while ((read_size = read(0, buf, BUFSIZE)) > 0) {

      /* fill argv to exec */
      exec_argc = fill_argv(exec_argv, i, buf, read_size);

      /* if there's empty input, continue */
      if (exec_argc == i)
        continue;
      exec_argv[exec_argc++] = 0;

      pid = fork();
      if (pid == 0) {
        exec(exec_argv[0], exec_argv);
      } else {
        /* free argv[](s) */
        for (int i = argc - 1; i < exec_argc; ++i) {
          free(exec_argv[i]);
        }
        i = argc - 1;
        if (pid > 0)
          wait(); /* maybe wait(0) in future */
        else
          fprintf(2, "Fork failed!");
      }
    }
    for (int i = 0; i < argc - 1; ++i) {
      free(exec_argv[i]);
    }
  }
  return 0;
}
