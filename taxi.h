#include "map.h"
#include "utils.h"

#ifndef TAXI_H
#define TAXI_H

/**
 * Taxi struct
 */
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

/**
 * function to position the taxi on the cell
 * @param a1: pointer to shared_data struct
 * @return : the position in which is positioned the taxi
 */
cell set_taxi(shared_data *shared);

/**
 * function to set all the information about the taxi
 * @param a1: pointer to shared_data struct
 * @return : the struct taxi named cab
 */
taxi taxi_gen(shared_data *shared);

/**
 * function to print all the taxi's statistics 
 * @param a1: taxi
 */
void print_taxi(taxi t);

/**
 * function to calculate the manhattan distance between two coordinates
 * @param a1: starter abscissa a 
 * @param a2: starter ordinate b
 * @param a3: destination abscissa x
 * @param a4: destination ordinate y
 */
int manhattan(int a, int b, int x, int y);

/**
 * function to drive the taxi on the map
 * @param a1: pointer to shared_data struct
 * @param a2: taxi
 * @return : the taxi in the final position 
 */
taxi drive(shared_data *shared, taxi t);

/**
 * function to move the taxi up on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_up(shared_data *shared, cell position, taxi t);

/**
 * function to move the taxi down on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_down(shared_data *shared, cell position, taxi t);

/**
 * function to move the taxi left on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_left(shared_data *shared, cell position, taxi t);

/**
 * function to move the taxi right on the map
 * @param a1: pointer to shared_data struct
 * @param a2: current position on the map
 * @param a3: the taxi
 * @return : next cell position of the taxi
 */
cell go_right(shared_data *shared, cell position, taxi t);

#endif
