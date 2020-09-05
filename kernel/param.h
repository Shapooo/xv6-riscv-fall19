#ifndef __PARAM_H__
#define __PARAM_H__

#define NPROC        10  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       0  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define NDISK        2
#define PROT_READ	0x1    // Page can be read.
#define PROT_WRITE	0x2    // Page can be written.
#define PROT_EXEC	0x4    // Page can be executed.
#define PROT_NONE	0x0    // Page can not be accessed.
#define MAP_SHARED	 0x01  // Share changes.
#define MAP_PRIVATE	 0x02  // Changes are private.
#endif // __PARAM_H__
