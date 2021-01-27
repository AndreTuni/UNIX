#ifndef SOURCES_H
#define SOURCES_H

/*structs*/


typedef struct
{
  long mtype;
  cell origin;
  cell dest;
} message;

  /*functions*/
  settings cfg(settings rules);
  void signal_handler(int sig);
  void print_stats(shared_data *shared, settings rules);


#endif
