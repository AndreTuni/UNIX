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

int sem_id, sem_id_cell, q_id, q_id_t;

/**
 * function to position the taxi on the cell
 * @param a1: pointer to shared_data struct
 * @return : the position in which is positioned the taxi
 */
cell set_taxi(shared_data *shared)
{
  int i = 0;
  cell pos;
  struct sembuf sops;
  while (i < 1)
  {
    pos = get_random_cell(shared->m);
    sops.sem_flg = IPC_NOWAIT;
    sops.sem_num = pos.id - 1;
    sops.sem_op = -1;
    if (semop(sem_id_cell, &sops, 1) < 0)
    {
      if (errno == EAGAIN)
      {
      }
      else
      {
        TEST_ERROR;
        break;
      }
    }
    else
    {
      sem_reserve(sem_id, 1);
      shared->m.city[pos.x][pos.y].here++;
      i++;
      sem_release(sem_id, 1);
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
  cab.status = 0;
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
  shared->m.city[t.position.x][t.position.y].here--;
  next_pos = shared->m.city[position.x - 1][position.y];
  shared->m.city[position.x][position.y].n_attr++;
  shared->m.city[next_pos.x][next_pos.y].here++;
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
  shared->m.city[t.position.x][t.position.y].here--;
  next_pos = shared->m.city[position.x + 1][position.y];
  shared->m.city[position.x][position.y].n_attr++;
  shared->m.city[next_pos.x][next_pos.y].here++;
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
  shared->m.city[t.position.x][t.position.y].here--;
  next_pos = shared->m.city[position.x][position.y - 1];
  shared->m.city[position.x][position.y].n_attr++;
  shared->m.city[next_pos.x][next_pos.y].here++;
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
  shared->m.city[t.position.x][t.position.y].here--;
  next_pos = shared->m.city[position.x][position.y + 1];
  shared->m.city[position.x][position.y].n_attr++;
  shared->m.city[next_pos.x][next_pos.y].here++;
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
    if (t.position.id == t.destination.id)
    {
      shared->s.v_comp++;
      t.previous = get_hole(shared->m);
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
        u = 0;
      }
      if (shared->m.city[t.position.x + 1][t.position.y].type != 0 &&
          shared->m.city[t.position.x + 1][t.position.y].id != t.previous.id)
      {
        d = 1;
      }
      else
      {
        d = 0;
      }
      if (shared->m.city[t.position.x][t.position.y - 1].type != 0 &&
          shared->m.city[t.position.x][t.position.y - 1].id != t.previous.id)
      {
        l = 1;
      }
      else
      {
        l = 0;
      }
      if (shared->m.city[t.position.x][t.position.y + 1].type != 0 &&
          shared->m.city[t.position.x][t.position.y + 1].id != t.previous.id)
      {
        r = 1;
      }
      else
      {
        r = 0;
      }
      if (up >= 0 && up <= current && u)
      {
        age = 0;
        sem_reserve(sem_id, 1);
        alarm(shared->taxi_timeout);
        sem_release(sem_id_cell, tmp);
        sem_reserve_sim(sem_id_cell, t.position.id - 1);
        sem_release(sem_id, 1);
        alarm(0);
        t.previous = t.position;
        t.position = go_up(shared, t.position, t);
        t.stats.km[1] += 1;
        if (t.status == 1)
        {
          t.stats.tempo[1] += shared->m.city[t.position.x][t.position.y].t_attr;
        }
      }
      else if (down >= 0 && down <= current && d)
      {
        age = 0;
        sem_reserve(sem_id, 1);
        alarm(shared->taxi_timeout);
        sem_release(sem_id_cell, tmp);
        sem_reserve_sim(sem_id_cell, t.position.id - 1);
        sem_release(sem_id, 1);
        alarm(0);
        t.previous = t.position;
        t.position = go_down(shared, t.position, t);
        t.stats.km[1] += 1;
        if (t.status == 1)
        {
          t.stats.tempo[1] += shared->m.city[t.position.x][t.position.y].t_attr;
        }
      }
      else if (left >= 0 && left <= current && l)
      {
        age = 0;
        sem_reserve(sem_id, 1);
        alarm(shared->taxi_timeout);
        sem_release(sem_id_cell, tmp);
        sem_reserve_sim(sem_id_cell, t.position.id - 1);
        sem_release(sem_id, 1);
        alarm(0);
        t.previous = t.position;
        t.position = go_left(shared, t.position, t);
        t.stats.km[1] += 1;
        if (t.status == 1)
        {
          t.stats.tempo[1] += shared->m.city[t.position.x][t.position.y].t_attr;
        }
      }
      else if (right >= 0 && right <= current && r)
      {
        age = 0;
        sem_reserve(sem_id, 1);
        alarm(shared->taxi_timeout);
        sem_release(sem_id_cell, tmp);
        sem_reserve_sim(sem_id_cell, t.position.id - 1);
        sem_release(sem_id, 1);
        alarm(0);
        t.previous = t.position;
        t.position = go_right(shared, t.position, t);
        t.stats.km[1] += 1;
        if (t.status == 1)
        {
          t.stats.tempo[1] += shared->m.city[t.position.x][t.position.y].t_attr;
        }
      }
      else
      {
        age++;
      }
    }
  }
  return t;
}

/**
 * function that create the message to send in message queue
 * @param a1: struct of taxi
 * @return : feed
 */
feedback feed_gen(taxi t)
{
  feedback feed;
  feed.mtype = getpid();
  feed.new_stats.km[0] = t.stats.km[0];
  feed.new_stats.km[1] = t.stats.km[1];
  feed.new_stats.tempo[0] = t.stats.tempo[0];
  feed.new_stats.tempo[1] = t.stats.tempo[1];
  feed.new_stats.clienti[0] = t.stats.clienti[0];
  feed.new_stats.clienti[1] = t.stats.clienti[1];
  return feed;
}

/**
 * function to in initialize the taxi before the simulation starts
 * @param a1: pointer to shared_data memory 
 * @return : cab
 */
taxi init_taxi(shared_data *shared)
{
  fprintf(stderr, "CHILD PID: %d INITIALIZED AS TAXI\n", getpid());
  struct sembuf sops;
  struct msqid_ds msqid_ds, *buf;
  message msg;
  buf = &msqid_ds;
  srand(getpid());
  taxi cab;
  cab = taxi_gen(shared);
  return cab;
}

/**
 * function to execute the simulation of taxi on the map
 * @param a1: pointer to shared_data memory 
 * @param a2: struct of taxi named cab
 */
void taxi_simulation(shared_data *shared, taxi cab)
{
  struct sembuf sops;
  struct msqid_ds msqid_ds, *buf;
  message msg;
  buf = &msqid_ds;
  srand(getpid());
  while (shared->sim_timeout)
  {
    cab = drive(shared, cab);
    if (msgrcv(q_id, &msg, WHISTLE, cab.position.id, 0) < 0)
    {
      if (errno == ENOMSG)
      {
      }
      if (errno == EINTR)
      {
        break;
      }
      if (errno == EINVAL)
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
      cab.destination = msg.dest;
      cab.status = 1;
      shared->s.n_viaggi++;
      cab = drive(shared, cab);
      if (cab.destination.type != 2)
      {
        cab.destination = get_random_source(shared->m);
        cab.status = 0;
      }
      cab.stats.clienti[1] += 1;
      feedback f;
      f = feed_gen(cab);
      if (msgsnd(q_id_t, &f, FEED, 0) < 0)
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
          TEST_ERROR;
        }
      }
      else
      {
        cab.stats.tempo[1] = 0;
      }
    }
  }
}