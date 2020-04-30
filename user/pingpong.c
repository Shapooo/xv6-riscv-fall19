#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char* argv[])
{
  int parent_p[2], child_p[2];
  char buf[64];
  pipe(parent_p);
  pipe(child_p);

  int pid = fork();
  int my_pid;
  if (pid == 0) {
    my_pid = getpid();
    close(parent_p[1]);
    close(child_p[0]);

    read(parent_p[0], buf, 1);
    printf("%d: received ping\n", my_pid);

    write(child_p[1], buf, 1);

    close(parent_p[0]);
    close(child_p[1]);
  } else {
    my_pid = getpid();
    close(parent_p[0]);
    close(child_p[1]);

    write(parent_p[1], buf, 1);

    read(child_p[0], buf, 1);
    printf("%d: received pong\n", my_pid);

    wait(); /* maybe wait(0) in future */
    close(child_p[0]);
    close(parent_p[1]);
  }
  return 0;
}
