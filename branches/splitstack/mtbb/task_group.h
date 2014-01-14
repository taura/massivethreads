/* 
 * task_group.h
 */

/* 
 * this file provides a class task_group compatible to
 * the task_group class in the Intel Threading Building Block.
 * see the following TBB "design pattern" page to learn what
 * task_group class provides.
 http://software.intel.com/sites/products/documentation/doclib/tbb_sa/help/tbb_userguide/Design_Patterns/Divide_and_Conquer.htm

 * unlike TBB, scheduler is a genuine, greedy work-first work 
 * stealing scheduler, which tends to have a better scalability
 * than TBB.
 * 
 */

#pragma once
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <functional>


/* 
 * 
 */

#if !defined(TO_MTHREAD) && !defined(TO_MTHREAD_NATIVE) && !defined(TO_QTHREAD) && !defined(TO_NANOX)
/* from Nov 8. 2013, native is default */
#define TO_MTHREAD_NATIVE 1
/* the default used to be pthread-compatible MassiveThreads */
// #define TO_MTHREAD 1
#endif

#if TO_QTHREAD 
#include <qthread.h>
#elif TO_NANOX
#include <nanos.h>
#elif TO_MTHREAD
#include <pthread.h>
#elif TO_MTHREAD_NATIVE
#include <myth.h>
#else
#error "none of TO_QTHREAD/TO_NANOX/TO_MTHREAD/TO_MTHREAD_NATIVE defined"
#endif

#if TO_MTHREAD
#define th_func_ret_type void *
#elif TO_MTHREAD_NATIVE
#define th_func_ret_type void *
#elif TO_QTHREAD
#define th_func_ret_type aligned_t
#elif TO_NANOX
#define th_func_ret_type void
#endif

#if !defined(TASK_GROUP_INIT_SZ)
#define TASK_GROUP_INIT_SZ 8
#endif

#if !defined(TASK_GROUP_NULL_CREATE)
#define TASK_GROUP_NULL_CREATE 0
#endif

#if !defined(TASK_GROUP_VER)
#define TASK_GROUP_VER 2
#endif

#if !defined(TASK_MEMORY_CHUNK_SZ)
#define TASK_MEMORY_CHUNK_SZ 256
#endif

namespace mtbb {

  struct task {
#if TASK_GROUP_VER >= 2
    virtual void * execute() = 0;
#else
    std::function<void ()> f;
#endif
#if TO_MTHREAD
    pthread_t tid;
#elif TO_MTHREAD_NATIVE
    myth_thread_t hthread;
#elif TO_QTHREAD
    aligned_t ret;
#elif TO_NANOX
    
#else
#error "none of TO_QTHREAD/TO_NANOX/TO_MTHREAD/TO_MTHREAD_NATIVE defined"
#endif
  };

#if TASK_GROUP_VER >= 2
  template<typename F>
  struct callable_task : task {
    F f;
    callable_task(F f) : f(f) {}
    void * execute() { f(); return NULL; }
  };
#endif

  struct task_list_node {
    task_list_node * next;
    int capacity;
    int n;
#if TASK_GROUP_VER >= 2
    task * a[TASK_GROUP_INIT_SZ];
#else
    task a[TASK_GROUP_INIT_SZ];
#endif
    void init() {
      next = NULL;
      capacity = TASK_GROUP_INIT_SZ;
      n = 0;
    }
  };
  
  struct task_list {
    task_list_node head[1];
    task_list_node * tail;

    void init() {
      head->init();
      tail = head;
    }
    void reset() {
      task_list_node * q = NULL;
      for (task_list_node * p = head->next; p; p = q) {
	q = p->next;
	delete p;
      }
      init();
    }
    void new_node() {
      task_list_node * new_node = new task_list_node();
      new_node->init();
      tail->next = new_node;
      tail = new_node;
    }
    task * add(
#if TASK_GROUP_VER >= 2
	       task * t
#else
	       std::function<void ()> f
#endif
	       ) {
      if (tail->n == tail->capacity) new_node();

#if TASK_GROUP_VER >= 2
      tail->a[tail->n] = t;
#else
      task * t = &tail->a[tail->n];
      t->f = f;
#endif
      tail->n++;
      return t;
    }
  };

#if TASK_GROUP_VER >= 2
  struct task_memory_chunk {
    char a_[TASK_MEMORY_CHUNK_SZ];
    char * a;
    char * p;			/* allocation ptr */
    char * end;
    task_memory_chunk * next;
    void init(size_t s) {
      if (s <= TASK_MEMORY_CHUNK_SZ) {
	s = TASK_MEMORY_CHUNK_SZ;
	a = a_;
      } else {
	a = new char[s];
      }
      end = a + s;
      p = a;
      next = 0;
    }
    ~task_memory_chunk() {
      if (a != a_) delete a;
    }
  };

  struct task_memory_allocator {
    task_memory_chunk head[1];
    task_memory_chunk * tail;
    void init() { 
      head->init(TASK_MEMORY_CHUNK_SZ);
      tail = head;
    }
    char * new_chunk(size_t s) {
      task_memory_chunk * ch = new task_memory_chunk();
      ch->init(s);
      tail->next = ch;
      tail = ch;
      return ch->p;
    }
    void * alloc(size_t s) {
      char * p = tail->p;
      if (p + s > tail->end)
	p = new_chunk(s);
      assert(tail->p == p);
      assert(tail->p + s <= tail->end);
      tail->p = p + s;
      return (void *)p;
    }
    void reset() {
      task_memory_chunk * q = NULL;
      for (task_memory_chunk * p = head->next; p; p = q) {
	q = p->next;
	delete p;
      }
      init();
    }
  };
#endif

  static th_func_ret_type invoke_task(void * arg_) {
    task * arg = (task *)arg_;
#if TASK_GROUP_VER >= 2
    arg->execute();
#else
    std::function<void()> f = arg->f;
    f();
#endif
#if TO_MTHREAD || TO_QTHREAD || TO_MTHREAD_NATIVE
    return 0;
#elif TO_NANOX
    
#else
#error "none of TO_QTHREAD/TO_NANOX/TO_MTHREAD/TO_MTHREAD_NATIVE defined"
#endif
  }
  
#if TO_NANOX
  static nanos_smp_args_t invoke_task_arg={invoke_task};
  
  struct nanos_const_wd_definition_for_task {
    nanos_const_wd_definition_t base;
    nanos_device_t devices[1];
  };
  
  static struct nanos_const_wd_definition_for_task wd_definition_for_task = {
    {
      {
	0,			/* mandatory_creation */
	0,			/* tied */
	0,0,0,0,0,0		/* reserved0-5 */
      },
      __alignof__(struct task),	/* data_alignment */
      0,			/* num_copies */
      1,			/* num_devices */
      1,			/* num_dimensions */
      NULL			/* description */
    },
    {
      {
	nanos_smp_factory,
	&invoke_task_arg
      }
    }
  };
#endif
  
  struct task_group {
    task_list tasks;
#if TASK_GROUP_VER >= 2
    task_memory_allocator mem;
#endif
    task_group() {
      tasks.init();
#if TASK_GROUP_VER >= 2
      mem.init();
#endif
    }

    void 
#if TASK_GROUP_VER >= 2
    run_task(task * t)  
#else
      run(std::function<void ()> f) 
#endif
    {
#if TASK_GROUP_VER >= 2
      tasks.add(t);
#else
      task * t = tasks.add(f);
#endif
#if TASK_GROUP_NULL_CREATE
      invoke_task((void *)t);
#elif TO_MTHREAD
      pthread_create(&t->tid, NULL, invoke_task, (void*)t);
#elif TO_MTHREAD_NATIVE
      t->hthread=myth_create(invoke_task,(void*)t);
#elif TO_QTHREAD
      qthread_fork(invoke_task, (void*)t, &t->ret);
#elif TO_NANOX
      nanos_wd_t wd=NULL;
      nanos_wd_dyn_props_t dyn_props = { { 0,0,0,0,0,0,0,0 }, 0, 0 };
      NANOS_SAFE(nanos_create_wd_compact(&wd,&wd_definition_for_task.base,
					 &dyn_props,
					 sizeof(struct task), (void**)&t,
					 nanos_current_wd(),NULL,NULL));
#if OBSOLETE_NANOS
      /* Nanos at some prior versions. obsolete  */
      nanos_device_t dev[1] = {NANOS_SMP_DESC(invoke_task_arg)};
      nanos_wd_props_t props;	// originally, ={true,false,false};
      props.mandatory_creation = true;
      props.tied = false;
      props.reserved0 = false;
      NANOS_SAFE(nanos_create_wd(&wd,1,dev,sizeof(struct task),
				 __alignof__(struct task),
				 (void**)&t,nanos_current_wd(),&props,0,NULL));
#endif
      NANOS_SAFE(nanos_submit(wd,0,0,0));
#else
#error "none of TO_QTHREAD/TO_NANOX/TO_MTHREAD/TO_MTHREAD_NATIVE defined"
#endif
    }

#if TASK_GROUP_VER >= 2
    void run_task_if(bool x, task * t) {
      if (x) run_task(t);
      else t->execute();
    }
#endif

#if TASK_GROUP_VER >= 2
    template<typename C>
    void run(C c) {
      void * a = mem.alloc(sizeof(mtbb::callable_task<C>));
      mtbb::callable_task<C> * ct = new (a) mtbb::callable_task<C>(c);
      run_task(ct);
    }
#endif

#if TASK_GROUP_VER >= 2
    template<typename C>
    void run_if(bool x, C c) {
      if (x) c();
      else run(c);
    }
#else
    void run_if(bool x, std::function<void ()> f) {
      if (x) f();
      else run(f);
    }
#endif

    void wait() {
      int n_joined = 0;
      for (task_list_node * p = tasks.head; p; p = p->next) {
	for (int i = 0; i < p->n; i++) {
#if TASK_GROUP_VER >= 2
#define DOT ->
#else
#define DOT .
#endif

#if TASK_GROUP_NULL_CREATE 
	  /* noop */
#elif TO_MTHREAD
	  pthread_join(p->a[i] DOT tid, NULL);
#elif TO_MTHREAD_NATIVE
	  myth_join(p->a[i] DOT hthread,NULL);
#elif TO_QTHREAD
	  aligned_t ret;
	  qthread_readFF(&ret,&p->a[i] DOT ret);
#elif TO_NANOX
	  if (n_joined == 0) {
	    NANOS_SAFE(nanos_wg_wait_completion(nanos_current_wd(), 1));
	  }
#else

#error "none of TO_QTHREAD/TO_NANOX/TO_MTHREAD/TO_MTHREAD_NATIVE defined"

#endif
	  n_joined++;
	}
      }
      tasks.reset();
#if TASK_GROUP_VER >= 2
      mem.reset();
#endif
    }
  };

} /* namespace mtbb */
