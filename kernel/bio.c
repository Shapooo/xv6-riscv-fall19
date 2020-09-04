// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "bcache.h"
#include "buf.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBCBKT 13
#define NUMS(f)                                                                \
  f(0), f(1), f(2), f(3), f(4), f(5), f(6), f(7), f(8), f(9), f(a), f(b), f(c)
#define BCNAME(a) "bcache_" #a
#define MAP(c, f) c(f)
char* bcname[NBCBKT] = { MAP(NUMS, BCNAME) };
struct
{
  struct buf buf[NBUF];
  struct bclist_t bkt[NBCBKT];
} bcache;

void
binit(void)
{
  int count = 0;
  struct buf* b;
  for (int i = 0; i < NBCBKT; ++i) {
    initlock(&bcache.bkt[i].lock, bcname[i]);
    bcache.bkt[i].head.prev = &bcache.bkt[i].head;
    bcache.bkt[i].head.next = &bcache.bkt[i].head;
  }

  // Create linked list of buffers

  for (b = bcache.buf; count < NBUF; b++, count++) {
    b->next = bcache.bkt[count % NBCBKT].head.next;
    b->prev = &bcache.bkt[count % NBCBKT].head;
    initsleeplock(&b->lock, "buffer");
    bcache.bkt[count % NBCBKT].head.next->prev = b;
    bcache.bkt[count % NBCBKT].head.next = b;
  }
}

inline int
cal_hash(uint blockno)
{
  return blockno % NBCBKT;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int n = cal_hash(blockno);

  acquire(&bcache.bkt[n].lock);

  // Is the block already cached?
  for(b = bcache.bkt[n].head.next; b != &bcache.bkt[n].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bkt[n].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  for(b = bcache.bkt[n].head.prev; b != &bcache.bkt[n].head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bkt[n].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  int n = cal_hash(b->blockno);
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.bkt[n].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bkt[n].head.next;
    b->prev = &bcache.bkt[n].head;
    bcache.bkt[n].head.next->prev = b;
    bcache.bkt[n].head.next = b;
  }

  release(&bcache.bkt[n].lock);
}

void
bpin(struct buf *b) {
  int n = cal_hash(b->blockno);
  acquire(&bcache.bkt[n].lock);
  b->refcnt++;
  release(&bcache.bkt[n].lock);
}

void
bunpin(struct buf *b) {
  int n = cal_hash(b->blockno);
  acquire(&bcache.bkt[n].lock);
  b->refcnt--;
  release(&bcache.bkt[n].lock);
}
