#include <hash.h>
#include <bitmap.h>

//Data structures for the frame table allocator

struct frame_table {
  //fr is a hashtable containing frame table entries
  struct hash fr; 
  //Used to keep track of free frames
  struct bitmap *free_frames; 
  //Required for synchronizing read writes to the frame table
  struct lock lock;
};

struct frame_table_entry {
  //Pointer to the user page
  void * frame;
  //Use for other info on the frame
  uint8_t unused;
  //Each entry is a hash element in frame table's fr
  struct hash_elem hash_elem;
};




