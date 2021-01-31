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


/**
 * function to print the map
 * @param a1: struct of map
 */
void print_map(map my_map)
{
  int i, j;
  for (i = 0; i < SO_HEIGHT; i++)
  {
    for (j = 0; j < SO_WIDTH; j++)
    {
      printf(" ------------");
    }
    putchar('\n');
    for (j = 0; j < SO_WIDTH; j++)
    {
      if (my_map.city[i][j].id <= 9)
      {
        printf("|%d          %d", my_map.city[i][j].id, my_map.city[i][j].type);
      }
      else if (my_map.city[i][j].id <= 99)
      {
        printf("|%d         %d", my_map.city[i][j].id, my_map.city[i][j].type);
      }
      else if (my_map.city[i][j].id <= 999)
      {
        printf("|%d        %d", my_map.city[i][j].id, my_map.city[i][j].type);
      }
      else
      {
        printf("|%d        %d", my_map.city[i][j].id, my_map.city[i][j].type);
      }
    }
    printf("|\n");
    for (j = 0; j < SO_WIDTH; j++)
    {
      printf("|            ");
    }
    printf("|\n");
    for (j = 0; j < SO_WIDTH; j++)
    {
      printf("|      %d     ", my_map.city[i][j].here);
    }
    printf("|\n");
    for (j = 0; j < SO_WIDTH; j++)
    {
      printf("|           %d", my_map.city[i][j].cap);
    }
    printf("|\n");
  }
  for (j = 0; j < SO_WIDTH; j++)
  {
    printf(" ------------");
  }
  putchar('\n');
}



/**
 * function to create a hole in the map
 * @param a1: cell
 */
cell hole_gen(cell cella)
{
  cella.t_attr = 999999999;
  cella.cap = 0;
  cella.type = 0;
  return cella;
}

/**
 * function to create a source in the map
 * @param a1: cell
 */
cell source_gen(cell cella)
{
  cella.type = 2;
  cella.status = 0;
  return cella;
}

/**
 * function to check the close cells
 * @param a1:struct of map
 * @param a2:cell
 * @param a3:abscissa x
 * @param a4:ordinate y
 * @return:r
 */
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

/**
 * function to get a random cell position
 * @param a1:struct of map
 * @return: cell's coordinates
 */
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

/**
 * function to get a valid source cell position
 * @param a1:struct of map
 * @return: source cell
 */
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

/**
 * function to get a random source cell position
 * @param a1:struct of map
 * @return: source's cell position
 */
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

/**
 * function to get a hole cell position
 * @param a1:struct of map
 * @return: hole's cell position
 */
cell get_hole(map my_map)
{
  int i, x, y;
  i = 0;
  while (i < 1)
  {
    x = random_extraction(0, SO_WIDTH - 1);
    y = random_extraction(0, SO_HEIGHT - 1);
    if (my_map.city[x][y].type == 0)
    {
      i++;
    }
  }
  return my_map.city[x][y];
}
/*----------------------------------------------------------------------------*/
