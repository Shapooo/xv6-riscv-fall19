#ifndef __KALLOC_H__
#define __KALLOC_H__
#include "spinlock.h"
struct run
{
  struct run* next;
};

struct kmem_t
{
  struct spinlock lock;
  struct run* freelist;
};
#endif // __KALLOC_H__
