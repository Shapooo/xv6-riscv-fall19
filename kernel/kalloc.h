struct run
{
  struct run* next;
};

struct kmem_t
{
  struct spinlock lock;
  struct run* freelist;
};
