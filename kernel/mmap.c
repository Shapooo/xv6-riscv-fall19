#include "mmap.h"
#include "defs.h"
#include "file.h"
#include "proc.h"
#include "riscv.h"

// handle mmap page fault
int
mmap_handle(uint64 vaddr)
{
  int s;
  void* pa;
  struct proc* p;
  struct mmapitem_t* mip;
  struct file* f = 0;

  p = myproc();
  for (mip = p->mmap; mip < p->mmap + MAXMMAP; ++mip) {
    if (mip->vstart && (vaddr >= mip->vstart) && (vaddr < mip->vend)) {
      f = mip->file;
      break;
    }
  }
  if (mip >= p->mmap + MAXMMAP)
    panic("mmap_handle: mmap not found");

  int perm = MMAPPROT2PTE(mip->prot);

  uint64 fileoff, a;
  a = PGROUNDDOWN(vaddr);
  pa = kalloc();
  if (mappages(p->pagetable, a, PGSIZE, (uint64)pa, perm) < 0) {
    kderef(pa);
    return -1;
  }
  fileoff = a - mip->vstart + mip->fileoff;
  if (f) {
    begin_op(f->ip->dev);
    ilock(f->ip);
    s = readi(f->ip, 1, vaddr, fileoff, PGSIZE);
    iunlock(f->ip);
    end_op(f->ip->dev);
    if (PGSIZE - s)
      memset(pa + s, 0, PGSIZE - s);
  } else {
    kderef(pa);
    return -1;
  }

  return 0;
}

// vaddr must be page aligned
int
munmap(pagetable_t pagetable,
       struct mmapitem_t* mip,
       uint64 vaddr,
       uint64 vend,
       int do_update)
{
  uint64 a;
  uint64 last = PGROUNDUP(vend);

  if (vaddr % PGSIZE || mip->fileoff % PGSIZE)
    panic("munmap: un-aligned");
  if (vaddr > vend)
    return -1;

  if (do_update) {
    begin_op(mip->file->ip->dev);
    ilock(mip->file->ip);
  }

  uint fileoff = mip->fileoff + vaddr - mip->vstart;
  for (a = vaddr; a < last; a += PGSIZE, fileoff += PGSIZE) {
    if (do_update) {
      writei(mip->file->ip, 1, a, fileoff, PGSIZE);
    }
    uvmunmap(pagetable, a, PGSIZE, 1);
  }
  if (do_update) {
    iunlock(mip->file->ip);
    end_op(mip->file->ip->dev);
  }
  return 0;
}

int
mmapcopy(pagetable_t oldpt,
         pagetable_t newpt,
         struct mmapitem_t* oldmp,
         struct mmapitem_t* newmp)
{
  uint64 va;
  void* pa;
  if (oldmp->vstart % PGSIZE)
    panic("mmapcopy");
  newmp->vstart = oldmp->vstart;
  newmp->fileoff = oldmp->fileoff;
  newmp->file = filedup(oldmp->file);
  newmp->vend = oldmp->vend;
  newmp->flags = oldmp->flags;
  newmp->prot = oldmp->prot;
  for (va = oldmp->vstart; va < oldmp->vend; va += PGSIZE) {
    if (walkaddr(oldpt, va)) {
      if (oldmp->flags == MAP_PRIVATE) {
        if ((pa = kalloc()) < 0) {
          panic("mmapcopy");    /* todo: handle fail */
        }
        copyout(oldpt, va, pa, PGSIZE);
      } else {
        pa = (void*)walkaddr(oldpt, va);
      }
      mappages(newpt, va, PGSIZE, (uint64)pa, MMAPPROT2PTE(newmp->prot));
    }
  }

  return 0;
}
