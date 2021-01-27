#ifndef MAP_H
#define MAP_H

#define SO_WIDTH 6
#define SO_HEIGHT 6

/*structs*/
typedef struct{
    int id;
    int x;
    int y;
    int t_attr;
    int cap;
    int type;
    int here;
    int n_attr;
    int status;
    int sem_id;
} cell;


// typedef struct
// { /* [0] = id, [1] = statistica*/
//   int km[2];
//   int tempo[2];
//   int clienti[2];
// } taxi_stat;
//
// typedef struct
// {
//   int n_viaggi;
//   int v_comp;
//   int v_abort;
//   int evasi;
//   int top_cells[SO_WIDTH * SO_HEIGHT];
//   taxi_stat top_taxi;
// } stats;


// typedef struct
// {
//   int SO_HOLES;
//   int SO_TOP_CELLS;
//   int SO_SOURCES;
//   int SO_CAP_MIN;
//   int SO_CAP_MAX;
//   int SO_TAXI;
//   int SO_TIMENSEC_MIN;
//   int SO_TIMENSEC_MAX;
//   int SO_TIMEOUT;
//   int SO_DURATION;
// } settings;


typedef struct
{
  cell city[SO_WIDTH][SO_HEIGHT];
} map;

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
// map map_gen(settings rules, struct sembuf sops);
void print_map(map my_map);
//cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops);
cell hole_gen(cell cella);
cell source_gen(cell cella);
int check_neighbours(map this_map, cell cella, int x, int y);
cell get_random_cell(map my_map);
cell get_valid_source(map my_map);
cell get_random_source(map my_map);
// cell get_new_source(shared_data *shared);


#endif
