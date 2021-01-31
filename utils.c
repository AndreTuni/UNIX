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

int shm_id, parent_pid, q_id, sem_id, sem_id_cell;
int *taxis, *sources;

/**
 * function to read and set the rules of the simulation
 * @param a1: struct of settings named rules
 * @return : rules of the simulation
 */
settings cfg(settings rules)
{
  FILE *config;
  int i;
  int arrayFile[10];
  config = fopen("custom.cfg", "r");
  for (i = 0; i < 10; i++)
  {
    fscanf(config, "%d", &arrayFile[i]);
  }
  fclose(config);
  rules.SO_HOLES = arrayFile[0];
  rules.SO_TOP_CELLS = arrayFile[1];
  rules.SO_SOURCES = arrayFile[2];
  rules.SO_CAP_MIN = arrayFile[3];
  rules.SO_CAP_MAX = arrayFile[4];
  rules.SO_TAXI = arrayFile[5];
  rules.SO_TIMENSEC_MIN = arrayFile[6];
  rules.SO_TIMENSEC_MAX = arrayFile[7];
  rules.SO_TIMEOUT = arrayFile[8];
  rules.SO_DURATION = arrayFile[9];
  return rules;
}

/**
 * function to handle the different signal errors
 * @param a1: struct of settings named rules
 */
void signal_handler(int sig)
{
  int i, j;
  settings rules;
  rules = cfg(rules);
  shared_data *shared;
  shared = shmat(shm_id, NULL, 0);
  switch (sig)
  {
  case SIGALRM:
    if (getpid() == parent_pid)
    {
      shared->sim_timeout = 0;
      for (i = 0; i < rules.SO_TAXI; i++)
      {
        if (kill(taxis[i], SIGUSR1) < 0)
        {
          if (errno == EINVAL)
          {
          }
          else
          {
            TEST_ERROR
          }
        }
      }
      for (i = 0; i < rules.SO_SOURCES; i++)
      {
        if (kill(sources[i], SIGUSR1) < 0)
        {
          if (errno == EINVAL)
          {
          }
          else
          {
            TEST_ERROR;
          }
        }
      }
    }
    break;
  case SIGINT:
    if (getpid() == parent_pid)
    {
      msgctl(q_id, IPC_RMID, NULL);
      while (shmctl(shm_id, IPC_RMID, NULL))
      {
        TEST_ERROR;
      }
      semctl(sem_id_cell, 0, IPC_RMID);
      semctl(sem_id, 0, IPC_RMID);
    }
    exit(0);

  case SIGUSR1:
    break;
  case SIGUSR2:
    TEST_ERROR;
  }
}

/**
 * function to print the simulation statistics about the 
 * @param a1: pointer to shared_data struct 
 * @param a2: rules of the simulation
 */
void print_stats(shared_data *shared, settings rules)
{
  int i, j;

  printf("******************************************************\n");
  printf("statistiche sui viaggi\n");
  printf("numero viaggi completati %d\n", shared->s.v_comp);
  printf("numero viaggi annullati %d\n", shared->s.v_abort);
  printf("numero viaggi inevasi %ld\n", shared->s.inevasi);
  printf("SO_SOURCES\n");
  for (i = 0; i < SO_HEIGHT; i++)
  {
    for (j = 0; j < SO_WIDTH; j++)
    {
      if (shared->m.city[i][j].type == 2)
      {
        printf("id %d\n", shared->m.city[i][j].id);
      }
    }
  }
}

/**
 * function to generate the map 
 * @param a1: struct of settings named rules
 * @param a2: semaphore struct
 * @return : the new map generated
 */
map map_gen(settings rules, struct sembuf sops)
{
  int i, j, x, y, id, c, k;
  map new_map;
  int max_holes = (SO_WIDTH * SO_HEIGHT) / 9;
  if (rules.SO_HOLES > max_holes)
  {
    printf(
        "**IMPOSSIBILE CREARE UNA MAPPA ADEGUATA** \n**AMPLIARE LA MAPPA O "
        "RIDURRE IL NUMERO DI SO_HOLES**\n");
    exit(0);
  }
  else
  {
    id = 1;
    for (i = 0; i < SO_WIDTH; i++)
    {
      for (j = 0; j < SO_HEIGHT; j++)
      {
        new_map.city[i][j] = cell_gen(rules, i, j, id, sops);
        id++;
      }
    }
    k = 0;
    while (k < rules.SO_HOLES + 1)
    {
      // se la cella estratta è sui bordi della mappa non può essere hole
      x = random_extraction(1, SO_WIDTH - 2);
      y = random_extraction(1, SO_HEIGHT - 2);
      if (check_neighbours(new_map, new_map.city[x][y], x, y) == 1)
      {
        new_map.city[x][y] = hole_gen(new_map.city[x][y]);
        k++;
      }
    }
    k = 0;
    while (k < rules.SO_SOURCES)
    {
      x = random_extraction(0, SO_WIDTH - 1);
      y = random_extraction(0, SO_HEIGHT - 1);
      if (new_map.city[x][y].type == 1)
      {
        new_map.city[x][y] = source_gen(new_map.city[x][y]);
        k++;
      }
    }
  }
  return new_map;
}

/**
 * function to configure the cell following the rules
 * @param a1: struct of settings named rules
 * @param a2: abscissa x on the map
 * @param a3: ordinate y on the map
 * @param a4: id of the cell
 * @param a5: semaphore struct
 * @return : the new cell configurated 
 */
cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops)
{
  cell new_cell;
  new_cell.id = id;
  new_cell.x = x;
  new_cell.y = y;
  new_cell.t_attr = random_extraction(rules.SO_TIMENSEC_MIN, rules.SO_TIMENSEC_MAX);
  new_cell.cap = random_extraction(rules.SO_CAP_MIN, rules.SO_CAP_MAX);
  new_cell.type = 1; // definito dopo
  new_cell.here = 0;
  new_cell.n_attr = 0;
  semctl(sem_id_cell, id - 1, SETVAL, new_cell.cap);
  new_cell.sem_id = sem_id_cell;
  return new_cell;
}

/**
 * function to configure the source cells
 * @param a1: pointer to shared_data struct
 * @return : the source cell s
 */
cell get_new_source(shared_data *shared)
{
  cell s;
  int i = 0;
  while (i < 1)
  {
    s = get_random_source(shared->m);
    if ((shared->m.city[s.x][s.y].status == 0))
    {
      shared->m.city[s.x][s.y].status = 1;
      i++;
    }
  }
  return s;
}

/**
 * function to calculate a random number r
 * @param a1: a
 * @param a2: b
 * @return : the r number
 */
int random_extraction(int a, int b)
{
  int r;
  if (a < b)
  {
    r = (rand() % (b - a + 1)) + a;
  }
  else
  {
    r = (rand() % (a - b + 1)) + b;
  }
  return r;
}

/**
 * function to release the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_release(int sem_id, int sem_num)
{
  struct sembuf sops;
  //printf("pid %d releasing su %d \n", getpid(), sem_num + 1);
  sops.sem_num = sem_num;
  sops.sem_op = 1;
  sops.sem_flg = 0;
  semop(sem_id, &sops, 1);
  //printf("pid %d RELEASED su %d \n", getpid(), sem_num + 1);
}

/**
 * function to try the access to the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_reserve(int sem_id, int sem_num)
{
  struct sembuf sops;
  // printf("pid %d waiting on sem %d\n", getpid(), sem_num);

  sops.sem_num = sem_num;
  sops.sem_op = -1;
  sops.sem_flg = 0;
  return semop(sem_id, &sops, 1);
}

/**
 * function to test the access to the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_reserve_sim(int sem_id, int sem_num)
{
  struct sembuf sops;
  settings rules;
  rules = cfg(rules);
  int i;
  int r;
  printf("pid %d waiting in cell %d\n", getpid(), sem_num + 1);

  sops.sem_num = sem_num;
  sops.sem_op = -1;
  if (r = semop(sem_id, &sops, 1) < 0)
  {
    if (errno == EINTR)
    {
      printf(" pid %d exit 129\n", getpid());

      exit(129);
    }
    else
    {
      TEST_ERROR;
    }
  }
  return r;
}

/**
 * function to wait the zero of the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int wait_for_zero(int sem_id, int sem_num)
{
  struct sembuf sops;
  sops.sem_num = sem_num;
  sops.sem_op = -1;
  semop(sem_id, &sops, 1);
  sops.sem_op = 0;
  return semop(sem_id, &sops, 1);
}

