#ifndef __MMAP_H__
#define __MMAP_H__
#include "param.h"
#include "types.h"
#define MAXMMAP 10
#define MMAP_MIN_ADDR 65536

#define MMAPPROT2PTE(prot) (((prot) << 1) | PTE_U)

struct mmapitem_t
{
  struct file* file;
  uint64 vstart;
  uint64 fileoff;
  uint64 vend;
  uint flags;
  uint prot;
};
#endif // __MMAP_H__
