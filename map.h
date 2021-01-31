#ifndef MAP_H
#define MAP_H

#define SO_WIDTH 3
#define SO_HEIGHT 3

/*structs*/
typedef struct
{
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

/**
 * function to print the map
 * @param a1: struct of map
 */
void print_map(map my_map);

/**
 * function to create a hole in the map
 * @param a1: cell
 */
cell hole_gen(cell cella);

/**
 * function to create a source in the map
 * @param a1: cell
 */
cell source_gen(cell cella);

/**
 * function to check the close cells
 * @param a1:struct of map
 * @param a2:cell
 * @param a3:abscissa x
 * @param a4:ordinate y
 */
int check_neighbours(map this_map, cell cella, int x, int y);

/**
 * function to get a random cell position
 * @param a1:struct of map
 * @return: cell's coordinates
 */
cell get_random_cell(map my_map);

/**
 * function to get a valid source cell position
 * @param a1:struct of map
 * @return: source cell
 */
cell get_valid_source(map my_map);

/**
 * function to get a random source cell position
 * @param a1:struct of map
 * @return: source's cell position
 */
cell get_random_source(map my_map);

/**
 * function to get a hole cell position
 * @param a1:struct of map
 * @return: source's cell position
 */
cell get_hole(map my_map);

#endif
