#include "map.h"
#include "utils.h"

#ifndef TAXI_H
#define TAXI_H

/*structs*/


// typedef struct
// { /* [0] = id, [1] = statistica*/
//   int km[2];
//   int tempo[2];
//   int clienti[2];
// } taxi_stat;

typedef struct
{
  int id;
  cell position;
  cell destination;
  int queue_id;
  int time_out;
  taxi_stat stats;
} taxi;



// typedef struct{
//     int id;
//     int x;
//     int y;
//     int t_attr;
//     int cap;
//     int type;
//     int here;
//     int n_attr;
//     int status;
//     int sem_id;
// } cell;

// typedef struct
// {
//   map m;
//   stats s;
//   int sim_timeout;
//   int taxi_timeout;
//   int sources[SO_WIDTH * SO_HEIGHT];
//   int sources_ids[SO_WIDTH * SO_HEIGHT];
//   int sem_ids[SO_WIDTH * SO_HEIGHT];
// } shared_data;


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
