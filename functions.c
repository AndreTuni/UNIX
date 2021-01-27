#ifndef FUNCTIONS_H
#define FUNCTIONS_H
map map_gen(settings rules, struct sembuf sops);
void print_map(map my_map);
settings cfg(settings rules);
cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops);
cell hole_gen(cell cella);
cell source_gen(cell cella);
int check_neighbours(map this_map, cell cella, int x, int y);
cell get_random_cell(map my_map);
cell get_valid_source(map my_map);
cell get_random_source(map my_map);
cell set_taxi(shared_data *shared);
taxi taxi_gen(shared_data *shared);
void print_taxi(taxi t);
message msg_gen(map my_map, cell s);
void print_msg(message msg);
cell get_new_source(shared_data *shared);
void signal_handler(int sig);
void print_stats(shared_data *shared, settings rules);
int sem_release(int sem_id, int sem_num);
int sem_reserve(int sem_id, int sem_num);
int sem_reserve_sim(int sem_id, int sem_num);
taxi drive(shared_data *shared, taxi t);
int wait_for_zero(int sem_id, int sem_num);
int manhattan(int a, int b, int x, int y);

#endif
