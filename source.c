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

struct sembuf sops;

int sem_id, q_id;
/**
 * function to create a message
 * @param a1: struct of map
 * @param a2: cell s
 * @return : rules the message
 */
message msg_gen(map my_map, cell s)
{
  message msg;
  msg.mtype = s.id;
  msg.created_by = getpid();
  msg.origin = s;
  msg.dest = get_dest(my_map, s);
  return msg;
}

/**
 * function to print the messages created
 * @param a1: message msg
 */
void print_msg(message msg)
{
  printf("MSG\n");
  printf("mtype = %ld\n", msg.mtype);
  printf("created by = %d\n", msg.created_by);
  printf("origin = %d\n", msg.origin.id);
  printf("destination id = %d\n", msg.dest.id);
}

/**
 * function to get a random cell destination
 * @param a1: struct of map
 * @param a2: position of a cell
 */
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

/**
 * function to execute the simulation of the sources
 * @param a1: pointer to shared_data memory 
 */
void source_simulation(shared_data *shared)
{
  fprintf(stderr, "CHILD PID: %d INITIALIZED AS SO_SOURCE\n", getpid());
  srand(getpid());
  cell s;
  message msg;
  s = get_new_source(shared);
  sops.sem_num = 0;
  sops.sem_op = -1;
  semop(sem_id, &sops, 1);
  sops.sem_op = 0;
  semop(sem_id, &sops, 1);
  /*------------------------------------------------------------------------------
                               INIZIO SIMULAZIONE
  ------------------------------------------------------------------------------*/
  while (shared->sim_timeout)
  {
    nanosleep((const struct timespec[]){{1, 0L}}, NULL);
    msg = msg_gen(shared->m, s);
    if (msgsnd(q_id, &msg, WHISTLE, 0) < 0)
    {
      if (errno == EINTR)
      {
        break;
      }
      if (errno == EIDRM)
      {
        break;
      }
      if (errno == EINVAL)
      {
        break;
      }
      else
      {
        print_msg(msg);
        TEST_ERROR;
      }
    }
  }
}

/**
 * function to get a random source cell position
 * @param a1:struct of map
 * @return: source's cell position
 */
cell get_random_source(map my_map)
{
  int i, x, y;
  i = 0;
  while (i < 1)
  {
    x = random_extraction(0, SO_WIDTH - 1);
    y = random_extraction(0, SO_HEIGHT - 1);
    if (my_map.city[x][y].type == 2)
    {
      i++;
    }
  }
  return my_map.city[x][y];
}
