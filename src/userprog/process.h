#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
struct process_file {
  struct file *file;
  int fd;
  int mapid;
  struct list_elem elem;
};
int process_add_file (struct file *f);
int process_add_mapped_file (struct file *file, int mapid);
struct file* process_get_file (int fd);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_close_mapped_file (int mapid);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
