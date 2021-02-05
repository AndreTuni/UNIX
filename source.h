#include "utils.h"
#ifndef SOURCE_H
#define SOURCE_H
#define WHISTLE sizeof(msg) - sizeof(long)

/**
 * Struct for messages
 */
typedef struct
{
  long mtype;
  int created_by;
  cell origin;
  cell dest;
} message;

/**
 * function to create a message
 * @param a1: struct of map
 * @param a2: cell s
 * @return : rules the message
 */
message msg_gen(map my_map, cell s);

/**
 * function to print the messages created
 * @param a1: message msg
 */
void print_msg(message msg);

/**
 * function to get a random cell destination
 * @param a1: struct of map
 * @param a2: position of a cell
 */
cell get_dest(map my_map, cell pos);

/**
 * function to execute the simulation of the sources
 * @param a1: pointer to shared_data memory 
 */
void source_simulation(shared_data *shared);

/**
 * function to get a random source cell position
 * @param a1:struct of map
 * @return: source's cell position
 */
cell get_random_source(map my_map);

#endif
