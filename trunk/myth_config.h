#ifndef MYTH_CONFIG_H_
#define MYTH_CONFIG_H_

//Enable debug
//#define MYTH_DEBUG

//Cache line size
#define CACHE_LINE_SIZE 64

//Page size
#define PAGE_SIZE 4096

//TLS key size
#define MYTH_TLS_KEY_SIZE 256

//Stack size
#define STACK_SIZE (16*1024)
//Scheduler stack size
#define SCHED_STACK_SIZE (1024*1024)

//#define USE_STACK_GUARDPAGE

//How many stack blocks allocated at stack allocation
#define STACK_ALLOC_UNIT 128
#define INITIAL_STACK_ALLOC_UNIT 0

//Use malloc+freelist instead of mmap+freelist
//#define ALLOCATE_STACK_BY_MALLOC

//Split desc and stack allocation
#define MYTH_SPLIT_STACK_DESC

//Runqueue length
#define INITIAL_QUEUE_SIZE (65536*2)

//Wrap malloc function as worker-private freelist
#define MYTH_WRAP_MALLOC

//Wrap and multipelx I/O functions
#define MYTH_WRAP_IO

//Wrapped select() return immediately regardless of I/O readiness
#define SELECT_ALWAYS_RETURN_IMMEDIATELY

//Replace myth_assert() with assert(): used for debugging
//#define MYTH_SANITY_CHECK

//Insert special instruction into unreachable code: used for debugging
#define USE_MYTH_UNREACHABLE

//epoll file decriptor size
#define MYTH_EPOLL_SIZE 256

//Decide a fd's worker thread at random
#define MYTH_RANDOM_IO_WORKER
//Make all fds belong to worker thread 0
//#define MYTH_ONE_IO_WORKER

//Use interval timer for I/O checking and preemption
//#define MYTH_USE_ITIMER
//Use SIGIO for I/O checking
//#define MYTH_USE_SIGIO

//Use signal handler
//#define MYTH_USE_SIGHANDLER
//Use another thread
//#define MYTH_USE_IO_THREAD
//#define MYTH_IO_THREAD_PERIOD 1

//Enable PTHREAD_MUTEX_INITIALIZER
#define MYTH_SUPPORT_MUTEX_INITIALIZER
//This option makes mutex operation faster but may be unsafe in some platform
#define MYTH_UNSAFE_MUTEX_INITIALIZER

//Use spinlock as mutex, faster but higher risk of deadlock
#define MYTH_MUTEX_BY_SPIN

//Debug log setting
//#define FREE_MYTH_THREAD_STRUCT_DEBUG
//#define MYTH_INIT_DEBUG
//#define MYTH_WORKER_INIT_DEBUG
//#define MYTH_FINI_DEBUG
//#define MYTH_YIELD_DEBUG
//#define MYTH_JOIN_DEBUG
//#define MYTH_SCHED_ADD_DEBUG
//#define MYTH_SCHED_LOOP_DEBUG
//#define MYTH_ENTRY_POINT_DEBUG

//Profiling option
//#define MYTH_CREATE_PROF
//#define MYTH_CREATE_PROF_DETAIL
//#define MYTH_ENTRY_POINT_PROF
//#define MYTH_EP_PROF_DETAIL
//#define MYTH_JOIN_PROF
//#define MYTH_JOIN_PROF_DETAIL
//#define MYTH_WS_PROF_DETAIL
//#define MYTH_ALLOC_PROF
//#define MYTH_SWITCH_PROF
//#define MYTH_IO_PROF_DETAIL
//#define MYTH_FL_PROF
//Display result as CSV
//#define MYTH_PROF_COUNT_CSV
//Display result of each worker thread
//#define MYTH_PROF_SHOW_WORKER

//Profile myth_flmalloc
//#define MYTH_FLMALLOC_PROF

//Collect and emit sslog
//#define MYTH_COLLECT_LOG
//Log buffer size
#define MYTH_LOG_INITIAL_BUFFER_SIZE 1024
//Collect context switching events
#define MYTH_COLLECT_CONTEXT_SWITCH
//Enable thread annotation
#define MYTH_ENABLE_THREAD_ANNOTATION
//Maximum length for thread annotation
#define MYTH_THREAD_ANNOTATION_MAXLEN 100

//Internal lock implementation
//#define MYTH_INTERNAL_LOCK_MUTEX
//#define MYTH_INTERNAL_LOCK_SPINLOCK1
#define MYTH_INTERNAL_LOCK_SPINLOCK2

//Barrier implementations
//#define MYTH_BARRIER_FENCES
//#define MYTH_BARRIER_CILK
#define MYTH_BARRIER_CILK_WEAK
//#define MYTH_BARRIER_NONE

//Select TLS implementation
//#define TLS_BY_PTHREAD
#define TLS_BY_ELF
//#define TLS_NONE

//Choose work stealing target at random
#define WS_TARGET_RANDOM

//Bind each worker thread to each CPU
#define WORKER_SET_AFFINITY

//Switch to new thread immediately after it is created
#define SWITCH_AFTER_CREATE
//When a thread is finished, switch to the thread that waits for it.
#define SWITCH_AFTER_EXIT

//Scheduling policy of runqueue
//FIFO
//#define MYTH_QUEUE_FIFO

//Check the number of threads in runqueue before work stealing
#define CHECK_NUM_BEFORE_STEAL

//Do trylock before work stealing
//#define TRY_LOCK_BEFORE_STEAL

//Use malloc with recycle by freelist
#define USE_MYTH_FLMALLOC
//Use TLS on malloc with freelist
//#define MYTH_FLMALLOC_TLS

//Inline context switching codes by inline assembler
#define MYTH_INLINE_CONTEXT
//At inlined codes, save callee-saved registers explicitly
#define MYTH_INLINE_PUSH_CALLEE_SAVED

//Option for overhead evaluation
//#define MYTH_NO_JOIN
//#define MYTH_NO_SWITCH
//#define MYTH_ONE_STACK
//#define MYTH_NO_QUEUEOP

//To switch context, use jmp instruction instead of ret
#define USE_JUMP_INSN_A
#define USE_JUMP_INSN_B

#endif /* MYTH_CONFIG_H_ */