// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "defs.h"
#include "memlayout.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "types.h"

void
freerange(void* pa_start, void* pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run* next;
};

struct
{
  struct spinlock lock;
  struct run* freelist;
  char* ref_count;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.ref_count = (char*)end;
  char* refcount_end = (char*)end;
  refcount_end += PGROUNDUP(PHYSTOP - (uint64)end) >> 12;
  memset(end, 0, refcount_end - end);
  freerange(refcount_end, (void*)PHYSTOP);
}

void
freerange(void* pa_start, void* pa_end)
{
  char* p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void* pa)
{
  struct run* r;
  uint index = ((uint64)pa - (uint64)end) >> 12;

  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.ref_count[index] = 0;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void*
kalloc(void)
{
  struct run* r;
  uint index;

  acquire(&kmem.lock);
  r = kmem.freelist;

  if (r) {
    index = ((uint64)r - (uint64)end) >> 12;
    kmem.freelist = r->next;
    kmem.ref_count[index] = 1;
  }
  release(&kmem.lock);

  if (r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
kref(void* pa)
{
  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kref");
  uint index = ((uint64)pa - (uint64)end) >> 12;

  acquire(&kmem.lock);
  kmem.ref_count[index]++;
  release(&kmem.lock);
}

void
kderef(void* pa)
{
  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kderef");
  uint index = ((uint64)pa - (uint64)end) >> 12;
  uint to_be_free = 0;

  acquire(&kmem.lock);
  kmem.ref_count[index]--;
  if (kmem.ref_count[index] < 0)
    panic("kderef");
  if (kmem.ref_count[index] == 0)
    to_be_free = 1;
  release(&kmem.lock);

  if (to_be_free)
    kfree(pa);
}
