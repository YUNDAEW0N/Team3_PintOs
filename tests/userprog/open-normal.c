/* Open a file. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
#include <stdio.h>

void
test_main (void) 
{
  int handle = open ("sample.txt");
  // printf("handle: %d\n",handle);
  if (handle < 2)
    fail ("open() returned %d", handle);
}
