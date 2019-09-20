//------------------------------------------------------------------------------
//
// memtrace
//
// trace calls to the dynamic memory manager
//
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memlog.h>
#include <memlist.h>
#include "callinfo.h"

//
// function pointers to stdlib's memory management functions
//
static void *(*mallocp)(size_t size) = NULL;
static void (*freep)(void *ptr) = NULL;
static void *(*callocp)(size_t nmemb, size_t size);
static void *(*reallocp)(void *ptr, size_t size);

//
// statistics & other global variables
//
static unsigned long n_malloc  = 0;
static unsigned long n_calloc  = 0;
static unsigned long n_realloc = 0;
static unsigned long n_allocb  = 0;
static unsigned long n_freeb   = 0;
static item *list = NULL;

//
// init - this function is called once when the shared library is loaded
//
__attribute__((constructor))
void init(void)
{
  char *error;

  LOG_START();

  // initialize a new list to keep track of all memory (de-)allocations
  // (not needed for part 1)
  list = new_list();

  // ...
}

void* malloc(size_t size) {
	char* error;
	if (!mallocp) {
        mallocp = dlsym(RTLD_NEXT, "malloc");
        if ((error = dlerror()) != NULL) {
            fputs(error, stderr);
            exit(1);
		}
	}
	n_allocb+=size;
	n_malloc++;
	void* ptr = mallocp(size);
	alloc(list, ptr, size);
	LOG_MALLOC(size, ptr);
	return ptr;
}

void* calloc(size_t nmemb, size_t size) {
	char* error;
	if (!callocp) {
        callocp = dlsym(RTLD_NEXT, "calloc");
        if ((error = dlerror()) != NULL) {
            fputs(error, stderr);
            exit(1);
		}
	}
	n_allocb+=(size*nmemb);
	n_calloc++;
	void* ptr = callocp(nmemb, size);
	alloc(list, ptr, size*nmemb);
	LOG_CALLOC(nmemb, size, ptr);
	return ptr;
}

void* realloc(void* ptr, size_t size) {
	char* error;
	char ill = 0;
	if (!reallocp) {
        reallocp = dlsym(RTLD_NEXT, "realloc");
        if ((error = dlerror()) != NULL) {
            fputs(error, stderr);
            exit(1);
		}
	}
	item* i = dealloc(list, ptr);
	n_allocb+=size;
	n_realloc++;
	if(i==NULL){
		ptr = NULL;
		ill = 1;
	}else if(i->cnt<0){
		i->cnt++;
		ptr = NULL;
		ill = 1;
	}else if(i->size>=size){
		n_freeb += (i->size-size);
	}
	void* newptr = reallocp(ptr, size);
	alloc(list, newptr, size);
	LOG_REALLOC(ptr, size, newptr);
	if(ill){
		LOG_ILL_FREE();
	}
	return newptr;
}

void free(void* ptr) {
	char* error;
	if (!freep) {
        freep = dlsym(RTLD_NEXT, "free");
        if ((error = dlerror()) != NULL) {
            fputs(error, stderr);
            exit(1);
		}
	}
	LOG_FREE(ptr);
	item* i = dealloc(list, ptr);
	if(i==NULL){
		LOG_ILL_FREE();
		return;
	}
	if(i->cnt<0){
		LOG_DOUBLE_FREE();
		i->cnt++;
		return;
	}
	n_freeb += (i->size);
	freep(ptr);
}
//
// fini - this function is called once when the shared library is unloaded
//
__attribute__((destructor))
void fini(void)
{
  // ...
  unsigned long n_avg = n_allocb/(n_malloc+n_calloc+n_realloc);
  LOG_STATISTICS(n_allocb, n_avg, n_freeb);

  assert(list != NULL);
  int nonfreed = 0;
  item *i = list->next;
  while(i != NULL){
	  if(i->cnt!=0){
		  nonfreed++;
	  }
	  i = i->next;
  }
  if(nonfreed != 0){
	  LOG_NONFREED_START();
  }
  i = list->next;
  
  while(i != NULL){
      if(i->cnt!=0)
      	LOG_BLOCK(i->ptr, i->size, i->cnt, i->fname, i->ofs);
      i = i->next;
  }

  LOG_STOP();

  // free list (not needed for part 1)
  free_list(list);
}

// ...
