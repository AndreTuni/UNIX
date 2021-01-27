#ifndef MAP_H
#define MAP_H

/*structs*/
typedef struct{
    int id;
    int x;
    int y;
    int t_attr;
    int cap;
    int type;
    int here;
    int n_attraversamenti;
    int status;
    int sem_id;
} cell;

typedef struct
{
  cell city[SO_WIDTH][SO_HEIGHT];
} map;

/*functions*/
map map_gen(settings rules, struct sembuf sops);
void print_map(shared_data *shared);
cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops);
cell hole_gen(cell cella);
cell source_gen(cell cella);
int check_neighbours(map this_map, cell cella, int x, int y);
cell get_random_cell(map my_map);
cell get_valid_source(map my_map);
cell get_random_source(map my_map);
cell get_new_source(shared_data *shared);


#endif
