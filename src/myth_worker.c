/* 
 * myth_worker.c
 */

#include "myth_config.h"
#include "config.h"

#include "myth_worker.h"
#include "myth_worker_func.h"

#if EXPERIMENTAL_SCHEDULER
static myth_thread_t myth_steal_func_with_prob(int rank);
myth_steal_func_t g_myth_steal_func = myth_steal_func_with_prob;
#else
myth_thread_t myth_default_steal_func(int rank);
myth_steal_func_t g_myth_steal_func = myth_default_steal_func;
#endif

myth_steal_func_t myth_wsapi_set_stealfunc(myth_steal_func_t fn)
{
  myth_steal_func_t prev=g_myth_steal_func;
  g_myth_steal_func=fn;
  return prev;
}

extern myth_running_env_t g_envs;

myth_thread_t myth_default_steal_func(int rank) {
  myth_running_env_t env,busy_env;
  myth_thread_t next_run = NULL;
#if MYTH_WS_PROF_DETAIL
  uint64_t t0, t1;
  t0 = myth_get_rdtsc();
#endif
  //Choose a worker thread that seems to be busy
  env = &g_envs[rank];
  busy_env = myth_env_get_first_busy(env);
  if (busy_env){
    //int ws_victim;
#if 0
#if MYTH_SCHED_LOOP_DEBUG
    myth_dprintf("env %p is trying to steal thread from %p...\n",env,busy_env);
#endif
#endif
    //ws_victim=busy_env->rank;
    //Try to steal thread
    next_run = myth_queue_take(&busy_env->runnable_q);
    if (next_run){
#if MYTH_SCHED_LOOP_DEBUG
      myth_dprintf("env %p is stealing thread %p from %p...\n",env,steal_th,busy_env);
#endif
      myth_assert(next_run->status==MYTH_STATUS_READY);
      //Change worker thread descriptor
    }
  }
#if MYTH_WS_PROF_DETAIL
  t1 = myth_get_rdtsc();
  if (g_sched_prof){
    env->prof_data.ws_attempt_count[busy_env->rank]++;
    if (next_run){
      env->prof_data.ws_hit_cycles += t1 - t0;
      env->prof_data.ws_hit_cnt++;
    }else{
      env->prof_data.ws_miss_cycles += t1 - t0;
      env->prof_data.ws_miss_cnt++;
    }
  }
#endif
  return next_run;
}

#if EXPERIMENTAL_SCHEDULER

static long * myth_steal_prob_table;

static myth_running_env_t
myth_env_choose_victim(myth_running_env_t e) {
  /* P[i] <= X < P[i+1] ==> i */
  int n = g_attr.n_workers;
  if (n == 1) {
    return NULL;
  } else {
    long * P = e->steal_prob;
    long x = nrand48(e->steal_rg);
    if (P[n - 1] <= x) {
      return &g_envs[n - 1];
    } else {
      int a = 0; int b = n - 1;
      assert(P[a] <= x);
      assert(x < P[b]);
      while (b - a > 1) {
	int c = (a + b) / 2;
	if (P[c] <= x) {
	  a = c;
	} else {
	  b = c;
	}
	assert(P[a] <= x);
	assert(x < P[b]);
      }
      assert(0 <= a);
      assert(a < n - 1);
      assert(P[a] <= x);
      assert(x < P[a + 1]);
      assert(a != e->rank);
      return &g_envs[a];
    }
  }
}

static myth_thread_t myth_steal_func_with_prob(int rank) {
  myth_running_env_t env = &g_envs[rank];
  myth_running_env_t busy_env = myth_env_choose_victim(env);
  if (busy_env){
    myth_thread_t next_run = myth_queue_take(&busy_env->runnable_q);
    if (next_run){
      myth_assert(next_run->status == MYTH_STATUS_READY);
    }
    return next_run;
  } else {
    return NULL;
  }
}

/* probability base stealing works by first building an 
   array of n_cpus x n_cpus and then converting it to 
   n_workers x n_workers.
   p is an array having n_cpus x n_cpus elements.
 */

static long * myth_prob_to_int(double * p, int n_cpus, int n_workers) {
  int i, j;
  double * q = (double *)myth_malloc(sizeof(double) * n_workers * n_workers);

  /* copy the appropriate part of p into q, 
     repeating itself if p is smaller than q */
  for (i = 0; i < n_workers; i++) {
    for (j = 0; j < n_workers; j++) {
      int ii = i % n_cpus;
      int jj = j % n_cpus;
      q[i * n_workers + j] = p[ii * n_cpus + jj];
    }
  }
  /* set diagonal line to zero */
  for (i = 0; i < n_workers; i++) {
    q[i * n_workers + i] = 0.0;
  }
  p = 0;

  /* normalize */
  for (i = 0; i < n_workers; i++) {
    double t = 0;
    for (j = 0; j < n_workers; j++) {
      t += q[i * n_workers + j];
    }
    assert(n_workers == 1 || t > 0);
    if (t > 0) {
      for (j = 0; j < n_workers; j++) {
	q[i * n_workers + j] /= t;
      }
    }
  }
  
  /* print */
  if (0) {
    for (i = 0; i < n_workers; i++) {
      for (j = 0; j < n_workers; j++) {
	if (j > 0) printf(" ");
	printf("%f", p[i * n_workers + j]);
      }
      printf("\n");
    }
  }

  /* convert p into P, whose P[i][j] is 2^31 x the 
     probability that i chooses one of 0 ... j-1 as a victim.
     the probablity i steals from j is (P[i][j+1] - P[i][j]) / 2^31.
     the victim is chosen by drawing a random number x from [0,2^31],
     and find j s.t. P[i][j] <= x < P[i][j+1] (binary search)
  */
  long * P = myth_malloc(n_workers * n_workers * sizeof(long));
  for (i = 0; i < n_workers; i++) {
    double x = 0.0;
    for (j = 0; j < n_workers; j++) {
      assert(0 <= x);
      P[i * n_workers + j] = x * (1UL << 31);
      assert(P[i * n_workers + j] <= (1UL << 31));
      x += q[i * n_workers + j];
    }
  }
  return P;
}

/* probability distribution based on a small description
   of the machine, like this:
   4,1.0x8,5.0x2,10.0

   this describes a machine that consists of 4x8x2 processors.
   you can think of it as a node having
   4 sockets at the upper most level,
   8 cores per socket at the second level,
   and 2 hardware threads per core at the third level.

   the numbers after comma describe (relative) probability
   with which a processor apart at that level is chosen as a 
   victim.

   for example, 1.0 : 5.0 : 10.0 means the following.
   let's say you have a processor trying to perform work stealing.
   let's say a processor p is in a different domain at the first level,
   q in the same domain at the first level but in a different domain
   at the second, and r in the same domain in the second domain.
   then the relative probability the first processor chooses p, q, and r
   is 1 : 5 : 10.  q is five times more likely to be chosen than p
   and r is ten times more likely to be chosen than p.

*/

typedef struct {
  double probability;
  int n_partitions;
} domain_level;

typedef struct {
  int depth;
  domain_level * levels;
} domain;

/* convert domain hierarchy of the machine given by H,
   fill p[a:b,a:b] by the steal probability. */

static void myth_domain_to_prob_rec(double * p, int a, int b, int l,
				    domain D, int n_cpus) {
  if (l == D.depth) {
    /* we are bottom of the hierarchy. we should have exactly one core.
       the steal probability is zero */
    assert(b - a == 1);
    p[0] = 0.0;
  } else {
    /* this level has np partitions */
    int np = D.levels[l].n_partitions;
    int workers_per_part = (b - a) / np;
    int i;
    assert((b - a) % np == 0);
    /* split the subarray p[a:b,a:b] into np x np partitions */
    for (i = 0; i < np; i++) {
      int s = a + i * workers_per_part;
      int j;
      for (j = 0; j < np; j++) {
	int t = a + j * workers_per_part;
	if (i == j) {
	  /* a tile on diagonal line. recursively decompose it */
	  myth_domain_to_prob_rec(p, s, s + workers_per_part, l + 1,
				  D, n_cpus);
	} else {
	  /* a tile not on diagonal line. the probability is
	     what is specified in the machine description */
	  int ii, jj;
	  for (ii = s; ii < s + workers_per_part; ii++) {
	    for (jj = t; jj < t + workers_per_part; jj++) {
	      p[ii * n_cpus + jj] = D.levels[l].probability;
	    }
	  }
	}
      }
    }
  }
}

static double * myth_domain_to_prob(domain D, int n_cpus) {
  double * p = (double *)myth_malloc(sizeof(double) * n_cpus * n_cpus);
  myth_domain_to_prob_rec(p, 0, n_cpus, 0, D, n_cpus);
  return p;
}

/* parse a string like
   a,b:c,d:e,f:...
 */
domain parse_cpu_hierarchy(char * s) {
  char * buf = strdup(s);
  char * p = buf;
  int level = 1;
  int l;
  while (1) {
    char * x = strrchr(p, 'x');
    if (x) {
      p = x + 1;
      level++;
    } else {
      break;
    }
  }
  domain_level * H = (domain_level *)malloc(sizeof(domain_level) * level);
  p = buf;
  for (l = 0; l < level; l++) {
    char * x = strrchr(p, 'x');
    /* terminate string at colon */
    if (x) {
      *x = 0;
    }
    /* look for , */
    char * comma = strrchr(p, ',');
    if (comma) {
      *comma = 0;
      H[l].n_partitions = atoi(p);
      H[l].probability = atof(comma + 1);
    } else {
      fprintf(stderr,
	      "could not parse domain string (%s)."
	      " should have a form n,p:n,p:...\n", s);
      exit(1);
    }
    p = x + 1;
  }
  domain D = { level, H };
  return D;
}

static long * myth_prob_from_domain(char * s, int n_workers) {
  int n_cpus = 1;
  int l; 
  domain D = parse_cpu_hierarchy(s);
  for (l = 0; l < D.depth; l++) {
    n_cpus *= D.levels[l].n_partitions;
  }
  double * p = myth_domain_to_prob(D, n_cpus);
  long * P = myth_prob_to_int(p, n_cpus, n_workers);
  myth_free(D.levels);
  myth_free(p);
  return P;
}

static long * myth_prob_from_file(FILE * fp, int nw) {
  /* read a file describing the (relative) probability a worker chooses
     a victime
     
     N
     P0,0 P0,1 P0,2 ... P0,N-1
     P1,0 P1,1 P1,2 ... P1,N-1
          ...
     PN-1,0,        ... PN-1,N-1

  */
  /* nw is the number of workers used in this execution
     of the program; n_workers_in_file is the number of workers
     described in the file. they may be different. */
  int n_cpus = 0;
  if (fp) {
    int x = fscanf(fp, "%d", &n_cpus);
    assert(x == 1);
  } else {
    /* no file. default (uniform distribution) */
    n_cpus = nw;
  }
  /* p[i][j] is a relative probability that i chooses j as a victim */
  double * p = myth_malloc(n_cpus * n_cpus * sizeof(double));
  int i, j;
  for (i = 0; i < n_cpus; i++) {
    for (j = 0; j < n_cpus; j++) {
      if (fp) {
	int x = fscanf(fp, "%lf", &p[i * n_cpus + j]);
	assert(x == 1);
	assert(p[i * n_cpus + j] >= 0.0);
      } else {
	p[i * n_cpus + j] = 1.0; /* uniform (diagonal line set to zero below) */
      }
    }
  }
  long * P = myth_prob_to_int(p, n_cpus, nw);
  myth_free(p);
  return P;
}

int myth_scheduler_global_init(int nw) {
  long * P = 0;
  char * hierarchy = getenv("MYTH_CPU_HIERARCHY");
  char * prob_file = getenv("MYTH_PROB_FILE");
  if (hierarchy) {
    P = myth_prob_from_domain(hierarchy, nw);
  } else if (prob_file) {
    FILE * fp = fopen(prob_file, "rb");
    if (!fp) { perror("fopen"); exit(1); }
    P = myth_prob_from_file(fp, nw);
    fclose(fp);
  } else {
    /* default. uniform */
    P = myth_prob_from_file(0, nw);
  }
  myth_steal_prob_table = P;
  return 0;
}


int myth_scheduler_worker_init(int rank, int nw) {
  myth_running_env_t env = &g_envs[rank];
  env->steal_prob = &myth_steal_prob_table[rank * nw];
  /* random seed */
  env->steal_rg[0] = rank;
  env->steal_rg[1] = rank + 1;
  env->steal_rg[2] = rank + 2;
  return 0;
}

#endif	/* EXPERIMENTAL_SCHEDULER */
