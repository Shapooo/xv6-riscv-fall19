// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void
freerange(void* pa_start, void* pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
extern struct cpu cpus[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; ++i) {
    initlock(&cpus[i].kmem.lock, "kmem");
  }

  freerange(end, (void*)PHYSTOP);
}

inline void
fr(void* pa, int n)
{
  struct run* r;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&cpus[n].kmem.lock);
  r->next = cpus[n].kmem.freelist;
  cpus[n].kmem.freelist = r;
  release(&cpus[n].kmem.lock);
}

void
freerange(void* pa_start, void* pa_end)
{
  char* p;
  int count = 0;

  p = (char*)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    fr(p, (count++) % NCPU);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void* pa)
{
  int n;

  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  n = mycpu() - cpus;
  fr(pa, n);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void*
kalloc(void)
{
  struct run* r;
  int n;

  n = mycpu() - cpus;

  acquire(&cpus[n].kmem.lock);
  r = cpus[n].kmem.freelist;
  if (r) {
    cpus[n].kmem.freelist = r->next;
    release(&cpus[n].kmem.lock);
    memset((char*)r, 5, PGSIZE); // fill with junk
  } else {
    release(&cpus[n].kmem.lock);
    for (int i = NCPU - 1; i >= 0; --i) {
      acquire(&cpus[i].kmem.lock);
      r = cpus[i].kmem.freelist;
      if (r) {
        cpus[i].kmem.freelist = r->next;
        release(&cpus[i].kmem.lock);
        memset((char*)r, 5, PGSIZE); // fill with junk
        break;
      } else
        release(&cpus[i].kmem.lock);
    }
  }

  return (void*)r;
}
