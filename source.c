#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "map.h"
#include "source.h"
#include "taxi.h"
#include "utils.h"

/*----------------------------------------------------------------------------*/

message msg_gen(map my_map, cell s)
{
  message msg;
  msg.mtype = s.id;
  msg.created_by = getpid();
  msg.origin = s;
  msg.dest = get_dest(my_map, s);
  return msg;
}

/*----------------------------------------------------------------------------*/
void print_msg(message msg)
{
  printf("MSG\n");
  printf("mtype = %ld\n", msg.mtype);
  printf("created by = %d\n", msg.created_by);
  printf("origin = %d\n", msg.origin.id);
  printf("destination id = %d\n", msg.dest.id);
}

cell get_dest(map my_map, cell pos)
{
  int i = 0;
  cell dest;
  while (i < 1)
  {
    dest = get_random_cell(my_map);
    if (pos.id != dest.id)
    {
      i++;
    }
  }
  return dest;
}
