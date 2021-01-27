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

/*----------------------------------------------------------------------------*/

map map_gen(settings rules, struct sembuf sops)
{
  int i, j, x, y, id, c, k;
  map new_map;
  int max_holes = SO_WIDTH * SO_HEIGHT / 9;
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

/*----------------------------------------------------------------------------*/

void print_map(map my_map)
{
  int i, j;
  printf("******************************************************\n");
  printf("|  ID  |  X  |  Y  |   DURATA      | ");
  printf("CAP | TIPO | TAXI \n");
  for (i = 0; i < SO_WIDTH; i++)
  {
    for (j = 0; j < SO_HEIGHT; j++)
    {
      printf("|  %d   |  %d  |  %d  |  %d    | %d   |  %d   | %d\n",
             my_map.city[i][j].id, my_map.city[i][j].x, my_map.city[i][j].y,
             my_map.city[i][j].durata_attraversamento, my_map.city[i][j].cap,
             my_map.city[i][j].type, my_map.city[i][j].here);
    }
  }
}

/*----------------------------------------------------------------------------*/

cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops)
{
  cell new_cell;
  new_cell.id = id;
  new_cell.x = x;
  new_cell.y = y;
  new_cell.durata_attraversamento =
      random_extraction(rules.SO_TIMENSEC_MIN, rules.SO_TIMENSEC_MAX);
  new_cell.cap = random_extraction(rules.SO_CAP_MIN, rules.SO_CAP_MAX);
  new_cell.type = 1; // definito dopo
  new_cell.here = 0;
  new_cell.n_attraversamenti = 0;
  semctl(sem_id_cell, id - 1, SETVAL, new_cell.cap);

  new_cell.sem_id = sem_id_cell;
  return new_cell;
}

/*----------------------------------------------------------------------------*/

cell hole_gen(cell cella)
{
  cella.durata_attraversamento = 999999999;
  cella.cap = 0;
  cella.type = 0;
  return cella;
}

cell source_gen(cell cella)
{
  cella.type = 2;
  cella.status = 0;
  return cella;
}

/*----------------------------------------------------------------------------*/

int check_neighbours(map this_map, cell cella, int x, int y)
{
  int r = -1;
  if (this_map.city[x - 1][y - 1].type != 0 &&
      this_map.city[x - 1][y].type != 0 &&
      this_map.city[x - 1][y + 1].type != 0 &&
      this_map.city[x][y - 1].type != 0 && this_map.city[x][y + 1].type != 0 &&
      this_map.city[x + 1][y - 1].type != 0 &&
      this_map.city[x + 1][y].type != 0 &&
      this_map.city[x + 1][y + 1].type != 0)
  {
    r = 1;
  }
  return r;
}

/*----------------------------------------------------------------------------*/

cell get_random_cell(map my_map)
{
  int i, x, y;
  i = 0;
  while (i < 1)
  {
    x = random_extraction(0, SO_WIDTH - 1);
    y = random_extraction(0, SO_HEIGHT - 1);
    if (my_map.city[x][y].type != 0)
    {
      i++;
    }
  }
  return my_map.city[x][y];
}

/*----------------------------------------------------------------------------*/

cell get_valid_source(map my_map)
{
  cell s;
  int i = 0;
  while (i < 1)
  {
    s = get_random_source(my_map);
    if (my_map.city[s.x][s.y].here < my_map.city[s.x][s.y].cap)
    {
      i++;
    }
  }
  return s;
}

/*----------------------------------------------------------------------------*/

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
/*----------------------------------------------------------------------------*/
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
