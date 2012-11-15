#include "vm/frame.h"
#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include <stdbool.h>

/* Implements supplemental page table 
   Code for hash table functionality taken from A.8.5 in Pintos
   reference 
*/

/* Takes a frame and maps it to a page. 
   Returns the newly created page */
struct page * create_page(void *addr, int flags)
{
  struct page * upage = malloc (sizeof(struct page));
  upage -> addr = (void *)(((uint32_t)addr) & (~PGMASK));
  upage -> dirty = 0;
  upage -> accessed = 0;
  upage -> flags = flags;
//  hash_insert (&t->page_table, &kpage->hash_elem);
  return upage;
}

void insert_page (struct page * upage)
{
  struct thread *t = thread_current();
  hash_insert (&t->page_table, &upage->hash_elem);
}

void map_frame_to_page(void *addr, void *frame)
{
  struct page * upage = page_lookup ((void *)((uint32_t)addr & (~PGMASK)));
  upage -> addr = frame;
}

/* Used only when file read bytes == PGSIZE */
struct page * create_unmapped_page(void *addr, uint8_t flags)
{
   struct thread *t = thread_current();
   struct page * upage = malloc(sizeof(struct page));
   upage -> addr = (void *)((uint32_t)addr & (~PGMASK)); 
   upage -> flags = flags; 
   hash_insert (&t->page_table, &upage->hash_elem);
   return upage;
}

struct page * map_page_to_frame (void *addr, int flags)
{
  struct thread *t = thread_current();
  /*Create a new page*/
  if (flags == NEW_PAGE)
    {
      struct page * upage = malloc (sizeof(struct page));
      upage -> addr = (void *)((uint32_t)addr & (~PGMASK));
      upage -> dirty = 0;
      upage -> accessed = 0;
      upage -> flags = NEW_PAGE;
      /*Insert page into supplemental page table*/
      hash_insert (&t->page_table, &upage->hash_elem);
      /*Create a frame */
      void *kpage = get_frame(flags);
      if (kpage) 
	{
	  pagedir_set_page (t->pagedir, upage->addr, kpage, true);
      	}	
      else
	{
	  /*Shouldn't happen as frame eviction is taken care of in get_frame*/
	}
      return upage;
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

unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->addr, sizeof p->addr);
}

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED)
{
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->addr < b->addr;
}

struct page * page_lookup (void *address)
{
  struct thread *t = thread_current();
  struct page p;
  struct hash_elem *e;
  p.addr = (void *)((uint32_t)address & (~PGMASK));
  e = hash_find (&t->page_table, &p.hash_elem);
  return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}

