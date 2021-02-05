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
#define FEED sizeof(f) - sizeof(long)

int parent_pid, q_id, q_id_t, shm_id, shm_id_s, sem_id, sem_id_cell, child_pid, flag, segnale, stats_handler, master_pid;
int *taxis, *sources;

int main()
{
  int i, j, status;
  sigset_t my_mask;
  struct sembuf sops;
  struct sigaction sa;
  unsigned int my_pid;
  struct msqid_ds msqid_ds, *buf;
  taxi_stat *best;
  buf = &msqid_ds;
  sem_id_cell = semget(IPC_PRIVATE, SO_WIDTH * SO_HEIGHT, 0600);
  parent_pid = getpid();
  srand(getpid());
  settings rules;
  feedback f;
  rules = cfg(rules);
  shared_data *shared;
  q_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
  q_id_t = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
  shm_id = shmget(IPC_PRIVATE, sizeof(*shared), 0600);
  shm_id_s = shmget(IPC_PRIVATE, sizeof(*best), 0600);
  sem_id = semget(IPC_PRIVATE, 2, 0600);
  shared = shmat(shm_id, NULL, 0);
  best = shmat(shm_id_s, NULL, 0);
  shared->m = map_gen(rules, sops);
  shared->taxi_timeout = rules.SO_TIMEOUT;
  sa.sa_handler = signal_handler;
  sa.sa_flags = 0; // No special behaviour
  // Create an empty mask
  sigemptyset(&my_mask); //Do not mask any signal
  sa.sa_mask = my_mask;
  //Installation of signal handler
  if (sigaction(SIGINT, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));
  if (sigaction(SIGALRM, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));
  if (sigaction(SIGUSR1, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));
  if (sigaction(SIGUSR2, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));
  /*Semaphores creation*/
  semctl(sem_id, 0, SETVAL, 0); //wait for 0
  semctl(sem_id, 1, SETVAL, 1); //mutex
  /*Semaphores wait for zero*/
  sops.sem_num = 0;
  sops.sem_flg = 0;
  sops.sem_op = rules.SO_TAXI + rules.SO_SOURCES + 2;
  semop(sem_id, &sops, 1);
  /*Semaphores mutex*/
  sops.sem_num = 1;
  sops.sem_flg = 0;
  sops.sem_op = 1;
  semop(sem_id, &sops, 1);
  shared->sim_timeout = 1;
  fprintf(stderr, "MAIN PID: %d \n", getpid());
  /*------------------------------------------------------------------------------
                                  TAXI CREATION
   ------------------------------------------------------------------------------*/
  taxis = calloc(rules.SO_TAXI, sizeof(*taxis));
  for (i = 0; i < rules.SO_TAXI; i++)
  {
    switch (child_pid = fork())
    {
    case -1:
      fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
      break;
    case 0:
      free(taxis);
      taxi cab;
      cab = init_taxi(shared);
      sops.sem_num = 0;
      sops.sem_op = -1;
      semop(sem_id, &sops, 1);
      sops.sem_op = 0;
      semop(sem_id, &sops, 1);
      taxi_simulation(shared, cab);
      exit(0);
    default:
      taxis[i] = child_pid;
      break;
    }
  }
  /*------------------------------------------------------------------------------
                                 SOURCES CREATION
   ------------------------------------------------------------------------------*/
  sources = calloc(rules.SO_SOURCES, sizeof(*sources));
  for (i = 0; i < rules.SO_SOURCES; i++)
  {
    switch (child_pid = fork())
    {
    case -1:
      fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
      break;
    case 0:
      free(sources);
      source_simulation(shared);
      exit(0);
    default:
      sources[i] = child_pid;
      break;
    }
  }
  /*------------------------------------------------------------------------------
                                  MASTER CREATION
  ------------------------------------------------------------------------------*/
  switch (child_pid = fork())
  {
  case -1:
    fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
    break;
  case 0:
    fprintf(stderr, "MASTER PID: %d \n", getpid());
    sops.sem_num = 0;
    sops.sem_op = -1;
    semop(sem_id, &sops, 1);
    sops.sem_op = 0;
    semop(sem_id, &sops, 1);
    while (shared->sim_timeout == 1)
    {
      printf("\n");
      if (nanosleep((const struct timespec[]){{1, 0L}}, NULL) < 0)
      {
        if (errno == EINTR)
        {
          break;
        }
        else
        {
          TEST_ERROR;
        }
      }
      print_map(shared->m);
    }
    exit(0);
  default:
    master_pid = child_pid;
  }
  /*------------------------------------------------------------------------------
                            STATISTICS HANDLER CREATION
  ------------------------------------------------------------------------------*/
  switch (child_pid = fork())
  {
  case -1:
    fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
    break;
  case 0:
    fprintf(stderr, "Stats handler PID: %d \n", getpid());
    sops.sem_num = 0;
    sops.sem_op = -1;
    semop(sem_id, &sops, 1);
    sops.sem_op = 0;
    semop(sem_id, &sops, 1);
    best->km[0] = 0;
    best->km[1] = 0;
    best->tempo[0] = 0;
    best->tempo[1] = 0;
    best->clienti[0] = 0;
    best->clienti[1] = 0;
    while (shared->sim_timeout)
    {
      if (msgrcv(q_id_t, &f, FEED, 0, 0) < 0)
      {
        if (errno == ENOMSG)
        {
        }
        if (errno == EINTR)
        {
          break;
        }
        if (errno == EIDRM)
        {
          break;
        }
        else
        {
          TEST_ERROR;
          break;
        }
      }
      else
      {
        if (best->km[1] < f.new_stats.km[1])
        {
          best->km[0] = f.new_stats.km[0];
          best->km[1] = f.new_stats.km[1];
        }
        if (best->tempo[1] < f.new_stats.tempo[1])
        {
          best->tempo[0] = f.new_stats.tempo[0];
          best->tempo[1] = f.new_stats.tempo[1];
        }
        if (best->clienti[1] < f.new_stats.clienti[1])
        {
          best->clienti[0] = f.new_stats.clienti[0];
          best->clienti[1] = f.new_stats.clienti[1];
        }
      }
    }
    exit(0);
  default:
    stats_handler = child_pid;
  }
  sops.sem_num = 0;
  sops.sem_op = 0;
  semop(sem_id, &sops, 1);
  alarm(rules.SO_DURATION);
  printf("** INIZIO SIMULAZIONE **\n");
  while ((my_pid = wait(&status)) != -1)
  {
  }
  printf("** FINE SIMULAZIONE **\n\n");

  /* MAIN AGAIN */
  printf("** MAPPA AL TERMINE DELLA SIMULAZIONE **\n");
  print_map(shared->m);
  msgctl(q_id, IPC_STAT, buf);
  shared->s.inevasi = buf->msg_qnum;
  print_stats(shared, rules);
  top_cells(shared->m, rules);
  print_top_taxis(best);
  msgctl(q_id, IPC_RMID, 0);
  msgctl(q_id_t, IPC_RMID, 0);
  shmctl(shm_id, IPC_RMID, NULL);
  shmctl(shm_id_s, IPC_RMID, NULL);
  semctl(sem_id, IPC_RMID, 0);
  semctl(sem_id_cell, IPC_RMID, 0);
  exit(EXIT_SUCCESS);
}
