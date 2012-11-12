#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"

/* Implements supplemental page table 
   Code for hash table functionality taken from A.8.5 in Pintos
   reference 
*/

/* Takes a frame and maps it to a page. 
   Returns the newly created page */
struct page * map_frame_to_page (void *page_frame, int flags)
{
  struct thread *t = thread_current();
  /*Create a new page*/
  if (flags == NEW_PAGE)
    {
      struct page * kpage = malloc (sizeof(struct page));
      kpage -> addr = page_frame;
      kpage -> dirty = 0;
      kpage -> accessed = 0;
      /*Insert page into supplemental page table*/
      hash_insert (&t->page_table, &kpage->hash_elem);
      return kpage;
    } 
  else
    {
      return 0;
    }  
}

void set_page_accessed(struct page *page)
{
  page->accessed = 1; 
}

void set_page_dirty(struct page * page)
{
  page->dirty = 1;
}

unsigned page_hash (const struct hash_elem *p_, void *aux)
{
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->addr, sizeof p->addr);
}

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux)
{
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->addr < b->addr;
}

struct page * page_lookup (const void *address)
{
  struct thread *t = thread_current();
  struct page p;
  struct hash_elem *e;
  p.addr = address;
  e = hash_find (&t->page_table, &p.hash_elem);
  return e!= NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}

