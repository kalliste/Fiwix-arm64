/*
 * fiwix/kernel/syscalls/fork.c
 *
 * Copyright 2018-2022, Jordi Sanfeliu. All rights reserved.
 * Distributed under the terms of the Fiwix License.
 */

#include <fiwix/asm.h>
#include <fiwix/kernel.h>
#include <fiwix/types.h>
#include <fiwix/segments.h>
#include <fiwix/sigcontext.h>
#include <fiwix/process.h>
#include <fiwix/sched.h>
#include <fiwix/sleep.h>
#include <fiwix/mm.h>
#include <fiwix/errno.h>
#include <fiwix/stdio.h>
#include <fiwix/string.h>

static void free_vma_table(struct proc *p)
{
	struct vma *vma, *tmp;

	vma = p->vma_table;
	while(vma) {
		tmp = vma;
		vma = vma->next;
		kfree((unsigned int)tmp);
	}
}

#ifdef CONFIG_SYSCALL_6TH_ARG
int sys_fork(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, struct sigcontext *sc)
#else
int sys_fork(int arg1, int arg2, int arg3, int arg4, int arg5, struct sigcontext *sc)
#endif /* CONFIG_SYSCALL_6TH_ARG */
{
	int count, pages;
	unsigned int n;
	unsigned int *child_pgdir;
	struct sigcontext *stack;
	struct proc *child, *p;
	struct vma *vma, *child_vma;
	__pid_t pid;

#ifdef __DEBUG__
	printk("(pid %d) sys_fork()\n", current->pid);
#endif /*__DEBUG__ */

	/* check the number of processes already allocated by this UID */
	count = 0;
	FOR_EACH_PROCESS(p) {
		if(p->uid == current->uid) {
			count++;
		}
		p = p->next;
	}
	if(count > current->rlim[RLIMIT_NPROC].rlim_cur) {
		printk("WARNING: %s(): RLIMIT_NPROC exceeded.\n", __FUNCTION__);
		return -EAGAIN;
	}

	if(!(pid = get_unused_pid())) {
		return -EAGAIN;
	}
	if(!(child = get_proc_free())) {
		return -EAGAIN;
	}

	/* 
	 * This memcpy() will overwrite the prev and next pointers, so that's
	 * the reason why proc_slot_init() is separated from get_proc_free().
	 */
	memcpy_b(child, current, sizeof(struct proc));

	proc_slot_init(child);
	child->pid = pid;
	sprintk(child->pidstr, "%d", child->pid);

	if(!(child_pgdir = (void *)kmalloc(PAGE_SIZE))) {
		release_proc(child);
		return -ENOMEM;
	}
	child->rss++;
	memcpy_b(child_pgdir, kpage_dir, PAGE_SIZE);
	child->tss.cr3 = V2P((unsigned int)child_pgdir);

	child->ppid = current;
	child->flags = 0;
	child->children = 0;
	child->cpu_count = (current->cpu_count >>= 1);
	child->start_time = CURRENT_TICKS;
	child->sleep_address = NULL;

	vma = current->vma_table;
	child->vma_table = NULL;
	while(vma) {
		if(!(child_vma = (struct vma *)kmalloc(sizeof(struct vma)))) {
			kfree((unsigned int)child_pgdir);
			free_vma_table(child);
			release_proc(child);
			return -ENOMEM;
		}
		*child_vma = *vma;
		child_vma->prev = child_vma->next = NULL;
		if(child_vma->inode) {
			child_vma->inode->count++;
		}
		if(!child->vma_table) {
			child->vma_table = child_vma;
		} else {
			child_vma->prev = child->vma_table->prev;
			child->vma_table->prev->next = child_vma;
		}
		child->vma_table->prev = child_vma;
		vma = vma->next;
	}

	child->sigpending = 0;
	child->sigexecuting = 0;
	memset_b(&child->sc, 0, sizeof(struct sigcontext));
	memset_b(&child->usage, 0, sizeof(struct rusage));
	memset_b(&child->cusage, 0, sizeof(struct rusage));
	child->it_real_interval = 0;
	child->it_real_value = 0;
	child->it_virt_interval = 0;
	child->it_virt_value = 0;
	child->it_prof_interval = 0;
	child->it_prof_value = 0;
#ifdef CONFIG_SYSVIPC
	current->semundo = NULL;
#endif /* CONFIG_SYSVIPC */


	if(!(child->tss.esp0 = kmalloc(PAGE_SIZE))) {
		kfree((unsigned int)child_pgdir);
		free_vma_table(child);
		release_proc(child);
		return -ENOMEM;
	}

	if(!(pages = clone_pages(child))) {
		printk("WARNING: %s(): not enough memory when cloning pages.\n", __FUNCTION__);
		free_page_tables(child);
		kfree((unsigned int)child_pgdir);
		free_vma_table(child);
		release_proc(child);
		return -ENOMEM;
	}
	child->rss += pages;
	invalidate_tlb();

	child->tss.esp0 += PAGE_SIZE - 4;
	child->rss++;
	child->tss.ss0 = KERNEL_DS;

	memcpy_b((unsigned int *)(child->tss.esp0 & PAGE_MASK), (void *)((unsigned int)(sc) & PAGE_MASK), PAGE_SIZE);
	stack = (struct sigcontext *)((child->tss.esp0 & PAGE_MASK) + ((unsigned int)(sc) & ~PAGE_MASK));

	child->tss.eip = (unsigned int)return_from_syscall;
	child->tss.esp = (unsigned int)stack;
	stack->eax = 0;		/* child returns 0 */

	/* increase file descriptors usage */
	for(n = 0; n < OPEN_MAX; n++) {
		if(current->fd[n]) {
			fd_table[current->fd[n]].count++;
		}
	}
	if(current->root) {
		current->root->count++;
	}
	if(current->pwd) {
		current->pwd->count++;
	}

	kstat.processes++;
	nr_processes++;
	current->children++;
	runnable(child);

	return child->pid;	/* parent returns child's PID */
}
