#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[])
{
  if (argc < 2) {
    printf("%s: missing operand\n", argv[0]);
    exit(); /* maybe exit(1) in future */
  } else {
    sleep(atoi(argv[1]) * 10);
    exit(); /* maybe exit(0) in future */
  }
}
