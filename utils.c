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
#include "utils.h"

/*----------------------------------------------------------------------------*/

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

/*----------------------------------------------------------------------------*/

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


/*----------------------------------------------------------------------------*/

void print_stats(shared_data *shared, settings rules)
{
  int i, j;
  printf("******************************************************\n");
  printf("statistiche sui viaggi\n");
  printf("numero viaggi completati %d\n", shared->s.v_comp);
  printf("numero viaggi annullati %d\n", shared->s.v_abort);
  printf("numero viaggi evasi %d\n", shared->s.evasi);

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

/*----------------------------------------------------------------------------*/
