#include "map.h"
#include "utils.h"

#ifndef TAXI_H
#define TAXI_H

/*structs*/

typedef struct
{
  int id;
  cell previous;
  cell position;
  cell destination;
  int queue_id;
  int time_out;
  taxi_stat stats;
} taxi;

/*functions*/
cell set_taxi(shared_data *shared);
taxi taxi_gen(shared_data *shared);
void print_taxi(taxi t);
int manhattan(int a, int b, int x, int y);
taxi drive(shared_data *shared, taxi t);
cell go_up(shared_data *shared, cell position, taxi t);
cell go_down(shared_data *shared, cell position, taxi t);
cell go_left(shared_data *shared, cell position, taxi t);
cell go_right(shared_data *shared, cell position, taxi t);

#endif
