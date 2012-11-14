#ifndef PAGE_H
#define PAGE_H

#include <hash.h>

#define NEW_PAGE 1 
#define UNMAPPED_PAGE 2
#define ZERO_PAGE 4
#define FILE_READ_PAGE 8
/* Very similar to the frame table entries
   Each thread has its own supplemental page table (spt)
   spt keeps track of all pages accessed/modified by thread
*/

struct sup_page_table {
   /* Current thread's pdbr (CR3) */
   uint8_t pd;
   /* Our SPT is a hash table containing pages */
   struct hash *pages;
};

/* Each page contains the address of the actual page
   and the associated bits. i.e. it is actually a pte 
*/
struct page
{ 
   struct hash_elem hash_elem;
   /* User Virtual Address of actual page */
   void *addr;
   /* Kernel virtual address of the frame*/
   void *kaddr;
   /* Dirty set when page modified*/
   uint8_t dirty; 
   /* Set when page has been read/written after creation */
   uint8_t accessed; 
   uint8_t flags;
   /* pointer to a file (if any) the page will access*/
   void *file_p;
};

 struct page * create_page(void *vaddr);
 struct page * create_unmapped_page (void *addr, uint8_t flags);
 void map_frame_to_page (void *addr, void *frame);
 struct page * map_page_to_frame (void *page, int flags);
 void set_page_accessed (struct page * page);
 void set_page_dirty (struct page * page);

 unsigned page_hash (const struct hash_elem *p_, void *aux);
 bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux);
 struct page * page_lookup (const void *address);

#endif
