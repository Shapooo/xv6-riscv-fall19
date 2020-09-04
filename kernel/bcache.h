#ifndef __BCACHE_H__
#define __BCACHE_H__
#include "buf.h"
#include "param.h"
#include "spinlock.h"
struct bclist_t
{
  struct spinlock lock;

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head;
};
#endif // __BCACHE_H__
