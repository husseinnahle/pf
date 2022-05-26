/* ********************* *
 * TP1 INF3173 H2021
 * Auteur: Hussein Nahle
 * ********************* */

#if !defined _GNU_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// isdigit()
#include <ctype.h>

// fork(), exec()
#include <sys/types.h>
#include <unistd.h>

// wait4()
#include <sys/resource.h>
#include <sys/wait.h>

// clock_gettime()
#include <time.h>

// perf_event_open()
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>


/* **********************************************************
 * USG_MSG --> utilisation de la commande
 * EXC_MSG --> exec()
 * FRK_MSG --> fork()
 * CLK_MSG --> clock_gettime()
 * ------------------------------------------
 * NANO    --> 10^9 (dix exposant neuf)
 * MICRO   --> 10^6 (dix exposant six)
 * ------------------------------------------
 * CMD_ERR --> code de retour en cas d'erreur
 */

#define USG_MSG "Usage: pf [-u|-c|-a] [-n n] [-s] commande [argument...]\n"
#define EXC_MSG "Échec execvp"
#define FRK_MSG "Échec fork\n"
#define CLK_MSG "Échec clock\n"
#define CMD_ERR 127
#define NANO 1e+9
#define MICRO 1e+6


/* **********************************************************
 * flag_t aide à gérer les options de la ligne de commande
 * Un flag représente l'état d'une option (u, c, a, n, s)
 *
 * État: -activé     --> 1
 *       -désactivé  --> 0
 *
 * Au début du programme les flags sont désactivés.
 * n_value est égale à 1 par défaut.
 *
 * total_flags  --> nombre total de flag activé
 *		    main_flags + (2 * n_flag) + s_flag
 *
 * main_flags   --> u_flag + c_flag + a_flag
 * u_flag       --> option "-u"
 * c_flag       --> option "-c"
 * a_flag       --> option "-a"
 * n_flag       --> option "-n"
 * n_value      --> valeur de n
 * s_flag       --> option "-s"
 *
 * Voir plus dans setFlags()
 */

typedef struct
{
  int total_flags;
  int main_flags;
  int u_flag;
  int c_flag;
  int a_flag;
  int n_flag;
  int n_value;
  int s_flag;
} flag_t;


/* **********************************************************
 * value_t aide à simplifier le code.
 *
 * usage 	    --> info sur le temp utilisateur
 * start	    --> début temp réel
 * stop		    --> fin temp réel
 * cpu_cycle_count  --> nombre de cycle processeur
 * returnValue      --> valeur de retour de exec
 */

typedef struct
{
  struct rusage usage;
  struct timespec start;
  struct timespec stop;
  long long cpu_cycle_count;
  int returnValue;
} value_t;


/* **********************************************************
 * av_time_t est utilisé pour stocker les différents valeurs:
 * temp utilisateur, temp réel et nombre de cycle cpu.
 */

typedef struct
{
  float user;
  float real;
  long long cpu;
} av_time_t;


/* **********************************************************
 * Trouver la longeur d'un tableau, dont le dernier élément
 * est null.
 *
 * Paramètre:
 *  char** array     --> tableau
 *
 * Retour:
 *  int size_t       --> longeur du tableau
 *
 */

size_t size(char** array)
{
  size_t i = 0;
  while(array[i] != NULL)
  {
    i++;
  }
  return i;
}


/* **********************************************************
 * Vider tout l'espace mémoire d'un pointeur de tableau
 * allouer par mallooc.
 *
 * Paramètre:
 *  char** array     --> tableau
 */

void freeArray(char** array)
{
  for(size_t i = 0; i < size(array); i++)
  {
    free(array[i]);
  }
  free(array);
}

void pexit(char* msg, char** cmd)
{
  perror(msg);
  freeArray(cmd);
  exit(CMD_ERR);
}


void printUsage()
{
  printf(USG_MSG);
  exit(CMD_ERR);
}

/* **********************************************************
 * Calculer et afficher la moyenne des valeurs demandées, en
 * fonction des flags activés.
 *
 * Paramètres:
 *  flag_t flags        --> voir struct flag_t
 *  av_time_t valTotal  --> voir struct av_time_t
 */

void printAvg(flag_t flags, av_time_t valTotal){
  if(flags.n_value > 1)
  {
    if(!flags.main_flags)
    {
      printf("%.2f", (float)(valTotal.real / flags.n_value));
    }
    
    else if(flags.u_flag)
    {
      printf("%.2f", (float)(valTotal.user / flags.n_value));
    }

    else if(flags.c_flag)
    {
      printf("%.lld", (long long)(valTotal.cpu / flags.n_value));
    }
    
    else if(flags.a_flag)
    {
      printf("%.2f ", (float)(valTotal.real / flags.n_value));
      printf("%.2f ", (float)(valTotal.user / flags.n_value));
      printf("%.lld", (long long)(valTotal.cpu / flags.n_value));
    }
    printf("\n");
  }
}

/* **********************************************************
 * Vérifier si une chaine de caractère ne contient
 * que des chiffres
 *
 * Paramètre:
 *  char* string --> chaine de caractère
 *
 * Retour:
 *  0 		 --> succès
 *
 * Exit:
 *  CMD_ERR 	 --> echec
 */

int isDecimal(char* string)
{
  for(int i = 0; i < strlen(string); i++)
  {
    if(!isdigit(string[i]))
    {
      printf("Erreur: -n %s\n", string);
      exit(CMD_ERR);
    }
  }
  return 0;
}


/* **********************************************************
 * Calculer et afficher le temp réel
 *
 * struct timespec contient:
 *   - time_t tv_sec      --> nombre de seconde
 *   - long int tv_nsec   --> nombre de nanoseconde
 *
 * Paramètres:
 *  struct timespec start --> début de l'enregistrement
 *  struct timespec stop  --> fin de l'enregistrement
 */

void printRealTime(struct timespec start, struct timespec stop, av_time_t* valTotal)
{
  double stopTime = stop.tv_sec + ((double)stop.tv_nsec / (double)NANO);
  double startTime = start.tv_sec + ((double)start.tv_nsec / (double)NANO);
  valTotal->real += (float)(stopTime - startTime);
  printf( "%.2lf", (stopTime - startTime) );
}


/* **********************************************************
 * Calculer et afficher le temp utilisateur
 *
 * struct rusage contient:
 *   - time_t tv_sec      --> nombre de seconde
 *   - long int tv_usec   --> nombre de nanoseconde
 *
 * Paramètre:
 *  struct rusage usage   --> temp utilisateur total d'execution
 */

void printUserTime(struct rusage usage, av_time_t* valTotal)
{
  float msec = usage.ru_utime.tv_usec/(float)MICRO;
  float sec = usage.ru_utime.tv_sec;
  valTotal->user += sec + msec;
  printf("%.2f", sec + msec);
}


/* **********************************************************
 * Créer un nouveau processus pour executer la commande
 * du tableau cmd. Initialiser usage et returnValue. (de value_t)
 *
 * Paramètres:
 *  char** cmd         --> tableau contenant la commande et les arguments
 *  value_t* val       --> pointeur value_t (voir struct value_t)
 */

void _system(char** cmd, value_t* val){
  int wstatus;
  pid_t cPid = fork();
  if(cPid < 0)
  {
    pexit(FRK_MSG, cmd);
  }
  else if(cPid == 0)
  {
    execvp(cmd[0], cmd);
    pexit(cmd[0], cmd);
  }
  if(wait4(-1, &wstatus, 0, &(val->usage)) == -1)
  {
  	pexit("Erreur wait4\n", cmd);
  }
  val->returnValue = WEXITSTATUS(wstatus);
}


/* **********************************************************
 * Initialiser start et stop (de struct value_t).
 * Commencer l'enregistrement, appeler _system(), terminer
 * l'enregistrement.
 *
 * Paramètres:
 *  char** cmd    --> tableau contenant la commande et les arguments
 *  value_t* val  --> pointeur value_t (voir struct value_t)
 */

void setTimeValues(char** cmd, value_t* val)
{
  if(clock_gettime(CLOCK_MONOTONIC, &val->start) == -1)
  {
    pexit(CLK_MSG, cmd);
  }
  _system(cmd, val);
  if(clock_gettime(CLOCK_MONOTONIC, &val->stop) == -1)
  {
    pexit(CLK_MSG, cmd);
  }
}


/* **********************************************************
 * Initialiser cpu_cycle_count (de struct value_t).
 *
 * Le comptage de cycles cpu commence lors de l'appel d'exec.
 * (pe.enable_on_exec = 1)
 *
 * Le compteur doit compter les cycles cpu des evenements du
 * processus fils. (pe.inherit = 1)
 *
 * On appel _system() avant de terminer le comptage.
 *
 * Paramètres:
 *  char** cmd    --> tableau contenant la commande et les arguments
 *  value_t* val  --> pointeur value_t (voir struct value_t)
 *
 * ---------------------------------------------------------
 * Cette fonction (et la fonction perf_event_open) est basée
 * sur l'exemple du man perf_event_open.
 * ---------------------------------------------------------
 */

void setCpuCycleCount(char** cmd, value_t* val){
  struct perf_event_attr pe;
  int fd;
  memset(&pe, 0, sizeof(pe));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = PERF_COUNT_HW_CPU_CYCLES;
  pe.disabled = 1;
  pe.enable_on_exec = 1;
  pe.exclude_kernel = 1;
  pe.inherit = 1;
  pe.exclude_hv = 1;
  // perf event open
  fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd == -1)
  {
    fprintf(stderr, "Error opening leader %llx\n", pe.config);
    exit(EXIT_FAILURE);
  }
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  setTimeValues(cmd, val);
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  read(fd, &val->cpu_cycle_count, sizeof(val->cpu_cycle_count));
  close(fd);
}


/* **********************************************************
 * Appeler les fonctions setCpuCycleCount() et setTimeValues().
 * Appeler les méthodes d'affichage, en fonction des flags
 * activés.
 *
 * Paramètres:
 *  char** cmd       --> tableau contenant la commande et les arguments
 *  value_t* val     --> pointeur value_t (voir struct value_t)
 *  flag_t flags     --> voir struct flag_t
 */

void run(char** cmd, value_t* val, av_time_t* valTotal, flag_t flags)
{
  setCpuCycleCount(cmd, val);
  if(flags.u_flag)
  {
    printUserTime(val->usage, valTotal);
  }

  else if(flags.c_flag)
  {
    printf("%lld", val->cpu_cycle_count);
  }
  
  else if(flags.a_flag)
  {
    printRealTime(val->start, val->stop, valTotal);
    printf(" ");
    printUserTime(val->usage, valTotal);
    printf(" ");
    printf("%lld", val->cpu_cycle_count);
  }
  
  else
  {
    printRealTime(val->start, val->stop, valTotal);
  }

  printf("\n");
  valTotal->cpu += val->cpu_cycle_count;
}


/* **********************************************************
 * Chercher la sous commande dans argv et ajouter
 * "bin/sh" et "-c" aux 2 premiers cases de cmd
 *
 * Paramètres:
 *  char** argv
 *  int argc
 *  int totaFlags    --> voir struct flag_t
 *
 * Retour:
 *  char** cmd	     --> tableau contenant la sous commande
 */

char** shCMD(char** argv, size_t size, int totalFlags)
{
  char** cmd = malloc((size+1) * sizeof(char*));
  cmd[0] = malloc(sizeof(char) * 8);
  strcpy(cmd[0], "/bin/sh");
  cmd[1] = malloc(sizeof(char)*3);
  strcpy(cmd[1], "-c");
  cmd[2] = malloc(sizeof(argv[1+totalFlags])+1);
  strcpy(cmd[2], argv[1+totalFlags]);
  return cmd;
}


/* **********************************************************
 * Chercher la sous commande dans argv
 *
 * Paramètres:
 *  char** argv
 *  int argc
 *  int totaFlags    --> voir struct flag_t
 *
 * Retour:
 *  char** cmd	     --> tableau contenant la sous commande
 */

char** bashCMD(char** argv, size_t size, int totalFlags)
{
  char** cmd = malloc((size+1) * sizeof(char*));
  for(size_t i = 0; i < size; i++)
  {
    cmd[i] = malloc(sizeof(argv[i+totalFlags+1])+1);
    strcpy(cmd[i], argv[i+totalFlags+1]);
  }
  return cmd;
}


/* **********************************************************
 * Appeler bashCMD() si le flag s est désactivé
 * Appeler shCMD() si le flag s est activé
 *
 * Paramètres:
 *  char** argv
 *  int argc
 *  flag_t flags    --> voir struct flag_t
 *
 * Retour:
 *  char** cmd	     --> tableau contenant la sous commande
 */

char** getCMD(char** argv, int argc, flag_t flags)
{
  char** cmd;
  size_t size = 0;
  if(flags.s_flag)
  {
    size = argc - flags.total_flags - 1 + 2;
    cmd = shCMD(argv, size, flags.total_flags);
  }
  else
  {
    size = argc - flags.total_flags - 1;
    cmd = bashCMD(argv, size, flags.total_flags);
  }
  cmd[size] = NULL;
  return cmd;
}

/* **********************************************************
 * Vérifier si la commmande dans argv est valide, et
 * ajuster les flags selon argv.
 *
 * Paramètres:
 *  char** argv
 *  int argc
 *
 * Retour:
 *  flag_t f --> voir struct flag_f
 */

flag_t setFlags(int argc, char** argv)
{
  flag_t f = {0, 0, 0, 0, 0, 0, 1, 0};
  for(int i = 1; i < argc; i++)
  {
    if(!strcmp(argv[i], "-u"))
    {
      if(f.main_flags || f.s_flag || f.n_flag) printUsage();
      f.u_flag++;
      f.main_flags++;
    }

    else if(!strcmp(argv[i], "-c"))
    {
      if(f.main_flags || f.s_flag || f.n_flag) printUsage();
      f.c_flag++;
      f.main_flags++;
    }

    else if(!strcmp(argv[i], "-a"))
    {
      if(f.main_flags || f.s_flag || f.n_flag) printUsage();
      f.a_flag++;
      f.main_flags++;
    }

    else if(!strcmp(argv[i], "-n"))
    {
      if(f.n_flag || f.s_flag || (isDecimal(argv[i+1]) && i+1 < argc)) printUsage();
      f.n_value = atoi(argv[i+1]);
      f.n_flag = 2;
      i++;
    }

    else if(!strcmp(argv[i], "-s"))
    {
      if(f.s_flag) printUsage();
      f.s_flag++;
    }
    
    else if(!strcmp(argv[i], "./pf"))
    {
      f.total_flags = f.main_flags + f.n_flag + f.s_flag;
      return f;
    }

    else if(argv[i][0] == '-')
    {
      printf("pf: illegal option %s", argv[i]);
      printUsage();
    }
  }
  f.total_flags = f.main_flags + f.n_flag + f.s_flag;
  return f;
}

int main(int argc, char** argv)
{
  if(argc == 1)
  {
    printUsage();
  }
  value_t val;
  av_time_t valTotal;
  flag_t flags = setFlags(argc, argv);
  char** cmd = getCMD(argv, argc, flags);
  for(int i = 0; i < flags.n_value; i++)
  {
    run(cmd, &val, &valTotal, flags);
  }
  printAvg(flags, valTotal);
  freeArray(cmd);
  return val.returnValue;
}

#endif
