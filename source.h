#ifndef SOURCE_H
#define SOURCE_H

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

#endif
