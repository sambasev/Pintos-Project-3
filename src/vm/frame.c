#include "frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"

/* Implement Frame table allocator/frame eviction here
   Should happen only once. Inits frame table containing 
   frame_cnt entries. Each entry in the table is a frame_entry
   that contains a pointer to a frame and its associated data
*/
struct frame_table * fr_table;
void init_frame_table(size_t frame_cnt)
{
  //Perform hash table init, struct inits here
  fr_table = malloc (sizeof(struct frame_table));
  lock_init(&fr_table->lock);
  hash_init(&fr_table->ft, frame_hash, frame_less, NULL);  
  //bitmap_init(fr_table->bm_frames, frame_cnt);
  fr_table->bm_frames = bitmap_create(frame_cnt);
}

/* Gets a physical frame if available and maps it to a page 
   obtained from the user pool
*/
void *get_frame(int flags) 
{
  //Check bitmap to see if free frame available
  struct thread *t = thread_current();
  size_t idx = bitmap_scan_and_flip (fr_table->bm_frames, 0, 1, false);
  void * free_frame;
  //If available, fetch a page from user pool by calling palloc_get_page
  if(idx != BITMAP_ERROR) 
    {
       free_frame = palloc_get_page(flags);
       if(!free_frame) 
	 {
	   /* Evict frame - shouldn't happen here since we scan 
		   the bitmap first*/
         }
    }
  else 
    {
     //if fetch failed, PANIC for now. Implement evict later.
       PANIC("out of frames!");
    }
  //else, set the appropriate bit in the ft bitmap (already done)
  //malloc free frame. map page to frame. Insert frame to hash table. 
  struct frame_entry *frame = malloc(sizeof(struct frame_entry));
  if(!frame)
    {
       PANIC("Malloc failed:Out of memory!");
    }
  frame->frame = free_frame;
  frame->pagedir = t->pagedir;
  hash_insert (&fr_table->ft, &frame->hash_elem);

  //If bitset, frame used. Else, frame available
  return free_frame;
  //Return page address
}

/*  Finds frame corresponding to addr. Frees the corresponding
    frame, updates the frame bitmap*/
void release_frame(void *addr)
{
  
}

unsigned frame_hash (const struct hash_elem *p_, void *aux UNUSED)
{
   const struct frame_entry *f = hash_entry(p_, struct frame_entry, hash_elem);
   return hash_bytes(&f->frame, sizeof f->frame);
}

bool frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
   const struct frame_entry *a = hash_entry (a_, struct frame_entry, hash_elem);
   const struct frame_entry *b = hash_entry (b_, struct frame_entry, hash_elem);
   return a->frame < b->frame;
}

void bitmap_init(struct bitmap *bm, size_t frame_cnt)
{
   bm = bitmap_create (frame_cnt); 
//   bitmap_set(bm, frame_cnt, FALSE);
}


