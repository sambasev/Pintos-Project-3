#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "vm/page.h"

#define MAX_ARGS 3
#define USER_VADDR_BOTTOM ((void *) 0x08048000)
#define SUCCESS 1
static void syscall_handler (struct intr_frame *);
int user_to_kernel_ptr(const void *vaddr);
void get_arg (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
void check_valid_buffer (void* buffer, unsigned size);
void check_valid_string (const void* str);
void check_valid_sp (uint32_t sp);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int arg[MAX_ARGS];
  int esp = (int)f->esp;
  check_valid_sp ((uint32_t)esp);
  //printf("f->esp is 0x%x esp is %x\n", (uint32_t)f->esp, (uint32_t)esp);
  switch (* (int *) esp)
    {
    case SYS_HALT:
      {
	halt(); 
	break;
      }
    case SYS_EXIT:
      {
	get_arg(f, &arg[0], 1);
	exit(arg[0]);
	break;
      }
    case SYS_EXEC:
      {
	get_arg(f, &arg[0], 1);
        check_valid_ptr((const void *) arg[0]);
	check_valid_string((const void *) arg[0]);
	//arg[0] = user_to_kernel_ptr((const void *) arg[0]);
	//printf("Executing %s\n", (char *)arg[0]);
	f->eax = exec((const char *) arg[0]); 
	break;
      }
    case SYS_WAIT:
      {
	get_arg(f, &arg[0], 1);
	f->eax = wait(arg[0]);
	break;
      }
    case SYS_CREATE:
      {
	get_arg(f, &arg[0], 2);
        check_valid_ptr((const void *) arg[0]);
	check_valid_string((const void *) arg[0]);
	//arg[0] = user_to_kernel_ptr((const void *) arg[0]);
        f->eax = create((const char *)arg[0], (unsigned) arg[1]);
	break;
      }
    case SYS_REMOVE:
      {
	get_arg(f, &arg[0], 1);
        check_valid_ptr((const void *) arg[0]);
	check_valid_string((const void *) arg[0]);
	//arg[0] = user_to_kernel_ptr((const void *) arg[0]);
	f->eax = remove((const char *) arg[0]);
	break;
      }
    case SYS_OPEN:
      {
	get_arg(f, &arg[0], 1);
        check_valid_ptr((const void *) arg[0]);
	check_valid_string((const void *) arg[0]);
	//arg[0] = user_to_kernel_ptr((const void *) arg[0]);
	f->eax = open((const char *) arg[0]);
	break; 		
      }
    case SYS_FILESIZE:
      {
	get_arg(f, &arg[0], 1);
	f->eax = filesize(arg[0]);
	break;
      }
    case SYS_READ:
      {
	get_arg(f, &arg[0], 3);
        check_valid_ptr((const void *) arg[1]);
	check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
	//arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
	break;
      }
    case SYS_WRITE:
      { 
	get_arg(f, &arg[0], 3);
	//printf("arg[1] : %x arg[2] : %x\n", (uint32_t) arg[1], (uint32_t) arg[2]);
        check_valid_ptr((const void *) arg[1]);
	check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
	//arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	f->eax = write(arg[0], (const void *) arg[1],
		       (unsigned) arg[2]);
	break;
      }
    case SYS_SEEK:
      {
	get_arg(f, &arg[0], 2);
	seek(arg[0], (unsigned) arg[1]);
	break;
      } 
    case SYS_TELL:
      { 
	get_arg(f, &arg[0], 1);
	f->eax = tell(arg[0]);
	break;
      }
    case SYS_CLOSE:
      { 
	get_arg(f, &arg[0], 1);
	close(arg[0]);
	break;
      }
    }
}

void halt (void)
{
  shutdown_power_off();
}

void exit (int status)
{
  struct thread *cur = thread_current();
  if (thread_alive(cur->parent) && cur->cp)
    {
      cur->cp->status = status;
    }
  printf ("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

pid_t exec (const char *cmd_line)
{
  pid_t pid = process_execute(cmd_line);
  struct child_process* cp = get_child_process(pid);
  if (!cp)
    {
      return ERROR;
    }
  if (cp->load == NOT_LOADED)
    {
      sema_down(&cp->load_sema);
    }
  if (cp->load == LOAD_FAIL)
    {
      remove_child_process(cp);
      return ERROR;
    }
  return pid;
}

int wait (pid_t pid)
{
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
  if (strlen(file) == 0)
    {
      return false;
    }
  lock_acquire(&filesys_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return success;
}

bool remove (const char *file)
{
  lock_acquire(&filesys_lock);
  bool success = filesys_remove(file);
  lock_release(&filesys_lock);
  return success;
}

int open (const char *file)
{
  lock_acquire(&filesys_lock);
  struct file *f = filesys_open(file);
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  int fd = process_add_file(f);
  lock_release(&filesys_lock);
  return fd;
}

int filesize (int fd)
{
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  int size = file_length(f);
  lock_release(&filesys_lock);
  return size;
}

int read (int fd, void *buffer, unsigned size)
{
  //printf("read fd is %d\n",fd);
  if (fd == STDIN_FILENO)
    {
      unsigned i;
      uint8_t* local_buffer = (uint8_t *) buffer;
      for (i = 0; i < size; i++)
	{
	  local_buffer[i] = input_getc();
	}
      return size;
    }
  lock_acquire(&filesys_lock);
  //printf("read filesys lock acquired\n");
  struct file *f = process_get_file(fd);
  //printf("get file %x\n", (uint32_t)f);
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  struct page * debug = page_lookup((void *)buffer);
  //printf("File read f:%x, buffer: %x, size %d page_lookup flags:%d\n", 
  //        (uint32_t)f, (uint32_t)buffer, size, debug->flags);
  int bytes = file_read(f, buffer, size);
  //printf("Bytes read is %d\n",bytes);
  lock_release(&filesys_lock);
  return bytes;
}

int write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO)
    {
      //printf("write to buffer %x is:%s\n", (uint32_t)buffer, (char *)buffer);
      putbuf(buffer, size);
      return size;
    }
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  int bytes = file_write(f, buffer, size);
  lock_release(&filesys_lock);
  return bytes;
}

void seek (int fd, unsigned position)
{
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&filesys_lock);
      return;
    }
  file_seek(f, position);
  lock_release(&filesys_lock);
}

unsigned tell (int fd)
{
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  off_t offset = file_tell(f);
  lock_release(&filesys_lock);
  return offset;
}

void close (int fd)
{
  lock_acquire(&filesys_lock);
  process_close_file(fd);
  lock_release(&filesys_lock);
}

int user_to_kernel_ptr(const void *vaddr)
{
  check_valid_ptr(vaddr);
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
    {
      exit(ERROR);
    }
  return (int) ptr;
}

struct child_process* add_child_process (int pid)
{
  struct child_process* cp = malloc(sizeof(struct child_process));
  if (!cp)
    {
      return NULL;
    }
  cp->pid = pid;
  cp->load = NOT_LOADED;
  cp->wait = false;
  cp->exit = false;
  sema_init(&cp->load_sema, 0);
  sema_init(&cp->exit_sema, 0);
  list_push_back(&thread_current()->child_list,
		 &cp->elem);
  return cp;
}

struct child_process* get_child_process (int pid)
{
  struct thread *t = thread_current();
  struct list_elem *e;

  for (e = list_begin (&t->child_list); e != list_end (&t->child_list);
       e = list_next (e))
        {
          struct child_process *cp = list_entry (e, struct child_process, elem);
          if (pid == cp->pid)
	    {
	      return cp;
	    }
        }
  return NULL;
}

void remove_child_process (struct child_process *cp)
{
  list_remove(&cp->elem);
  free(cp);
}

void remove_child_processes (void)
{
  struct thread *t = thread_current();
  struct list_elem *next, *e = list_begin(&t->child_list);

  while (e != list_end (&t->child_list))
    {
      next = list_next(e);
      struct child_process *cp = list_entry (e, struct child_process,
					     elem);
      list_remove(&cp->elem);
      free(cp);
      e = next;
    }
}

void get_arg (struct intr_frame *f, int *arg, int n)
{
  int i;
  int *ptr;
  for (i = 0; i < n; i++)
    {
      ptr = (int *) f->esp + i + 1;
      check_valid_ptr((const void *) ptr);
      arg[i] = *ptr;
    }
}

void check_valid_buffer (void* buffer, unsigned size)
{
  unsigned i;
  char* local_buffer = (char *) buffer;
  for (i = 0; i < size; i++)
    {
      check_valid_ptr((const void*) local_buffer);
      local_buffer++;
    }
}

void check_valid_string (const void* str)
{
  while (* (char *) str != 0)
    {
      str = (char *) str + 1;
    }
}

/* TODO: Change uint32_t to void * */
void check_valid_sp (uint32_t sp)
{
  if (!is_user_vaddr((void *)sp) || (void *)sp < USER_VADDR_BOTTOM)
    {
      exit(ERROR);
    }
  if (page_lookup((void *)sp)) 
    {
      /* TODO: page_lookup should take care of loading the page */
    }
  else if (((uint32_t)sp < (uint32_t)STACK_LIMIT) || 
      ((uint32_t)sp > (uint32_t)STACK_START))
    {

        //printf ("limit: %x start : %x sp : %x\n", 
	  //       (uint32_t) STACK_LIMIT, (uint32_t) STACK_START, sp);
	
 	exit(ERROR);
    }
}

void check_valid_ptr (const void *vaddr)
{
  if (!is_user_vaddr(vaddr) || vaddr < USER_VADDR_BOTTOM || !page_lookup((void *)vaddr))
    {
      exit(ERROR);
    }
}

mapid_t mmap (int fd, void *addr)
{
  struct thread * t = thread_current();
  struct file *file, *mmap_file;
  size_t size, trailing_bytes, zero_bytes;
  off_t ofs = 0;
  int num_pages, i;
  void *lookup_addr = addr;
  if (!fd || (fd == 1) || !addr || ((uint32_t)addr&(uint32_t)0xFFF))
    {
       return ERROR;
    }   	       
  file = process_get_file (fd);
  if (!file)
    {
       return ERROR;
    }
  size = filesize (fd);
  if (!size)	
    {
       return ERROR;
    }
  num_pages  = i = size / PGSIZE;
  trailing_bytes = size % PGSIZE;
  zero_bytes     = PGSIZE - trailing_bytes;
  do 
    {
       if (page_lookup (lookup_addr))
	 {
	   return ERROR;
	 }
       lookup_addr += (uint32_t)0x1000;
    } while(i--);
  lookup_addr = addr;
  lock_acquire (&filesys_lock);
  mmap_file = file_reopen (file); 
  if (!mmap_file) 
    {
      lock_release (&filesys_lock);
      return ERROR;
    }
  t->mapid++;
  process_add_mapped_file (mmap_file, t->mapid);
  lock_release (&filesys_lock);
  while (num_pages--)
    {
       struct page *page = create_page(lookup_addr, MMAP_PAGE);
       page->mmap_file = mmap_file;
       page->mapid = t->mapid;
       page->ofs = ofs; 
       page->read_bytes = PGSIZE;
       insert_page(page);
       lookup_addr += (uint32_t)0x1000;
       ofs += PGSIZE;
    } 
  if (trailing_bytes)
    {
       struct page *page = create_page(lookup_addr, MMAP_PAGE);
       page->mmap_file = mmap_file;
       page->mapid = t->mapid;
       page->ofs = ofs; 
       page->read_bytes = trailing_bytes;
       insert_page(page);
    } 
  return t->mapid;
}

void munmap (mapid_t mapping)
{
  struct hash_iterator i;
  int count;
  struct thread * t = thread_current();
  hash_first (&i, &t->page_table);
  while (hash_next (&i))
    {
       struct page *page = hash_entry (hash_cur (&i), struct page, hash_elem);
       if (page->mapid == mapping)
	 {
	   count++;
	   if (pagedir_is_dirty (t->pagedir, page->addr))
	     {
		write_page_to_file (page); 
		set_page_delete (page);
		pagedir_clear_page (t->pagedir, page->addr);
	     }
	 }
    }
  delete_set_pages ();
  process_close_mapped_file (mapping);
}


