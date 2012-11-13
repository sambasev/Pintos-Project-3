#ifndef FRAME_H
#define FRAME_H

#include <hash.h>
#include <bitmap.h>
#include "threads/synch.h"
//Data structures for the frame table allocator
#define FALSE 0
struct frame_table {
  //fr is a hashtable containing frame table entries
  struct hash ft; 
  //Used to keep track of free frames
  struct bitmap *bm_frames; 
  /*Required for synchronizing since the frame table is global*/
  struct lock lock;
};

struct frame_entry {
  //Pointer to the user page
  void * frame;
  //Pagedir of the thread that requested the frame - will be used for page eviction
  uint32_t * pagedir;
  //Use for other info on the frame
  uint8_t unused;
  //Each entry is a hash element in frame table's fr
  struct hash_elem hash_elem;
};


void init_frame_table (size_t frame_cnt);
void * get_frame (int palloc_flags);
void release_frame (void *frame);
unsigned frame_hash (const struct hash_elem *p_, void *aux);
bool frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux);
void bitmap_init (struct bitmap *bm, size_t frame_cnt);

#endif
