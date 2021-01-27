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
             my_map.city[i][j].t_attr, my_map.city[i][j].cap,
             my_map.city[i][j].type, my_map.city[i][j].here);
    }
  }
}

/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/

cell hole_gen(cell cella)
{
  cella.t_attr = 999999999;
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
