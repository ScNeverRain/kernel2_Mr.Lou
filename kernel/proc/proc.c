/******************************************************************************/
/* Important Spring 2016 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5f2e8d450c0c5851acd538befe33744efca0f1c4f9fb5f       */
/*         3c8feabc561a99e53d4d21951738da923cd1c7bbd11b30a1afb11172f80b       */
/*         984b1acfbbf8fae6ea57e0583d2610a618379293cb1de8e1e9d07e6287e8       */
/*         de7e82f3d48866aa2009b599e92c852f7dbf7a6e573f1c7228ca34b9f368       */
/*         faaef0c0fcf294cb                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void proc_init() {
	list_init(&_proc_list);
	proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
	KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid) {
	proc_t *p;
	list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
				{
					if (p->p_pid == pid) {
						return p;
					}
				}list_iterate_end();
	return NULL;
}

list_t *
proc_list() {
	return &_proc_list;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int _proc_getid() {
	proc_t *p;
	pid_t pid = next_pid;
	while (1) {
		failed:
		list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
					{
						if (p->p_pid == pid) {
							if ((pid = (pid + 1) % PROC_MAX_COUNT)
									== next_pid) {
								return -1;
							} else {
								goto failed;
							}
						}
					}list_iterate_end();
					next_pid = (pid + 1) % PROC_MAX_COUNT;
					return pid;
				}
			}

			/*
			 * The new process, although it isn't really running since it has no
			 * threads, should be in the PROC_RUNNING state.
			 *
			 * Don't forget to set proc_initproc when you create the init
			 * process. You will need to be able to reference the init process
			 * when reparenting processes to the init process.
			 */
proc_t *
proc_create(char *name) {
	/* TODO: kernel assignment 01 */
	pid_t pid = _proc_getid();

	KASSERT(PID_IDLE != pid || list_empty(&_proc_list));
	dbg(DBG_PRINT, "(GRADING1A 2.a)\n");

	proc_t *proc = slab_obj_alloc(proc_allocator);
	KASSERT(NULL != proc);
	memset(proc, 0, sizeof(proc_t));

	proc->p_pid = pid;

	size_t len = strlen(name);
	memcpy(proc->p_comm, name,
			(len >= PROC_NAME_LEN ? PROC_NAME_LEN - 1 : len) * sizeof(char));

	list_init(&(proc->p_threads));
	list_init(&(proc->p_children));

	proc->p_pproc = curproc;

	proc->p_status = 0;
	proc->p_state = PROC_RUNNING;

	sched_queue_init(&proc->p_wait);

	proc->p_pagedir = pt_create_pagedir();

	/*	add proc into the list of all process	*/
	list_insert_tail(proc_list(), &proc->p_list_link);

	if (NULL != proc->p_pproc) {
		list_insert_tail(&proc->p_pproc->p_children, &proc->p_child_link);
		dbg(DBG_PRINT, "(GRADING1C)\n");
	}
	/* VFS-related: */
	int i = 0;
	for(i = 0 ; i < NFILES ; i++){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		proc->p_files[i] = NULL;
	}
	if(curproc != NULL){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		proc->p_cwd = curproc->p_cwd;
		if(proc->p_cwd != NULL){
			dbg(DBG_PRINT,"(GRADING2B)\n");
			vref(proc->p_cwd);
		}
	}else{
		dbg(DBG_PRINT,"(GRADING2B)\n");
		proc->p_cwd = NULL;
	}
	/* VFS-related END */
	/* VM
	 proc->p_brk = NULL;
	 proc->p_start_brk = NULL;
	 proc->p_vmmap = NULL;
	 */
	KASSERT(PID_INIT != pid || PID_IDLE == curproc->p_pid);
	dbg(DBG_PRINT, "(GRADING1A 2.a)\n");

	if (proc->p_pid == PID_INIT || proc_initproc == NULL) {
		dbg(DBG_PRINT, "(GRADING1C)\n");
		proc_initproc = proc;
	}
	/*dbg(DBG_PROC, "A process created name: %s pid= %d\n", proc->p_comm,
	 proc->p_pid);*/
	dbg(DBG_PRINT, "(GRADING1C)\n");
	return proc;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void proc_cleanup(int status) {
	/* TODO: kernel assignment 01 proc_cleanup*/
	/*
	 * precondition
	 */
	KASSERT(NULL != proc_initproc);
	KASSERT(1 <= curproc->p_pid);
	KASSERT(NULL != curproc->p_pproc);
	dbg(DBG_PRINT, "(GRADING1A 2.b)\n");

	/* TODO: Closing all open files (VFS)*/
	int i = 0;
	dbg(DBG_PRINT,"(GRADING2B)\n");
	for(i = 0 ; i < NFILES ; i ++){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		if(curproc->p_files[i] != NULL){
			dbg(DBG_PRINT,"(GRADING2B)\n");
			do_close(i);
		}
	}
	/* TODO: Cleaning up VM mappings (VM)*/

	/*Waking up its parent if it is waiting*/
	/*
	 if (0 < curproc->p_pproc->p_wait.tq_size) {
	 dbg(DBG_PRINT, "(GRADING1C)\n");
	 */
	sched_wakeup_on(&curproc->p_pproc->p_wait);
	/*
	 }
	 */
	/*Reparenting any children to the init process*/
	if (curproc != proc_initproc) {
		dbg(DBG_PRINT, "(GRADING1C)\n");
		while (!list_empty(&curproc->p_children)) {
			dbg(DBG_PRINT, "(GRADING1C)\n");
			list_link_t *link = curproc->p_children.l_next;
			list_remove(link);
			list_insert_tail(&proc_initproc->p_children, link);
			((proc_t *) list_item(link, proc_t, p_child_link))->p_pproc =
					proc_initproc;
		}
	}
	dbg(DBG_PRINT, "(GRADING1C)\n");
	/*Setting its status and state appropriately*/
	curproc->p_state = PROC_DEAD;
	curproc->p_status = status;

	dbg(DBG_PRINT,"(GRADING2B)\n");
	if(curproc->p_cwd != NULL){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		vput(curproc->p_cwd);
		curproc->p_cwd = NULL;
	}
	/*
	 * postcondition
	 */
	KASSERT(NULL != curproc->p_pproc);
	dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void proc_kill(proc_t *p, int status) {
	/* TODO: kernel assignment 01 */
	KASSERT(NULL != p);
	/*
	 dbg(DBG_PROC, "Killing process pid=%d\n", p->p_pid);
	 */
	if (p->p_pid == curproc->p_pid) {
		dbg(DBG_PRINT, "(GRADING1C 9)\n");
		do_exit(status);
	}
	dbg(DBG_PRINT, "(GRADING1C)\n");
	list_t *list = &p->p_threads;
	kthread_t *cur = NULL;
	list_link_t *link = (list_link_t*) list->l_next;
	while (link != list) {
		dbg(DBG_PRINT, "(GRADING1C)\n");
		cur = list_item(link, kthread_t, kt_plink);
		link = (list_link_t*) link->l_next;
		kthread_cancel(cur, (void*) &status);

		sched_make_runnable(curthr);
		sched_switch();

	}
	dbg(DBG_PRINT, "(GRADING1C)\n");
	p->p_status = status;
	p->p_state = PROC_DEAD;
	sched_wakeup_on(&p->p_wait);
	/*
	 * TODO: proc_kill() finished?
	 */
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void proc_kill_all() {
	/* TODO: kernel assignment 01 */
	/*dbg(DBG_PROC, "\n\n\t\tI'm going to Kill all Processes!\n\n");*/
	proc_t *proc = NULL;
	dbg(DBG_PRINT, "(GRADING1C)\n");
	list_iterate_begin(&_proc_list, proc, proc_t, p_list_link)
				{
					dbg(DBG_PRINT, "(GRADING1C)\n");
					if (proc->p_pid != PID_IDLE
							&& proc->p_pproc->p_pid != PID_IDLE
							&& proc != curproc) {
						dbg(DBG_PRINT, "(GRADING1C)\n");
						proc_kill(proc, proc->p_status);
					}
				}list_iterate_end();
	dbg(DBG_PRINT, "(GRADING1C)\n");
	if (curproc->p_pid != PID_IDLE && curproc->p_pproc->p_pid != PID_IDLE) {
		dbg(DBG_PRINT, "(GRADING1C)\n");
		proc_kill(curproc, curproc->p_status);
	}
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void proc_thread_exited(void *retval) {
	/* TODO: kernel assignment 01 */
	dbg(DBG_PRINT, "(GRADING1C)\n");
	proc_cleanup(retval == NULL ? 0 : *(int*)retval);
	/*sched_wakeup_on(&curproc->p_pproc->p_wait);*/
	sched_switch();
}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t do_waitpid(pid_t pid, int options, int *status) {
	/* TODO: kernel assignment 01 proc_cleanup*/
	/* TODO: How to handle invalid parameter? */
	KASSERT(pid > 0 || pid == -1);
	KASSERT(options == 0);
	pid_t closed_pid = -ECHILD;
	if (-1 == pid) {
		if (list_empty(&curproc->p_children)) {
			dbg(DBG_PRINT, "(GRADING1C)\n");
			return -ECHILD;
		}
		while (1) {
			dbg(DBG_PRINT, "(GRADING1C)\n");
			proc_t *p = NULL;
			list_link_t *link = (list_link_t*) curproc->p_children.l_next;
			while ((list_t*) link != &curproc->p_children) {
				dbg(DBG_PRINT, "(GRADING1C)\n");
				p = list_item(link, proc_t, p_child_link);
				if (p->p_state == PROC_DEAD) {
					KASSERT(NULL != p);
					KASSERT(-1 == pid || p->p_pid == pid);
					dbg(DBG_PRINT, "(GRADING1A 2.c)\n");
					dbg(DBG_PRINT, "(GRADING1C)\n");
					kthread_t *thr = NULL;
					list_link_t *tlink = p->p_threads.l_next;

					while ((list_t*) tlink != &p->p_threads) {
						dbg(DBG_PRINT, "(GRADING1C)\n");
						thr = list_item(tlink, kthread_t, kt_plink);

						KASSERT(thr->kt_state == KT_EXITED);
						dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

						tlink = (list_link_t*) tlink->l_next;
						kthread_destroy(thr);
					}
					dbg(DBG_PRINT, "(GRADING1C)\n");
					closed_pid = p->p_pid;
					/*pid_t closed_pid = p->p_pid;*/
					*status = (p->p_status);
					list_remove(&p->p_list_link);
					list_remove(&p->p_child_link);

					KASSERT(NULL != p->p_pagedir);
					dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

					pt_destroy_pagedir(p->p_pagedir);
					slab_obj_free(proc_allocator, p);
					return closed_pid;
					/*return closed_id;*/
				}
				dbg(DBG_PRINT, "(GRADING1C)\n");
				link = (list_link_t*) link->l_next;
			}
			dbg(DBG_PRINT, "(GRADING1C)\n");
			sched_sleep_on(&curproc->p_wait);
		}
	} else if (pid > 0) {
		dbg(DBG_PRINT, "(GRADING1C)\n");
		proc_t *p = NULL;
		list_link_t *link = (list_link_t*) curproc->p_children.l_next;

		while (link != (list_t*) &curproc->p_children) {
			dbg(DBG_PRINT, "(GRADING1C)\n");
			p = list_item(link, proc_t, p_child_link);
			if (pid == p->p_pid) {
				dbg(DBG_PRINT, "(GRADING1C)\n");
				break;
			} else {
				dbg(DBG_PRINT, "(GRADING1C)\n");
				p = NULL;
			}
			link = (list_link_t*) link->l_next;
		}
		if (p == NULL) {
			dbg(DBG_PRINT, "(GRADING1C)\n");
			return -ECHILD;
		}
		KASSERT(-1 == pid || p->p_pid == pid);
		dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

		while (p->p_state != PROC_DEAD) {
			dbg(DBG_PRINT, "(GRADING1C)\n");
			sched_sleep_on(&curproc->p_wait);
		}
		dbg(DBG_PRINT, "(GRADING1C)\n");

		closed_pid = p->p_pid;
		/*pid_t closed_pid = p->p_pid;*/
		*status = (p->p_status);
		list_remove(&p->p_list_link);
		list_remove(&p->p_child_link);

		KASSERT(NULL != p->p_pagedir);
		dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

		pt_destroy_pagedir(p->p_pagedir);
		slab_obj_free(proc_allocator, p);
		/*return closed_pid;*/
	}
	dbg(DBG_PRINT, "(GRADING1C)\n");
	return closed_pid;
}

/*
 * Cancel all threads, join with them, and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void do_exit(int status) {
	/* TODO: kernel assignment 01 do_exit*/
	dbg(DBG_PRINT, "(GRADING1C)\n");

	curproc->p_state = PROC_DEAD;
	curproc->p_status = status;
	kthread_exit((void*) &status);
}

size_t proc_info(const void *arg, char *buf, size_t osize) {
	const proc_t *p = (proc_t *) arg;
	size_t size = osize;
	proc_t *child;

	KASSERT(NULL != p);
	KASSERT(NULL != buf);

	iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
	iprintf(&buf, &size, "name:         %s\n", p->p_comm);
	if (NULL != p->p_pproc) {
		iprintf(&buf, &size, "parent:       %i (%s)\n", p->p_pproc->p_pid,
				p->p_pproc->p_comm);
	} else {
		iprintf(&buf, &size, "parent:       -\n");
	}

#ifdef __MTP__
	int count = 0;
	kthread_t *kthr;
	list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
		++count;
	}list_iterate_end();
	iprintf(&buf, &size, "thread count: %i\n", count);
#endif

	if (list_empty(&p->p_children)) {
		iprintf(&buf, &size, "children:     -\n");
	} else {
		iprintf(&buf, &size, "children:\n");
	}
	list_iterate_begin(&p->p_children, child, proc_t, p_child_link)
				{
					iprintf(&buf, &size, "     %i (%s)\n", child->p_pid,
							child->p_comm);
				}list_iterate_end();

	iprintf(&buf, &size, "status:       %i\n", p->p_status);
	iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
	if (NULL != p->p_cwd) {
		char cwd[256];
		lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
		iprintf(&buf, &size, "cwd:          %-s\n", cwd);
	} else {
		iprintf(&buf, &size, "cwd:          -\n");
	}
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
	iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
	iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

	return size;
}

size_t proc_list_info(const void *arg, char *buf, size_t osize) {
	size_t size = osize;
	proc_t *p;

	KASSERT(NULL == arg);
	KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
	iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
	iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

	list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
				{
					char parent[64];
					if (NULL != p->p_pproc) {
						snprintf(parent, sizeof(parent), "%3i (%s)",
								p->p_pproc->p_pid, p->p_pproc->p_comm);
					} else {
						snprintf(parent, sizeof(parent), "  -");
					}

#if defined(__VFS__) && defined(__GETCWD__)
					if (NULL != p->p_cwd) {
						char cwd[256];
						lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
						iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
								p->p_pid, p->p_comm, parent, cwd);
					} else {
						iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
								p->p_pid, p->p_comm, parent);
					}
#else
					iprintf(&buf, &size, " %3i  %-13s %-s\n", p->p_pid,
							p->p_comm, parent);
#endif
				}list_iterate_end();
	return size;
}
