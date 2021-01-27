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
#include "taxi.h"

/*----------------------------------------------------------------------------*/

cell set_taxi(shared_data *shared)
{
  int i = 0;
  cell pos;
  while (i < 1)
  {
    pos = get_random_cell(shared->m);
    if (shared->m.city[pos.x][pos.y].here < shared->m.city[pos.x][pos.y].cap)
    {
      shared->m.city[pos.x][pos.y].here++;
      i++;
    }
  }

  return pos;
}

/*----------------------------------------------------------------------------*/

taxi taxi_gen(shared_data *shared)
{
  taxi cab;
  cab.id = getpid();
  cab.position = set_taxi(shared);
  cab.destination = get_valid_source(shared->m);
  cab.stats.km[0] = cab.id;
  cab.stats.km[1] = 0;
  cab.stats.tempo[0] = cab.id;
  cab.stats.tempo[1] = 0;
  cab.stats.clienti[0] = cab.id;
  cab.stats.clienti[1] = 0;
  return cab;
}


/*----------------------------------------------------------------------------*/

void print_taxi(taxi t)
{
  printf("TAXI\n");
  printf("id = %d\n", t.id);
  printf("position id = %d\n", t.position.id);
  printf("destination id = %d\n", t.destination.id);
  printf("percorsi = %d celle\n", t.stats.km[1]);
  printf("trascorsi = %d nanosecondi\n", t.stats.tempo[1]);
  printf("trasportati = %d clienti\n", t.stats.clienti[1]);
}

/*----------------------------------------------------------------------------*/

int manhattan(int a, int b, int x, int y) { return abs(a - b) + abs(x - y); }

taxi drive(shared_data *shared, taxi t)
{
  int j, u, d, l, r, tmp, age = 0;
  int i = 0;

  struct sembuf sops;

  settings rules;
  rules = cfg(rules);
  while (i < 1 && shared->sim_timeout)
  {
    if (t.position.id == t.destination.id)
    {
      //   printf("PID: %d DESTINATION REACHED at cell %d\n", t.id,
      //   t.position.id);
      shared->s.v_comp++;
      r = 1;
      i++;
    }
    else
    {
      int current = manhattan(t.position.x, t.destination.x, t.position.y,
                              t.destination.y);
      int up = manhattan(t.position.x - 1, t.destination.x, t.position.y,
                         t.destination.y) -
               age;
      int down = manhattan(t.position.x + 1, t.destination.x, t.position.y,
                           t.destination.y) -
                 age;
      int left = manhattan(t.position.x, t.destination.x, t.position.y - 1,
                           t.destination.y) -
                 age;
      int right = manhattan(t.position.x, t.destination.x, t.position.y + 1,
                            t.destination.y) -
                  age;

      tmp = t.position.id - 1;

      if (shared->m.city[t.position.x - 1][t.position.y].type != 0 ||
          shared->m.city[t.position.x - 1][t.position.y].id != tmp + 1)
      {
        u = 1;
      }
      else
      {
        u = 0;
      }
      if (shared->m.city[t.position.x + 1][t.position.y].type != 0 ||
          shared->m.city[t.position.x + 1][t.position.y].id != tmp + 1)
      {
        d = 1;
      }
      else
      {
        d = 0;
      }
      if (shared->m.city[t.position.x][t.position.y - 1].type != 0 ||
          shared->m.city[t.position.x][t.position.y - 1].id != tmp + 1)
      {
        l = 1;
      }
      else
      {
        l = 0;
      }
      if (shared->m.city[t.position.x][t.position.y + 1].type != 0 ||
          shared->m.city[t.position.x][t.position.y + 1].id != tmp + 1)
      {
        r = 1;
      }
      else
      {
        r = 0;
      }

      // printf("pid %d sta guidando...\n", getpid());

      if (up >= 0 && up <= current && u)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;
        alarm(0);
        // printf("PID: %d sem acquired\n", getpid());
        t.position = go_up(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x + 1][t.position.y].here--;
      }

      //   else  {
      //     printf(" pid %d manhattan curr = %d manhattan down = %d\n",
      //     getpid(),
      //            current, up);
      //   }
      else if (down >= 0 && down <= current && d)
      {
        alarm(shared->taxi_timeout);
        age = 0;
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;

        alarm(0);

        // printf("PID: %d sem acquired\n", getpid());
        t.position = go_down(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x - 1][t.position.y].here--;
      }

      //   else {
      //     printf(" pid %d manhattan curr = %d manhattan down = %d\n",
      //     getpid(),
      //            current, down);
      //   }
      else if (left >= 0 && left <= current && l)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;

        alarm(0);
        //   printf("PID: %d sem acquired\n", getpid());
        t.position = go_left(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y - 1].here--;
      }

      //   else {
      //     printf("pid %d manhattan curr = %d manhattan right = %d\n",
      //     getpid(),
      //            current, left);
      //   }
      else if (right >= 0 && right <= current && r)
      {
        alarm(shared->taxi_timeout);
        age = 0;
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;

        alarm(0);

        //   printf("PID: %d sem acquired\n", getpid());
        // shared->m.city[t.position.x][t.position.y].here--;
        t.position = go_right(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y + 1].here--;
      }

      else
      {
        age++;
        printf("curr %d up %d\n", current, up);
        printf("curr %d down %d\n", current, down);
        printf("curr %d left %d\n", current, left);
        printf("curr %d right %d\n", current, right);
        printf("pid %d position %d destination = %d \n", getpid(),
               t.position.id, t.destination.id);
        // printf("pid %d lost\n", getpid());
      }
    }
  }
  // printf("fine  drive %d\n", getpid());
  return t;
}
