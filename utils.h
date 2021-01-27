#ifndef UTILS_H
#define UTILS_H

/*structs*/


typedef struct{
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

  /*functions*/
  settings cfg(settings rules);
  void signal_handler(int sig);
  void print_stats(shared_data *shared, settings rules);


#endif
