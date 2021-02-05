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

int shm_id, shm_id_s, parent_pid, master_pid, stats_handler, q_id, q_id_t, sem_id, sem_id_cell;
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
          if (errno == ESRCH)
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
      if (kill(master_pid, SIGUSR1) < 0)
      {
        if (errno == EINVAL)
        {
        }
        else
        {
          TEST_ERROR;
        }
      }
      if (kill(stats_handler, SIGUSR1) < 0)
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
    break;
  case SIGINT:
    if (getpid() == parent_pid)
    {
      msgctl(q_id, IPC_RMID, NULL);
      msgctl(q_id_t, IPC_RMID, NULL);
      while (shmctl(shm_id, IPC_RMID, NULL))
      {
        TEST_ERROR;
      }
      while (shmctl(shm_id_s, IPC_RMID, NULL))
      {
        TEST_ERROR;
      }
      semctl(sem_id_cell, 0, IPC_RMID);
      semctl(sem_id, 0, IPC_RMID);
    }
    exit(0);
  case SIGUSR1:
    break;
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
  printf("\n******************************************************\n");
  printf("statistiche sui viaggi\n");
  printf("numero viaggi completati %d\n", shared->s.v_comp);
  printf("numero viaggi annullati %d\n", shared->s.v_abort);
  printf("numero viaggi inevasi %ld\n", shared->s.inevasi);
  printf("******************************************************\n");
  printf("SO_SOURCES\n");
  for (i = 0; i < SO_WIDTH; i++)
  {
    for (j = 0; j < SO_HEIGHT; j++)
    {
      if (shared->m.city[i][j].type == 2)
      {
        printf("id %d\n", shared->m.city[i][j].id);
      }
    }
  }
}

/**
 * function to print taxi that made the most travels 
 * @param a1: pointer to struct taxi_stat named best
 */
void print_top_taxis(taxi_stat *best)
{
  double tmp = best->tempo[1];
  double i = tmp / 1000000000;
  printf("******************************************************\n");
  printf("Migliori taxi:\n");
  printf("Maggior numero di celle percorse: TAXI %d con %d celle\n", best->km[0], best->km[1]);
  printf("Maggior tempo nel servire una richiesta: TAXI %ld con %f secondi\n", best->tempo[0], i);
  printf("Maggior numero di clienti: TAXI %d con %d clienti trasportati\n", best->clienti[0], best->clienti[1]);
}

/**
 * function to generate the map 
 * @param a1: struct of settings named rules
 * @param a2: semaphore struct
 * @return : the new map generated
 */
map map_gen(settings rules, struct sembuf sops)
{
  int i, j, x, y, id, c, k, count, togo;
  map new_map;
  int max_holes = (SO_WIDTH * SO_HEIGHT) / 9;
  if (rules.SO_HOLES > max_holes)
  {
    printf(
        "**IMPOSSIBILE CREARE UNA MAPPA ADEGUATA** \n**AMPLIARE LA MAPPA O "
        "RIDURRE IL NUMERO DI SO_HOLES**\n");
    msgctl(q_id, IPC_RMID, NULL);
    msgctl(q_id_t, IPC_RMID, NULL);
    while (shmctl(shm_id, IPC_RMID, NULL))
    {
      TEST_ERROR;
    }
    while (shmctl(shm_id_s, IPC_RMID, NULL))
    {
      TEST_ERROR;
    }
    semctl(sem_id_cell, 0, IPC_RMID);
    semctl(sem_id, 0, IPC_RMID);
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
    while (k < rules.SO_HOLES)
    {
      // if the extracted cell is on the border of the map can't be a hole
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
    return new_map;
  }
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
  new_cell.type = 1;
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
int sem_release(int id_sem, int sem_num)
{
  struct sembuf sops;
  sops.sem_num = sem_num;
  sops.sem_op = 1;
  sops.sem_flg = 0;
  semop(id_sem, &sops, 1);
}

/**
 * function to try the access to the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_reserve(int id_sem, int sem_num)
{
  struct sembuf sops;
  sops.sem_num = sem_num;
  sops.sem_op = -1;
  sops.sem_flg = 0;
  return semop(id_sem, &sops, 1);
}

/**
 * function to test the access to the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_reserve_sim(int id_sem_cell, int sem_num)
{
  struct sembuf sops;
  settings rules;
  rules = cfg(rules);
  int i;
  int r;
  sem_reserve(sem_id, 1);
  sops.sem_num = sem_num;
  sops.sem_op = -1;
  if (r = semop(sem_id_cell, &sops, 1) < 0)
  {
    if (errno == EINTR)
    {
      sem_release(sem_id, 1);
      exit(0);
    }
    if (errno == EINVAL)
    {
      exit(0);
    }
    if (errno == EIDRM)
    {
      exit(0);
    }
    else
    {
      TEST_ERROR;
    }
  }
  sem_release(sem_id, 1);
}

/**
 * function to wait the zero of the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int wait_for_zero(int sem_id, int sem_num)
{
  int i, j;
  struct sembuf sops;
  sops.sem_num = sem_num;
  sops.sem_op = -1;
  semop(sem_id, &sops, 1);
  sops.sem_op = 0;
  return semop(sem_id, &sops, 1);
}

/**
 * function to print the most crossed cells by the taxi
 * @param a1: struct of map
 * @param a2: struct of settings named rules
 */
void top_cells(map my_map, settings rules)
{
  int i, j, k = 0;
  cell topcells[rules.SO_TOP_CELLS];
  cell max;
  for (k = 0; k < rules.SO_TOP_CELLS; k++)
  {
    max.n_attr = 0;
    for (i = 0; i < SO_WIDTH; i++)
    {
      for (j = 0; j < SO_HEIGHT; j++)
      {
        if (max.n_attr < my_map.city[i][j].n_attr)
        {
          max = my_map.city[i][j];
          topcells[k] = max;
        }
      }
    }
    my_map.city[max.x][max.y].n_attr = 0;
  }
  printf("******************************************************\n");
  printf("Le %d TOP_CELLS sono:\n", rules.SO_TOP_CELLS);
  for (k = 0; k < rules.SO_TOP_CELLS; k++)
  {
    printf("Cell id  %d  con %d attraversamenti\n", topcells[k].id, topcells[k].n_attr);
  }
}