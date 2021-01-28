#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SO_WIDTH 20
#define SO_HEIGHT 10
#define MAX_TAXI 100
#define SIZE sizeof(msg) - sizeof(long)
#define TEST_ERROR                                                      \
  if (errno)                                                            \
  {                                                                     \
    dprintf(STDERR_FILENO, "%s:%d: PID=%5d: Error %d (%s)\n", __FILE__, \
            __LINE__, getpid(), errno, strerror(errno));                \
  }

/*----------------------------------------------------------------------------*/
struct timespec my_time;

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

/*----------------------------------------------------------------------------*/
int parent_pid, q_id, shm_id, sem_id, sem_id_cell, child_pid, flag, segnale;
int *taxis, *sources;

/*----------------------------------------------------------------------------*/

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
/*----------------------------------------------------------------------------*/

int main()
{
  int i, j, status;
  sigset_t my_mask;
  struct sembuf sops;
  struct sigaction sa;
  unsigned int my_pid;
  sem_id_cell = semget(IPC_PRIVATE, SO_WIDTH * SO_HEIGHT, 0600);
  parent_pid = getpid();

  srand(getpid());
  settings rules;
  message msg;
  rules = cfg(rules);
  shared_data *shared;
  shm_id = shmget(IPC_PRIVATE, sizeof(*shared), 0600);
  sem_id = semget(IPC_PRIVATE, 4, 0600);
  q_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
  shared = shmat(shm_id, NULL, 0);
  shared->m = map_gen(rules, sops);
  shared->taxi_timeout = rules.SO_TIMEOUT;

  sa.sa_handler = signal_handler;
  sa.sa_flags = 0; // No special behaviour
  // Create an empty mask
  sigemptyset(&my_mask); // Do not mask any signal
  sa.sa_mask = my_mask;
  // insatallazzione signal handler

  if (sigaction(SIGINT, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));
  if (sigaction(SIGALRM, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));
  if (sigaction(SIGUSR1, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));
  if (sigaction(SIGUSR2, &sa, NULL) == -1)
    fprintf(stderr, "Cannot set a user-defined handler for Signal #%d: %s\n", i,
            strsignal(i));

  /*creazione semafori*/
  semctl(sem_id, 0, SETVAL, 0);
  semctl(sem_id, 1, SETVAL, 1);
  semctl(sem_id, 2, SETVAL, 1);
  semctl(sem_id, 3, SETVAL, 0);

  /*sem wait for zero*/
  sops.sem_num = 0;
  sops.sem_flg = 0;
  sops.sem_op = rules.SO_TAXI + rules.SO_SOURCES;
  semop(sem_id, &sops, 1);

  /*sem mutex*/

  sops.sem_num = 1;
  sops.sem_flg = 0;
  sops.sem_op = 1;
  semop(sem_id, &sops, 1);

  sops.sem_num = 2;
  sops.sem_flg = 0;
  sops.sem_op = 1;
  semop(sem_id, &sops, 1);

  sops.sem_num = 3;
  sops.sem_flg = 0;
  sops.sem_op = rules.SO_TAXI;
  semop(sem_id, &sops, 1);

  shared->sim_timeout = 1;

  /*------------------------------------------------------------------------------
  creazione TAXI
  ------------------------------------------------------------------------------*/
  taxis = calloc(rules.SO_TAXI, sizeof(*sources));

  for (i = 0; i < rules.SO_TAXI; i++)
  {
    switch (child_pid = fork())
    {
    case -1:
      // gestisci errore
      fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
      break;
      // codice dei processi taxi
    case 0:
      free(taxis);

      j = 0;
      srand(getpid());
      fprintf(stderr, "CHILD PID: %d INITIALIZED AS TAXI\n", getpid());

      /*inizio sez critica*/
      sem_reserve(sem_id, 1);
      taxi cab;
      cab = taxi_gen(shared);
      /*fine sez critica*/
      sem_release(sem_id, 1);

      wait_for_zero(sem_id, 0);

      /*fine inizializzazione taxi*/
      /*------------------------------------------------------------------------------
                  INIZIO SIMULAZIONE
          ------------------------------------------------------------------------------*/
      while (shared->sim_timeout)
      {
        // destinazione raggiunta
        /*in ascolto su msgq per
           * ottenere la prosima destination*/
        if (msgrcv(q_id, &msg, SIZE, 0, 0) < 0)
        {
          if (errno == ENOMSG)
          {
            printf("coda vuota\n");
          }
          if (errno == EINTR)
          {
            printf("caught signal\n");
          }
          else
          {
            TEST_ERROR;
            break;
          }
        }
        else
        {
          cab.destination = msg.origin;
          drive(shared, cab);
          cab.destination = msg.dest;

          // printf("pid %d caricato passeggero verso = %d\n", getpid(),
          //        cab.destination.id);
          shared->s.n_viaggi++;
          shared->s.evasi++;
          drive(shared, cab);

          cab.stats.clienti[1] = cab.stats.clienti[1] + 1;
        }
      }
      //  printf("FINE taxi\n");

      sops.sem_num = 3;
      sops.sem_op = -1;
      semop(sem_id, &sops, 1);
      exit(0);
    default:
      taxis[i] = child_pid;
      break;

    } // fine switch taxi
  }

  /*------------------------------------------------------------------------------
  creazione Request
  ------------------------------------------------------------------------------*/
  sources = calloc(rules.SO_SOURCES, sizeof(*sources));

  for (i = 0; i < rules.SO_SOURCES; i++)
  {
    switch (child_pid = fork())
    {
    case -1:
      // gestisci errore
      fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
      break;
    case 0:
      free(sources);

      fprintf(stderr, "CHILD PID: %d INITIALIZED AS SO_SOURCE\n", getpid());
      srand(getpid());
      cell s;
      message msg;
      s = get_new_source(shared);

      wait_for_zero(sem_id, 0);

      /*------------------------------------------------------------------------------
        INIZIO SIMULAZIONE
          ------------------------------------------------------------------------------*/
      while (shared->sim_timeout)
      {
        msg = msg_gen(shared->m, s);

        if (msgsnd(q_id, &msg, SIZE, 0) < 0)
        {
          if (errno == EINTR)
          {
          }
          else if (errno == EIDRM)
          {
          }
          else
          {
            TEST_ERROR;
          }
        }
      }
      //  printf("FINE REQUEST\n");

      wait_for_zero(sem_id, 3);

      exit(0);
    default:
      sources[i] = child_pid;
      break;
    }
  }

  /*------------------------------------------------------------------------------
    fine Request, di nuovo main
  ------------------------------------------------------------------------------*/

  sops.sem_num = 0;
  sops.sem_op = 0;
  semop(sem_id, &sops, 1);

  /*----------------------------------------------------------------------------*/
  alarm(rules.SO_DURATION);
  printf("START SIMULATION\n");
  // while (shared->sim_timeout == 1) {
  //   print_map(shared->m);
  //   fflush(stdout);
  //   sleep(3);
  // }

  while ((my_pid = wait(&status)) != -1)
  {
    //  printf("wexit = %d \n", WEXITSTATUS(status));
    if (WEXITSTATUS(status) == 129)
    {
      shared->s.v_abort++;
      switch (child_pid = fork())
      {
      case -1:
        // gestisci errore
        fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
        break;
        // codice dei processi taxi
      case 0:
        sops.sem_num = 3;
        sops.sem_op = 1;
        semop(sem_id, &sops, 1);
        for (j = 0; j < rules.SO_TAXI; j++)
        {
          if (taxis[j] == 0)
          {
            taxis[j] = getpid();
          }
        }
        j = 0;
        srand(getpid());
        fprintf(stderr, "CHILD PID: %d INITIALIZED AS TAXI\n", getpid());
        sem_reserve(sem_id, 2);
        taxi cab;
        cab = taxi_gen(shared);
        // print_taxi(cab);
        sem_release(sem_id, 2);
        /*fine inizializzazione taxi*/
        /*------------------------------------------------------------------------------
                    INIZIO SIMULAZIONE
            ------------------------------------------------------------------------------*/
        while (shared->sim_timeout)
        {
          printf("INIZIO WHILE NUOVI TAXI\n");

          /*in ascolto su msgq per
             * ottenere la prosima destination*/
          if (msgrcv(q_id, &msg, SIZE, 0, 0) < 0)
          {
            if (errno == ENOMSG)
            {
              printf("coda vuota\n");
            }
            if (errno == EINTR)
            {
              printf("caught signal\n");
            }
            else
            {
              TEST_ERROR;
              break;
            }
          }
          else
          {
            cab.destination = msg.origin;
            drive(shared, cab);
            cab.destination = msg.dest;

            //  printf("pid %d caricato passeggero verso = %d\n", getpid(),
            //       cab.destination.id);
            shared->s.n_viaggi++;
            drive(shared, cab);
            shared->s.evasi++;
            cab.stats.clienti[1] = cab.stats.clienti[1] + 1;
          }
        }

        sops.sem_num = 3;
        sops.sem_op = -1;
        semop(sem_id, &sops, 1);
        exit(0);
      default:
        for (j = 0; j < rules.SO_TAXI; j++)
        {
          if (taxis[j] == my_pid)
          {
            taxis[j] = child_pid;
          }
        }
        break;
      }
    }
    else
    {
      for (j = 0; j < rules.SO_TAXI; j++)
      {
        if (taxis[j] == my_pid)
        {
          taxis[j] = 0;
        }
      }
    }
  }
  sops.sem_num = 3;
  sops.sem_op = 0;
  semop(sem_id, &sops, 1);

  printf("SIMULATION FINISHED\n");

  print_stats(shared, rules);

  print_map(shared->m);
  msgctl(q_id, 0, IPC_RMID);
  shmctl(shm_id, IPC_RMID, NULL);
  semctl(sem_id, 0, IPC_RMID);
  semctl(sem_id_cell, 0, IPC_RMID);

  printf("MASTER TERMINATED \n");
  exit(EXIT_SUCCESS);
}

/*----------------------------------------------------------------------------*/

void signal_handler(int sig)
{
  int i, j;
  settings rules;
  rules = cfg(rules);
  shared_data *shared;
  shared = shmat(shm_id, NULL, 0);
  switch (sig)
  {
  case SIGALRM:
    if (getpid() == parent_pid)
    {
      shared->sim_timeout = 0;

      for (i = 0; i < rules.SO_TAXI; i++)
      {
        if (kill(taxis[i], SIGUSR1) < 0)
        {
          if (errno == EINVAL)
          {
          }
          else
          {
            TEST_ERROR
          }
        }
      }
      for (i = 0; i < rules.SO_SOURCES; i++)
      {
        if (kill(sources[i], SIGUSR1) < 0)
        {
          if (errno == EINVAL)
          {
          }
          else
          {
            TEST_ERROR;
          }
        }
      }
    }

    break;
  case SIGINT:
    if (getpid() == parent_pid)
    {
      msgctl(q_id, IPC_RMID, NULL);
      while (shmctl(shm_id, IPC_RMID, NULL))
      {
        TEST_ERROR;
      }
      semctl(sem_id_cell, 0, IPC_RMID);
      semctl(sem_id, 0, IPC_RMID);
    }
    exit(0);

  case SIGUSR1:
    break;
  case SIGUSR2:
    TEST_ERROR;
  }
}

/*----------------------------------------------------------------------------*/

int random_extraction(int a, int b)
{
  int r;
  if (a < b)
  {
    r = (rand() % (b - a + 1)) + a;
  }
  else
  {
    r = (rand() % (a - b + 1)) + b;
  }
  return r;
}

/*----------------------------------------------------------------------------*/

settings cfg(settings rules)
{
  FILE *config;
  int i;
  int arrayFile[10];
  config = fopen("custom.cfg", "r");
  for (i = 0; i < 10; i++)
  {
    fscanf(config, "%d", &arrayFile[i]);
  }
  fclose(config);
  rules.SO_HOLES = arrayFile[0];
  rules.SO_TOP_CELLS = arrayFile[1];
  rules.SO_SOURCES = arrayFile[2];
  rules.SO_CAP_MIN = arrayFile[3];
  rules.SO_CAP_MAX = arrayFile[4];
  rules.SO_TAXI = arrayFile[5];
  rules.SO_TIMENSEC_MIN = arrayFile[6];
  rules.SO_TIMENSEC_MAX = arrayFile[7];
  rules.SO_TIMEOUT = arrayFile[8];
  rules.SO_DURATION = arrayFile[9];
  return rules;
}

/*----------------------------------------------------------------------------*/

cell cell_gen(settings rules, int x, int y, int id, struct sembuf sops)
{
  cell new_cell;
  new_cell.id = id;
  new_cell.x = x;
  new_cell.y = y;
  new_cell.durata_attraversamento =
      random_extraction(rules.SO_TIMENSEC_MIN, rules.SO_TIMENSEC_MAX);
  new_cell.cap = random_extraction(rules.SO_CAP_MIN, rules.SO_CAP_MAX);
  new_cell.type = 1; // definito dopo
  new_cell.here = 0;
  new_cell.n_attraversamenti = 0;
  semctl(sem_id_cell, id - 1, SETVAL, new_cell.cap);

  new_cell.sem_id = sem_id_cell;
  return new_cell;
}

/*----------------------------------------------------------------------------*/

cell hole_gen(cell cella)
{
  cella.durata_attraversamento = 999999999;
  cella.cap = 0;
  cella.type = 0;
  return cella;
}

cell source_gen(cell cella)
{
  cella.type = 2;
  cella.status = 0;
  return cella;
}

/*----------------------------------------------------------------------------*/

int check_neighbours(map this_map, cell cella, int x, int y)
{
  int r = -1;
  if (this_map.city[x - 1][y - 1].type != 0 &&
      this_map.city[x - 1][y].type != 0 &&
      this_map.city[x - 1][y + 1].type != 0 &&
      this_map.city[x][y - 1].type != 0 && this_map.city[x][y + 1].type != 0 &&
      this_map.city[x + 1][y - 1].type != 0 &&
      this_map.city[x + 1][y].type != 0 &&
      this_map.city[x + 1][y + 1].type != 0)
  {
    r = 1;
  }
  return r;
}

/*----------------------------------------------------------------------------*/

map map_gen(settings rules, struct sembuf sops)
{
  int i, j, x, y, id, c, k;
  map new_map;
  int max_holes = SO_WIDTH * SO_HEIGHT / 9;
  if (rules.SO_HOLES > max_holes)
  {
    printf(
        "**IMPOSSIBILE CREARE UNA MAPPA ADEGUATA** \n**AMPLIARE LA MAPPA O "
        "RIDURRE IL NUMERO DI SO_HOLES**\n");
    exit(0);
  }
  else
  {
    id = 1;
    for (i = 0; i < SO_WIDTH; i++)
    {
      for (j = 0; j < SO_HEIGHT; j++)
      {
        new_map.city[i][j] = cell_gen(rules, i, j, id, sops);
        id++;
      }
    }
    k = 0;
    while (k < rules.SO_HOLES + 1)
    {
      // se la cella estratta è sui bordi della mappa non può essere hole
      x = random_extraction(1, SO_WIDTH - 2);
      y = random_extraction(1, SO_HEIGHT - 2);
      if (check_neighbours(new_map, new_map.city[x][y], x, y) == 1)
      {
        new_map.city[x][y] = hole_gen(new_map.city[x][y]);
        k++;
      }
    }
    k = 0;
    while (k < rules.SO_SOURCES)
    {
      x = random_extraction(0, SO_WIDTH - 1);
      y = random_extraction(0, SO_HEIGHT - 1);
      if (new_map.city[x][y].type == 1)
      {
        new_map.city[x][y] = source_gen(new_map.city[x][y]);
        k++;
      }
    }
  }
  return new_map;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

void print_map(map my_map)
{
  int i, j;
  printf("******************************************************\n");
  printf("|  ID  |  X  |  Y  |   DURATA      | ");
  printf("CAP | TIPO | TAXI \n");
  for (i = 0; i < SO_WIDTH; i++)
  {
    for (j = 0; j < SO_HEIGHT; j++)
    {
      printf("|  %d   |  %d  |  %d  |  %d    | %d   |  %d   | %d\n",
             my_map.city[i][j].id, my_map.city[i][j].x, my_map.city[i][j].y,
             my_map.city[i][j].durata_attraversamento, my_map.city[i][j].cap,
             my_map.city[i][j].type, my_map.city[i][j].here);
    }
  }
}

/*----------------------------------------------------------------------------*/

cell get_random_source(map my_map)
{
  int i, x, y;
  i = 0;
  while (i < 1)
  {
    x = random_extraction(0, SO_WIDTH - 1);
    y = random_extraction(0, SO_HEIGHT - 1);
    if (my_map.city[x][y].type == 2)
    {
      i++;
    }
  }
  return my_map.city[x][y];
}

/*----------------------------------------------------------------------------*/

cell get_valid_source(map my_map)
{
  cell s;
  int i = 0;
  while (i < 1)
  {
    s = get_random_source(my_map);
    if (my_map.city[s.x][s.y].here < my_map.city[s.x][s.y].cap)
    {
      i++;
    }
  }
  return s;
}

/*----------------------------------------------------------------------------*/

cell get_new_source(shared_data *shared)
{
  cell s;
  int i = 0;
  while (i < 1)
  {
    s = get_random_source(shared->m);
    if ((shared->m.city[s.x][s.y].status == 0))
    {
      shared->m.city[s.x][s.y].status = 1;
      i++;
    }
  }
  return s;
}
/*----------------------------------------------------------------------------*/

cell get_random_cell(map my_map)
{
  int i, x, y;
  i = 0;
  while (i < 1)
  {
    x = random_extraction(0, SO_WIDTH - 1);
    y = random_extraction(0, SO_HEIGHT - 1);
    if (my_map.city[x][y].type != 0)
    {
      i++;
    }
  }
  return my_map.city[x][y];
}

/*----------------------------------------------------------------------------*/

cell set_taxi(shared_data *shared)
{
  int i = 0;
  cell pos;
  while (i < 1)
  {
    pos = get_random_cell(shared->m);
    if (shared->m.city[pos.x][pos.y].here < shared->m.city[pos.x][pos.y].cap)
    {
      shared->m.city[pos.x][pos.y].here++;
      i++;
    }
  }

  return pos;
}

/*----------------------------------------------------------------------------*/

taxi taxi_gen(shared_data *shared)
{
  taxi cab;
  cab.id = getpid();
  cab.position = set_taxi(shared);
  cab.destination = get_valid_source(shared->m);
  cab.stats.km[0] = cab.id;
  cab.stats.km[1] = 0;
  cab.stats.tempo[0] = cab.id;
  cab.stats.tempo[1] = 0;
  cab.stats.clienti[0] = cab.id;
  cab.stats.clienti[1] = 0;
  return cab;
}

/*----------------------------------------------------------------------------*/

void print_taxi(taxi t)
{
  printf("TAXI\n");
  printf("id = %d\n", t.id);
  printf("position id = %d\n", t.position.id);
  printf("destination id = %d\n", t.destination.id);
  printf("percorsi = %d celle\n", t.stats.km[1]);
  printf("trascorsi = %d nanosecondi\n", t.stats.tempo[1]);
  printf("trasportati = %d clienti\n", t.stats.clienti[1]);
}

/*----------------------------------------------------------------------------*/

message msg_gen(map my_map, cell s)
{
  message msg;
  msg.mtype = s.id;
  msg.created_by = getpid();
  msg.origin = s;
  msg.dest = get_random_cell(my_map);
  return msg;
}

/*----------------------------------------------------------------------------*/
void print_msg(message msg)
{
  printf("MSG\n");
  printf("mtype = %ld\n", msg.mtype);
  printf("created by = %d\n", msg.created_by);
  printf("origin = %d\n", msg.origin.id);
  printf("destination id = %d\n", msg.dest.id);
}

/*----------------------------------------------------------------------------*/

void print_stats(shared_data *shared, settings rules)
{
  int i, j;
  printf("******************************************************\n");
  printf("statistiche sui viaggi\n");
  printf("numero viaggi completati %d\n", shared->s.v_comp);
  printf("numero viaggi annullati %d\n", shared->s.v_abort);
  printf("numero viaggi evasi %d\n", shared->s.evasi);

  printf("SO_SOURCES\n");
  for (i = 0; i < SO_HEIGHT; i++)
  {
    for (j = 0; j < SO_WIDTH; j++)
    {
      if (shared->m.city[i][j].type == 2)
      {
        printf("id %d\n", shared->m.city[i][j].id);
      }
    }
  }
  // printf("Top cells\n");
  // for (i = 0; i < rules.SO_TOP_CELLS; i++) {
  //   printf("id = %d\n", shared->s.top_cells[i]);
  // }
  // printf("******************************************************\n");
  // printf("statistiche sui taxi\n");
  // printf("%d ha percorso %d celle\n", shared->s.top_taxi.km[0],
  //        shared->s.top_taxi.km[1]);
  // printf("%d ha viaggiato per %d nanosec\n", shared->s.top_taxi.tempo[0],
  //        shared->s.top_taxi.tempo[1]);
  // printf("%d ha trasportato %d clienti\n", shared->s.top_taxi.clienti[0],
  //        shared->s.top_taxi.clienti[1]);
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

cell go_up(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  // printf("Going up\n");

  next_pos = shared->m.city[position.x - 1][position.y];
  shared->m.city[position.x - 1][position.y].n_attraversamenti++;
  t.stats.km[1]++;
  t.stats.tempo[1] = t.stats.tempo[1] + next_pos.durata_attraversamento;
  my_time.tv_sec = 1;
  my_time.tv_nsec = next_pos.durata_attraversamento;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/*----------------------------------------------------------------------------*/

cell go_down(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  // printf("Going down\n");
  next_pos = shared->m.city[position.x + 1][position.y];
  shared->m.city[position.x + 1][position.y].n_attraversamenti++;
  t.stats.km[1]++;
  t.stats.tempo[1] = t.stats.tempo[1] + next_pos.durata_attraversamento;
  my_time.tv_sec = 1;
  my_time.tv_nsec = next_pos.durata_attraversamento;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/*----------------------------------------------------------------------------*/

cell go_left(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  // printf("Going left\n");
  next_pos = shared->m.city[position.x][position.y - 1];
  shared->m.city[position.x][position.y - 1].n_attraversamenti++;
  t.stats.km[1]++;
  t.stats.tempo[1] = t.stats.tempo[1] + next_pos.durata_attraversamento;
  my_time.tv_sec = 1;
  my_time.tv_nsec = next_pos.durata_attraversamento;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/*----------------------------------------------------------------------------*/

cell go_right(shared_data *shared, cell position, taxi t)
{
  cell next_pos;
  // printf("Going right\n");

  next_pos = shared->m.city[position.x][position.y + 1];
  shared->m.city[position.x][position.y + 1].n_attraversamenti++;
  t.stats.km[1]++;
  t.stats.tempo[1] = t.stats.tempo[1] + next_pos.durata_attraversamento;
  my_time.tv_sec = 1;
  my_time.tv_nsec = next_pos.durata_attraversamento;
  nanosleep(&my_time, NULL);
  return next_pos;
}

/*----------------------------------------------------------------------------*/

taxi drive(shared_data *shared, taxi t)
{
  int j, u, d, l, r, tmp, age = 0;
  int i = 0;

  struct sembuf sops;

  settings rules;
  rules = cfg(rules);
  while (i < 1 && shared->sim_timeout)
  {
    if (t.position.id == t.destination.id)
    {
      //   printf("PID: %d DESTINATION REACHED at cell %d\n", t.id,
      //   t.position.id);
      shared->s.v_comp++;
      r = 1;
      i++;
    }
    else
    {
      int current = manhattan(t.position.x, t.destination.x, t.position.y,
                              t.destination.y);
      int up = manhattan(t.position.x - 1, t.destination.x, t.position.y,
                         t.destination.y) -
               age;
      int down = manhattan(t.position.x + 1, t.destination.x, t.position.y,
                           t.destination.y) -
                 age;
      int left = manhattan(t.position.x, t.destination.x, t.position.y - 1,
                           t.destination.y) -
                 age;
      int right = manhattan(t.position.x, t.destination.x, t.position.y + 1,
                            t.destination.y) -
                  age;

      tmp = t.position.id - 1;

      if (shared->m.city[t.position.x - 1][t.position.y].type != 0 ||
          shared->m.city[t.position.x - 1][t.position.y].id != tmp + 1)
      {
        u = 1;
      }
      else
      {
        u = 0;
      }
      if (shared->m.city[t.position.x + 1][t.position.y].type != 0 ||
          shared->m.city[t.position.x + 1][t.position.y].id != tmp + 1)
      {
        d = 1;
      }
      else
      {
        d = 0;
      }
      if (shared->m.city[t.position.x][t.position.y - 1].type != 0 ||
          shared->m.city[t.position.x][t.position.y - 1].id != tmp + 1)
      {
        l = 1;
      }
      else
      {
        l = 0;
      }
      if (shared->m.city[t.position.x][t.position.y + 1].type != 0 ||
          shared->m.city[t.position.x][t.position.y + 1].id != tmp + 1)
      {
        r = 1;
      }
      else
      {
        r = 0;
      }

      // printf("pid %d sta guidando...\n", getpid());

      if (up >= 0 && up <= current && u)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;
        alarm(0);
        // printf("PID: %d sem acquired\n", getpid());
        t.position = go_up(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x + 1][t.position.y].here--;
      }

      //   else  {
      //     printf(" pid %d manhattan curr = %d manhattan down = %d\n",
      //     getpid(),
      //            current, up);
      //   }
      else if (down >= 0 && down <= current && d)
      {
        alarm(shared->taxi_timeout);
        age = 0;
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;

        alarm(0);

        // printf("PID: %d sem acquired\n", getpid());
        t.position = go_down(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x - 1][t.position.y].here--;
      }

      //   else {
      //     printf(" pid %d manhattan curr = %d manhattan down = %d\n",
      //     getpid(),
      //            current, down);
      //   }
      else if (left >= 0 && left <= current && l)
      {
        age = 0;
        alarm(shared->taxi_timeout);
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;

        alarm(0);
        //   printf("PID: %d sem acquired\n", getpid());
        t.position = go_left(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y - 1].here--;
      }

      //   else {
      //     printf("pid %d manhattan curr = %d manhattan right = %d\n",
      //     getpid(),
      //            current, left);
      //   }
      else if (right >= 0 && right <= current && r)
      {
        alarm(shared->taxi_timeout);
        age = 0;
        sem_reserve_sim(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y].here++;

        alarm(0);

        //   printf("PID: %d sem acquired\n", getpid());
        // shared->m.city[t.position.x][t.position.y].here--;
        t.position = go_right(shared, t.position, t);
        sem_release(sem_id_cell, tmp);
        shared->m.city[t.position.x][t.position.y + 1].here--;
      }

      else
      {
        age++;
        printf("curr %d up %d\n", current, up);
        printf("curr %d down %d\n", current, down);
        printf("curr %d left %d\n", current, left);
        printf("curr %d right %d\n", current, right);
        printf("pid %d position %d destination = %d \n", getpid(),
               t.position.id, t.destination.id);
        // printf("pid %d lost\n", getpid());
      }
    }
  }
  // printf("fine  drive %d\n", getpid());
  return t;
}

/* Try to access the resource */
int sem_reserve(int sem_id, int sem_num)
{
  struct sembuf sops;
  // printf("pid %d waiting on sem %d\n", getpid(), sem_num + 1);

  sops.sem_num = sem_num;
  sops.sem_op = -1;
  sops.sem_flg = 0;
  return semop(sem_id, &sops, 1);
}

/*----------------------------------------------------------------------------*/
/* Try to access the resource in simulation*/
int sem_reserve_sim(int sem_id, int sem_num)
{
  struct sembuf sops;
  settings rules;
  rules = cfg(rules);
  int i;
  int r;
  // printf("pid %d waiting in cell %d\n", getpid(), sem_num + 1);

  sops.sem_num = sem_num;
  sops.sem_op = -1;
  if (r = semop(sem_id, &sops, 1) < 0)
  {
    if (errno == EINTR)
    {
      printf(" pid %d exit 129\n", getpid());
      // for (i = 0; i < rules.SO_TAXI; i++) {
      //   if (taxis[i] == getpid()) {
      //     taxis[i] = 0;
      //   }
      //   printf(" %d \n", taxis[i]);
      // }
      // sem_release(sem_id, sem_num);
      exit(129);
    }
    else
    {
      TEST_ERROR;
    }
  }
  // printf("semop = %d\n", r);
  return r;
}

/*----------------------------------------------------------------------------*/

/* Release the resource */
int sem_release(int sem_id, int sem_num)
{
  struct sembuf sops;
  // printf("pid %d rilascia su %d \n", getpid(), sem_num + 1);
  sops.sem_num = sem_num;
  sops.sem_op = 1;
  sops.sem_flg = 0;

  return semop(sem_id, &sops, 1);
}

/*----------------------------------------------------------------------------*/
int wait_for_zero(int sem_id, int sem_num)
{
  struct sembuf sops;
  sops.sem_num = sem_num;
  sops.sem_op = -1;
  semop(sem_id, &sops, 1);

  sops.sem_op = 0;
  return semop(sem_id, &sops, 1);
}

/*----------------------------------------------------------------------------*/

int manhattan(int a, int b, int x, int y) { return abs(a - b) + abs(x - y); }
