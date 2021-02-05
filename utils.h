#ifndef UTILS_H
#define UTILS_H

#include "map.h"

#define TEST_ERROR                                                      \
  if (errno)                                                            \
  {                                                                     \
    dprintf(STDERR_FILENO, "%s:%d: PID=%5d: Error %d (%s)\n", __FILE__, \
            __LINE__, getpid(), errno, strerror(errno));                \
  }

/**
 * Timeval struct
 */
struct timespec my_time;

/**
 * Struct that contains the rules about the simulation
 */
typedef struct
{
  int SO_HOLES;
  int SO_TOP_CELLS;
  int SO_SOURCES;
  int SO_CAP_MIN;
  int SO_CAP_MAX;
  int SO_TAXI;
  int SO_TIMENSEC_MIN;
  int SO_TIMENSEC_MAX;
  int SO_TIMEOUT;
  int SO_DURATION;
} settings;

/**
 * Struct that contains taxi's statistics
 */
typedef struct
{ /* [0] = id, [1] = statistica*/
  int km[2];
  long tempo[2];
  int clienti[2];
} taxi_stat;

/**
 *  * Struct that contains the statistics about the simulation
 */
typedef struct
{
  int n_viaggi;
  int v_comp;
  int v_abort;
  long inevasi;
  int top_cells[SO_WIDTH * SO_HEIGHT];
  taxi_stat top_taxi;
} stats;

/**
 * Shared_data struct that contains info about simulation
 */
typedef struct
{
  map m;
  stats s;
  int sim_timeout;
  int taxi_timeout;
  int sources[SO_WIDTH * SO_HEIGHT];
  int sources_ids[SO_WIDTH * SO_HEIGHT];
  int sem_ids[SO_WIDTH * SO_HEIGHT];
} shared_data;

/**
 * function to read and set the rules of the simulation
 * @param a1: struct of settings named rules
 * @return : rules 
 */
settings cfg(settings rules);

/**
 * function to handle the different signal errors
 * @param a1: struct of settings named rules
 */
void signal_handler(int sig);

/**
 * function to print the simulation statistics about the 
 * @param a1: pointer to shared_data struct 
 * @param a2: rules of the simulation
 */
void print_stats(shared_data *shared, settings rules);

/**
 * function to print taxi that made the most travels 
 * @param a1: pointer to struct taxi_stat named best
 */
void print_top_taxis(taxi_stat *best);

/**
 * function to generate the map 
 * @param a1: struct of settings named rules
 * @param a2: semaphore struct
 * @return : the new map generated
 */
map map_gen(settings rules, struct sembuf sops);

/**
 * function to configure the cell following the rules
 * @param a1: struct of settings named rules
 * @param a2: abscissa x on the map
 * @param a3: ordinate y on the map
 * @param a4: id of the cell
 * @param a5: semaphore struct
 * @return : the new cell configurated 
 */
cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops);

/**
 * function to configure the source cells
 * @param a1: pointer to shared_data struct
 * @return : the source cell s
 */
cell get_new_source(shared_data *shared);

/**
 * function to calculate a random number r
 * @param a1: a
 * @param a2: b
 * @return : the r number
 */
int random_extraction(int a, int b);

/**
 * function to release the semaphores
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_release(int id_sem, int sem_num);

/**
 * function to try the access to the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_reserve(int id_sem, int sem_num);

/**
 * function to test the access to the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int sem_reserve_sim(int id_sem_cell, int sem_num);

/**
 * function to wait the zero of the semaphore
 * @param a1: semaphore id
 * @param a2: semaphore number
 */
int wait_for_zero(int sem_id, int sem_num);

/**
 * function to print the most crossed cells by the taxi
 * @param a1: struct of map
 * @param a2: struct of settings named rules
 */
void top_cells(map mymap, settings rules);

#endif
