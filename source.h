#ifndef SOURCE_H
#define SOURCE_H


/*structs*/

typedef struct
{
  long mtype;
  int created_by;
  cell origin;
  cell dest;
} message;



  /*functions*/
  message msg_gen(map my_map, cell s);
  void print_msg(message msg);
  cell get_dest(map my_map, cell pos);


#endif
