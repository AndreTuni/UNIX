#ifndef UTILS_H
#define UTILS_H

 #include "map.h"
#define SO_WIDTH 6
#define SO_HEIGHT 6
#define TEST_ERROR                                                      \
  if (errno)                                                            \
  {                                                                     \
    dprintf(STDERR_FILENO, "%s:%d: PID=%5d: Error %d (%s)\n", __FILE__, \
            __LINE__, getpid(), errno, strerror(errno));                \
  }
/*structs*/

struct timespec my_time;

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


  typedef struct
  { /* [0] = id, [1] = statistica*/
    int km[2];
    int tempo[2];
    int clienti[2];
  } taxi_stat;

  typedef struct
  {
    int n_viaggi;
    int v_comp;
    int v_abort;
    int evasi;
    int top_cells[SO_WIDTH * SO_HEIGHT];
    taxi_stat top_taxi;
  } stats;



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


  /*functions*/
  settings cfg(settings rules);
  void signal_handler(int sig);
  void print_stats(shared_data *shared, settings rules);
  map map_gen(settings rules, struct sembuf sops);
  cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops);
  cell get_new_source(shared_data *shared);
  int random_extraction(int a, int b);
  int sem_release(int sem_id, int sem_num);
  int sem_reserve(int sem_id, int sem_num);
  int sem_reserve_sim(int sem_id, int sem_num);
  int wait_for_zero(int sem_id, int sem_num);






#endif
