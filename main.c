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

#define SIZE sizeof(msg) - sizeof(long)

int parent_pid, q_id, shm_id, sem_id, sem_id_cell, child_pid, flag, segnale;
int *taxis, *sources;

int main()
{
  int i, j, status;
  sigset_t my_mask;
  struct sembuf sops;
  struct sigaction sa;
  unsigned int my_pid;
  sem_id_cell = semget(IPC_PRIVATE, SO_WIDTH * SO_HEIGHT, 0600);
  parent_pid = getpid();
  srand(getpid());
  settings rules;
  message msg;
  rules = cfg(rules);
  shared_data *shared;
  shm_id = shmget(IPC_PRIVATE, sizeof(*shared), 0600);
  sem_id = semget(IPC_PRIVATE, 4, 0600);
  q_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
  shared = shmat(shm_id, NULL, 0);
  shared->m = map_gen(rules, sops);
  shared->taxi_timeout = rules.SO_TIMEOUT;

  sa.sa_handler = signal_handler;
  sa.sa_flags = 0; // No special behaviour
  // Create an empty mask
  sigemptyset(&my_mask); // Do not mask any signal
  sa.sa_mask = my_mask;
  // insatallazzione signal handler

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

  /*creazione semafori*/
  semctl(sem_id, 0, SETVAL, 0);
  semctl(sem_id, 1, SETVAL, 1);
  semctl(sem_id, 2, SETVAL, 1);
  semctl(sem_id, 3, SETVAL, 0);

  /*sem wait for zero*/
  sops.sem_num = 0;
  sops.sem_flg = 0;
  sops.sem_op = rules.SO_TAXI + rules.SO_SOURCES;
  semop(sem_id, &sops, 1);

  /*sem mutex*/

  sops.sem_num = 1;
  sops.sem_flg = 0;
  sops.sem_op = 1;
  semop(sem_id, &sops, 1);

  sops.sem_num = 2;
  sops.sem_flg = 0;
  sops.sem_op = 1;
  semop(sem_id, &sops, 1);

  sops.sem_num = 3;
  sops.sem_flg = 0;
  sops.sem_op = rules.SO_TAXI;
  semop(sem_id, &sops, 1);

  shared->sim_timeout = 1;

  /*------------------------------------------------------------------------------
  creazione TAXI
  ------------------------------------------------------------------------------*/
  taxis = calloc(rules.SO_TAXI, sizeof(*sources));

  for (i = 0; i < rules.SO_TAXI; i++)
  {
    switch (child_pid = fork())
    {
    case -1:
      // gestisci errore
      fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
      break;
      // codice dei processi taxi
    case 0:
      free(taxis);

      j = 0;
      srand(getpid());
      fprintf(stderr, "CHILD PID: %d INITIALIZED AS TAXI\n", getpid());

      /*inizio sez critica*/
      sem_reserve(sem_id, 1);
      taxi cab;
      cab = taxi_gen(shared);
      /*fine sez critica*/
      sem_release(sem_id, 1);
      print_taxi(cab);

      wait_for_zero(sem_id, 0);

      /*fine inizializzazione taxi*/
      /*------------------------------------------------------------------------------
                  INIZIO SIMULAZIONE
          ------------------------------------------------------------------------------*/
      while (shared->sim_timeout)
      {
        cab = drive(shared, cab);
        printf("cerco passeggeri\n");
        print_taxi(cab);
        // destinazione raggiunta
        /*in ascolto su msgq per
           * ottenere la prosima destination*/
        if (msgrcv(q_id, &msg, SIZE, cab.position.id, 0) < 0)
        {
          if (errno == ENOMSG)
          {
            printf("coda vuota\n");
          }
          if (errno == EINTR)
          {
            //printf("caught signal\n");
            break;
          }
          if (errno == EINVAL)
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
          printf("Taxi %d legge\n", getpid());
          print_msg(msg);
          printf("-------------------------------------------\n");
          //  cab.position = msg.origin;
          cab.destination = msg.dest;

          printf("pid %d caricato passeggero verso = %d\n", getpid(),
                 cab.destination.id);
          shared->s.n_viaggi++;
          shared->s.evasi++;
          cab = drive(shared, cab);
          printf("PACCO CONSEGNATO\n");
          cab.destination = get_random_source(shared->m);
          print_taxi(cab);
          cab.stats.clienti[1] = cab.stats.clienti[1] + 1;
        }
      }
      //  printf("FINE taxi\n");

      sops.sem_num = 3;
      sops.sem_op = -1;
      semop(sem_id, &sops, 1);
      exit(0);
    default:
      taxis[i] = child_pid;
      break;

    } // fine switch taxi
  }

  /*------------------------------------------------------------------------------
  creazione Request
  ------------------------------------------------------------------------------*/
  sources = calloc(rules.SO_SOURCES, sizeof(*sources));

  for (i = 0; i < rules.SO_SOURCES; i++)
  {
    switch (child_pid = fork())
    {
    case -1:
      // gestisci errore
      fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
      break;
    case 0:
      free(sources);

      fprintf(stderr, "CHILD PID: %d INITIALIZED AS SO_SOURCE\n", getpid());
      srand(getpid());
      cell s;
      message msg;
      s = get_new_source(shared);

      wait_for_zero(sem_id, 0);

      /*------------------------------------------------------------------------------
        INIZIO SIMULAZIONE
          ------------------------------------------------------------------------------*/
      while (shared->sim_timeout)
      {
        nanosleep((const struct timespec[]){{0, 800000000L}}, NULL);
        msg = msg_gen(shared->m, s);

        if (msgsnd(q_id, &msg, SIZE, 0) < 0)
        {
          if (errno == EINTR)
          {
          }
          else if (errno == EIDRM)
          {
          }
          else
          {
            TEST_ERROR;
          }
        }
        //  printf("%d ha generato la richiesta\n", getpid());
      }

      wait_for_zero(sem_id, 3);

      exit(0);
    default:
      sources[i] = child_pid;
      break;
    }
  }

  /*------------------------------------------------------------------------------
    fine Request, di nuovo main
  ------------------------------------------------------------------------------*/

  sops.sem_num = 0;
  sops.sem_op = 0;
  semop(sem_id, &sops, 1);

  /*----------------------------------------------------------------------------*/
  alarm(rules.SO_DURATION);
  printf("START SIMULATION\n");
  // while (shared->sim_timeout == 1) {
  //   print_map(shared->m);
  //   fflush(stdout);
  //   sleep(3);
  // }

  while ((my_pid = wait(&status)) != -1)
  {
    //  printf("wexit = %d \n", WEXITSTATUS(status));
    if (WEXITSTATUS(status) == 129)
    {
      shared->s.v_abort++;
      switch (child_pid = fork())
      {
      case -1:
        // gestisci errore
        fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
        break;
        // codice dei processi taxi
      case 0:
        sops.sem_num = 3;
        sops.sem_op = 1;
        semop(sem_id, &sops, 1);
        for (j = 0; j < rules.SO_TAXI; j++)
        {
          if (taxis[j] == 0)
          {
            taxis[j] = getpid();
          }
        }
        j = 0;
        srand(getpid());
        fprintf(stderr, "CHILD PID: %d INITIALIZED AS TAXI\n", getpid());
        sem_reserve(sem_id, 2);
        taxi cab;
        cab = taxi_gen(shared);
        // print_taxi(cab);
        sem_release(sem_id, 2);
        /*fine inizializzazione taxi*/

        while (shared->sim_timeout)
        {
          printf("INIZIO WHILE NUOVI TAXI\n");

          /*in ascolto su msgq per
             * ottenere la prosima destination*/
          if (msgrcv(q_id, &msg, SIZE, cab.position.id, 0) < 0)
          {
            if (errno == ENOMSG)
            {
              printf("coda vuota\n");
            }
            if (errno == EINTR)
            {
              printf("caught signal\n");
            }
            else
            {
              TEST_ERROR;
              break;
            }
          }
          else
          {
            cab.position = msg.origin;
            cab.destination = msg.dest;

            printf("pid %d caricato passeggero verso = %d\n", getpid(),
                   cab.destination.id);
            shared->s.n_viaggi++;
            cab = drive(shared, cab);
            shared->s.evasi++;
            cab.stats.clienti[1] = cab.stats.clienti[1] + 1;
          }
        }

        sops.sem_num = 3;
        sops.sem_op = -1;
        semop(sem_id, &sops, 1);
        exit(0);
      default:
        for (j = 0; j < rules.SO_TAXI; j++)
        {
          if (taxis[j] == my_pid)
          {
            taxis[j] = child_pid;
          }
        }
        break;
      }
    }
    else
    {
      for (j = 0; j < rules.SO_TAXI; j++)
      {
        if (taxis[j] == my_pid)
        {
          taxis[j] = 0;
        }
      }
    }
  }
  sops.sem_num = 3;
  sops.sem_op = 0;
  semop(sem_id, &sops, 1);

  printf("SIMULATION FINISHED\n");

  print_stats(shared, rules);

  print_map(shared->m);
  msgctl(q_id, 0, IPC_RMID);
  shmctl(shm_id, IPC_RMID, NULL);
  semctl(sem_id, 0, IPC_RMID);
  semctl(sem_id_cell, 0, IPC_RMID);

  printf("MASTER TERMINATED \n");
  exit(EXIT_SUCCESS);
}
