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

int sem_id, sem_id_cell;

/**
 * function to position the taxi on the cell
 * @param a1: pointer to shared_data struct
 * @return : the position in which is positioned the taxi
 */
cell set_taxi(shared_data *shared)
{
  int i = 0;
  cell pos;
  while (i < 1)
  {
    pos = get_random_cell(shared->m);
    if (shared->m.city[pos.x][pos.y].here < shared->m.city[pos.x][pos.y].cap)
    {
      sem_reserve(sem_id, 4);
      shared->m.city[pos.x][pos.y].here++;
      sem_release(sem_id, 4);
      i++;
    }
  }
  return pos;
}

/**
 * function to set all the information about the taxi
 * @param a1: pointer to shared_data struct
 * @return : the struct taxi named cab
 */
taxi taxi_gen(shared_data *shared)
{
  taxi cab;
  cab.id = getpid();
  cab.previous = get_hole(shared->m);
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

/**
 * function to print all the taxi's statistics 
 * @param a1: taxi
 */
void print_taxi(taxi t)
{
  printf("TAXI\n");
  printf("id = %d\n", t.id);
  printf("previous id = %d\n", t.previous.id);
  printf("position id = %d\n", t.position.id);
  printf("destination id = %d\n", t.destination.id);
  //  printf("percorsi = %d celle\n", t.stats.km[1]);
  // printf("trascorsi = %d nanosecondi\n", t.stats.tempo[1]);
  // printf("trasportati = %d clienti\n", t.stats.clienti[1]);
}

/**
 * function to calculate the manhattan distance between two coordinates
 * @param a1: starter abscissa a 
 * @param a2: starter ordinate b
 * @param a3: destination abscissa x
 * @param a4: destination ordinate y
 */
int manhattan(int a, int b, int x, int y) { return abs(a - b) + abs(x - y); }

/**
 * function to move the taxi up on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_up(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  printf(" %d Going up\n", getpid());
  sem_reserve(sem_id, 4);
  shared->m.city[t.position.x][t.position.y].here--;
  printf(" %d here--\n", shared->m.city[t.position.x][t.position.y].id);
  sem_release(sem_id, 4);
  next_pos = shared->m.city[position.x - 1][position.y];
  sem_reserve(sem_id, 4);
  shared->m.city[next_pos.x][next_pos.y].here++;
  printf(" %d here++\n", shared->m.city[next_pos.x][next_pos.y].id);
  sem_release(sem_id, 4);
  //shared->m.city[position.x - 1][position.y].n_attr++;
  // t.stats.km[1]++;
  // t.stats.tempo[1] = t.stats.tempo[1] + next_pos.t_attr;
  my_time.tv_sec = 0;
  my_time.tv_nsec = next_pos.t_attr;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/**
 * function to move the taxi down on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_down(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  printf(" %d Going down\n", getpid());
  sem_reserve(sem_id, 4);
  shared->m.city[t.position.x][t.position.y].here--;
  printf(" %d here--\n", shared->m.city[t.position.x][t.position.y].id);
  sem_release(sem_id, 4);
  next_pos = shared->m.city[position.x + 1][position.y];
  sem_reserve(sem_id, 4);
  shared->m.city[next_pos.x][next_pos.y].here++;
  printf(" %d here++\n", shared->m.city[next_pos.x][next_pos.y].id);
  sem_release(sem_id, 4);
  //shared->m.city[position.x + 1][position.y].n_attr++;
  // t.stats.km[1]++;
  // t.stats.tempo[1] = t.stats.tempo[1] + next_pos.t_attr;
  my_time.tv_sec = 0;
  my_time.tv_nsec = next_pos.t_attr;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/**
 * function to move the taxi left on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_left(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  printf(" %d Going left\n", getpid());
  sem_reserve(sem_id, 4);
  shared->m.city[t.position.x][t.position.y].here--;
  printf(" %d here--\n", shared->m.city[t.position.x][t.position.y].id);
  sem_release(sem_id, 4);
  next_pos = shared->m.city[position.x][position.y - 1];
  sem_reserve(sem_id, 4);
  shared->m.city[next_pos.x][next_pos.y].here++;
  printf(" %d here++\n", shared->m.city[next_pos.x][next_pos.y].id);
  sem_release(sem_id, 4);
  // shared->m.city[position.x][position.y - 1].n_attr++;
  // t.stats.km[1]++;
  // t.stats.tempo[1] = t.stats.tempo[1] + next_pos.t_attr;
  my_time.tv_sec = 0;
  my_time.tv_nsec = next_pos.t_attr;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/**
 * function to move the taxi right on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_right(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  printf(" %d Going right\n", getpid());
  sem_reserve(sem_id, 4);
  shared->m.city[t.position.x][t.position.y].here--;
  printf(" %d here--\n", shared->m.city[t.position.x][t.position.y].id);
  sem_release(sem_id, 4);
  next_pos = shared->m.city[position.x][position.y + 1];
  sem_reserve(sem_id, 4);
  shared->m.city[next_pos.x][next_pos.y].here++;
  printf(" %d here++\n", shared->m.city[next_pos.x][next_pos.y].id);
  sem_release(sem_id, 4);
  //shared->m.city[position.x][position.y + 1].n_attr++;
  //t.stats.km[1]++;
  //t.stats.tempo[1] = t.stats.tempo[1] + next_pos.t_attr;
  my_time.tv_sec = 0;
  my_time.tv_nsec = next_pos.t_attr;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/**
 * function to drive the taxi on the map
 * @param a1: pointer to shared_data struct
 * @param a2: taxi
 * @return : the taxi in the final position 
 */
taxi drive(shared_data *shared, taxi t)
{
  int j, u, d, l, r, tmp, age = 0;
  int i = 0;

  struct sembuf sops;

  settings rules;
  rules = cfg(rules);
  while (shared->sim_timeout)
  {
    printf("taxi driving\n");
    // print_taxi(t);
    if (t.position.id == t.destination.id)
    {
      printf("PID: %d DESTINATION REACHED at cell %d\n", t.id,
             t.position.id);

      shared->s.v_comp++;

      t.previous = get_hole(shared->m);
      // r = 1;
      break;
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

      if (shared->m.city[t.position.x - 1][t.position.y].type != 0 &&
          shared->m.city[t.position.x - 1][t.position.y].id != t.previous.id)
      {
        u = 1;
      }
      else
      {
        printf("Stuck u\n");
        u = 0;
      }
      if (shared->m.city[t.position.x + 1][t.position.y].type != 0 &&
          shared->m.city[t.position.x + 1][t.position.y].id != t.previous.id)
      {
        printf("down cell id %d type %d  previous id %d\n", shared->m.city[t.position.x + 1][t.position.y].id, shared->m.city[t.position.x][t.position.y - 1].type, t.previous.id);

        d = 1;
      }
      else
      {
        printf("Stuck d\n");
        d = 0;
      }
      if (shared->m.city[t.position.x][t.position.y - 1].type != 0 &&
          shared->m.city[t.position.x][t.position.y - 1].id != t.previous.id)
      {
        printf("left cell id %d type %d  previous id %d\n", shared->m.city[t.position.x][t.position.y - 1].id, shared->m.city[t.position.x][t.position.y - 1].type, t.previous.id);
        l = 1;
      }
      else
      {
        printf("Stuck l\n");
        l = 0;
      }
      if (shared->m.city[t.position.x][t.position.y + 1].type != 0 &&
          shared->m.city[t.position.x][t.position.y + 1].id != t.previous.id)
      {
        printf("right cell id %d type %d  previous id %d\n", shared->m.city[t.position.x][t.position.y + 1].id, shared->m.city[t.position.x][t.position.y - 1].type, t.previous.id);

        r = 1;
      }
      else
      {
        printf("Stuck r\n");
        r = 0;
      }

      // printf("pid %d sta guidando...\n", getpid());

      if (up >= 0 && up <= current && u)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        alarm(0);

        t.previous = t.position;
        t.position = go_up(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
      }

      else if (down >= 0 && down <= current && d)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        alarm(0);

        t.previous = t.position;
        t.position = go_down(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
      }

      else if (left >= 0 && left <= current && l)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        alarm(0);

        t.previous = t.position;
        t.position = go_left(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        ;
      }

      else if (right >= 0 && right <= current && r)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        alarm(0);

        t.previous = t.position;
        t.position = go_right(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
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
