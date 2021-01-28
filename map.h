#ifndef MAP_H
#define MAP_H

#define SO_WIDTH 3
#define SO_HEIGHT 3

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




typedef struct
{
  cell city[SO_WIDTH][SO_HEIGHT];
} map;



/*functions*/
void print_map(map my_map);
cell hole_gen(cell cella);
cell source_gen(cell cella);
int check_neighbours(map this_map, cell cella, int x, int y);
cell get_random_cell(map my_map);
cell get_valid_source(map my_map);
cell get_random_source(map my_map);


#endif
