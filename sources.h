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
  message msg_gen(map my_map, cell s);
  void print_msg(message msg);


#endif
