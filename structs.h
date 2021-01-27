#ifndef STRUCTS_H
#define STRUCTS_H

/*structs*/
typedef struct
{
  int id; /*identificativo della cella utile per le code e le statistiche
             finali, forse bastano le coordinate*/
  int x;  /*coordinate per identificare una cella nella mappa*/
  int y;
  int durata_attraversamento; /* tempi di attraversamento*/
  int cap;                    /*capacità della cella*/
  int type;                   /* -1 = hole, 1 = attraversabile, 2 = source*/
  int here;
  int n_attraversamenti;
  int status; /* 0 = non ancora asociata, 1 associata ad un pid */
  int sem_id;
} cell;

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
  cell city[SO_WIDTH][SO_HEIGHT];
} map;

typedef struct
{
  int id;
  cell position;
  cell destination;
  int queue_id;
  int time_out;
  taxi_stat stats;
} taxi;

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
{
  long mtype;
  int created_by;
  cell origin;
  cell dest;
} message;

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

#endif
