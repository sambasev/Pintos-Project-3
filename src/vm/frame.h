#include <hash.h>
#include <bitmap.h>

//Data structures for the frame table allocator

struct frame_table {
  //fr is a hashtable containing frame table entries
  struct hash ft; 
  //Used to keep track of free frames
  struct bitmap *bm_frames; 
  //Required for synchronizing read writes to the frame table
  struct lock lock;
};

struct frame_entry {
  //Pointer to the user page
  void * frame;
  //Use for other info on the frame
  uint8_t unused;
  //Each entry is a hash element in frame table's fr
  struct hash_elem hash_elem;
};


void init_frame_table (size_t frame_cnt);
void * get_frame (enum palloc_flags flags);
unsigned frame_hash (const struct hash_elem *p_, void *aux UNUSED);
bool frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
void bitmap_init (struct bitmap *bm, size_t frame_cnt);

