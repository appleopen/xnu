/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)init_main.c	8.16 (Berkeley) 5/14/95
 */

/* 
 *
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 */

#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/mount_internal.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/systm.h>
#include <sys/vnode_internal.h>
#include <sys/conf.h>
#include <sys/buf_internal.h>
#include <sys/clist.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/mman.h>

#include <bsm/audit_kernel.h>

#include <sys/malloc.h>
#include <sys/dkstat.h>
#include <sys/codesign.h>

#include <kern/startup.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/ast.h>

#include <mach/vm_param.h>

#include <vm/vm_map.h>
#include <vm/vm_kern.h>

#include <sys/ux_exception.h>	/* for ux_exception_port */

#include <sys/reboot.h>
#include <mach/exception_types.h>
#include <dev/busvar.h>			/* for pseudo_inits */
#include <sys/kdebug.h>

#include <mach/mach_types.h>
#include <mach/vm_prot.h>
#include <mach/semaphore.h>
#include <mach/sync_policy.h>
#include <kern/clock.h>
#include <mach/kern_return.h>
#include <mach/thread_act.h>		/* for thread_resume() */
#include <mach/task.h>			/* for task_set_exception_ports() */
#include <sys/ux_exception.h>		/* for ux_handler() */
#include <sys/ubc_internal.h>		/* for ubc_init() */
#include <sys/mcache.h>			/* for mcache_init() */
#include <sys/mbuf.h>			/* for mbinit() */
#include <sys/event.h>			/* for knote_init() */
#include <sys/aio_kern.h>		/* for aio_init() */
#include <sys/semaphore.h>		/* for psem_cache_init() */
#include <net/dlil.h>			/* for dlil_init() */
#include <net/kpi_protocol.h>		/* for proto_kpi_init() */
#include <sys/pipe.h>			/* for pipeinit() */
#include <sys/socketvar.h>		/* for socketinit() */
#include <sys/protosw.h>		/* for domaininit() */
#include <kern/sched_prim.h>		/* for thread_wakeup() */
#include <net/if_ether.h>		/* for ether_family_init() */
#include <vm/vm_protos.h>		/* for vnode_pager_bootstrap() */
#include <miscfs/devfs/devfsdefs.h>	/* for devfs_kernel_mount() */
#include <mach/host_priv.h>		/* for host_set_exception_ports() */
#include <kern/host.h>			/* for host_priv_self() */
#include <vm/vm_kern.h>			/* for kmem_suballoc() */
#include <sys/semaphore.h>		/* for psem_lock_init() */
#include <sys/msgbuf.h>			/* for log_setsize() */

#include <net/init.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#include <security/mac_internal.h>	/* mac_init_bsd() */
#include <security/mac_mach_internal.h>	/* mac_update_task_label() */
#endif

#include <machine/exec.h>

#if CONFIG_IMAGEBOOT
#include <sys/imageboot.h>
#endif

#include <pexpert/pexpert.h>

void * get_user_regs(thread_t);		/* XXX kludge for <machine/thread.h> */
void IOKitInitializeTime(void);		/* XXX */
void loopattach(void);			/* XXX */

char    copyright[] =
"Copyright (c) 1982, 1986, 1989, 1991, 1993\n\t"
"The Regents of the University of California. "
"All rights reserved.\n\n";

/* Components of the first process -- never freed. */
struct	proc proc0;
struct	session session0;
struct	pgrp pgrp0;
struct	filedesc filedesc0;
struct	plimit limit0;
struct	pstats pstats0;
struct	sigacts sigacts0;
proc_t kernproc;
proc_t initproc;

long tk_cancc;
long tk_nin;
long tk_nout;
long tk_rawcc;

int lock_trace = 0;
/* Global variables to make pstat happy. We do swapping differently */
int nswdev, nswap;
int nswapmap;
void *swapmap;
struct swdevt swdevt[1];

dev_t	rootdev;		/* device of the root */
dev_t	dumpdev;		/* device to take dumps on */
long	dumplo;			/* offset into dumpdev */
long	hostid;
char	hostname[MAXHOSTNAMELEN];
int		hostnamelen;
char	domainname[MAXDOMNAMELEN];
int		domainnamelen;
#if __i386__
struct exec_archhandler exec_archhandler_ppc = {
	.path = "/usr/libexec/oah/translate",
};
#else /* __i386__ */
struct exec_archhandler exec_archhandler_ppc;
#endif /* __i386__ */

char rootdevice[16]; 	/* hfs device names have at least 9 chars */

#if  KMEMSTATS
struct	kmemstats kmemstats[M_LAST];
#endif

int	lbolt;				/* awoken once a second */
struct	vnode *rootvp;
int boothowto = RB_DEBUG;

void lightning_bolt(void *);
extern kern_return_t IOFindBSDRoot(char *, dev_t *, u_int32_t *);
extern void IOSecureBSDRoot(const char * rootName);
extern kern_return_t IOKitBSDInit(void );
extern void kminit(void);
extern void klogwakeup(void);
extern void file_lock_init(void);
extern void kmeminit(void);
extern void bsd_bufferinit(void);

extern int srv;
extern int ncl;

#define BSD_SIMUL_EXECS       33 /* 32 , allow for rounding */
#define	BSD_PAGABLE_MAP_SIZE	(BSD_SIMUL_EXECS * (NCARGS + PAGE_SIZE))
vm_map_t	bsd_pageable_map;
vm_map_t	mb_map;
semaphore_t execve_semaphore;

int	cmask = CMASK;
extern int customnbuf;

void bsd_init(void) __attribute__((section("__TEXT, initcode")));
__private_extern__ void ubc_init(void )  __attribute__((section("__TEXT, initcode")));
void vfsinit(void) __attribute__((section("__TEXT, initcode")));
kern_return_t bsd_autoconf(void) __attribute__((section("__TEXT, initcode")));
void bsd_utaskbootstrap(void) __attribute__((section("__TEXT, initcode")));

static void parse_bsd_args(void);
extern task_t bsd_init_task;
extern char    init_task_failure_data[];
extern void time_zone_slock_init(void);
static void process_name(const char *, proc_t);

static void setconf(void);

funnel_t *kernel_flock;

#if SYSV_SHM
extern void sysv_shm_lock_init(void);
#endif
#if SYSV_SEM
extern void sysv_sem_lock_init(void);
#endif
#if SYSV_MSG
extern void sysv_msg_lock_init(void);
#endif
extern void pthread_init(void);

/* kmem access not enabled by default; can be changed with boot-args */
int setup_kmem = 0;

/* size of kernel trace buffer, disabled by default */
unsigned int new_nkdbufs = 0;

/* mach leak logging */
int log_leaks = 0;
int turn_on_log_leaks = 0;

extern void stackshot_lock_init(void);

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *  hand craft 1st user process
 */

/*
 *	Sets the name for the given task.
 */
static void
process_name(const char *s, proc_t p)
{
	size_t length = strlen(s);

	bcopy(s, p->p_comm,
		length >= sizeof(p->p_comm) ? sizeof(p->p_comm) :
			length + 1);
}

/* To allow these values to be patched, they're globals here */
#include <machine/vmparam.h>
struct rlimit vm_initial_limit_stack = { DFLSSIZ, MAXSSIZ - PAGE_SIZE };
struct rlimit vm_initial_limit_data = { DFLDSIZ, MAXDSIZ };
struct rlimit vm_initial_limit_core = { DFLCSIZ, MAXCSIZ };

extern thread_t	cloneproc(proc_t, int);
extern int 	(*mountroot)(void);
extern int 	netboot_mountroot(void); 	/* netboot.c */
extern int	netboot_setup(void);

lck_grp_t * proc_lck_grp;
lck_grp_attr_t * proc_lck_grp_attr;
lck_attr_t * proc_lck_attr;
lck_mtx_t * proc_list_mlock;
lck_mtx_t * proc_klist_mlock;

/* hook called after root is mounted XXX temporary hack */
void (*mountroot_post_hook)(void);

/*
 * This function is called very early on in the Mach startup, from the
 * function start_kernel_threads() in osfmk/kern/startup.c.  It's called
 * in the context of the current (startup) task using a call to the
 * function kernel_thread_create() to jump into start_kernel_threads().
 * Internally, kernel_thread_create() calls thread_create_internal(),
 * which calls uthread_alloc().  The function of uthread_alloc() is
 * normally to allocate a uthread structure, and fill out the uu_sigmask,
 * uu_context fields.  It skips filling these out in the case of the "task"
 * being "kernel_task", because the order of operation is inverted.  To
 * account for that, we need to manually fill in at least the contents
 * of the uu_context.vc_ucred field so that the uthread structure can be
 * used like any other.
 */
void
bsd_init(void)
{
	proc_t p;
	struct uthread *ut;
	unsigned int i;
#if __i386__
	int error;
#endif	
	struct vfs_context context;
	kern_return_t	ret;
	struct ucred temp_cred;

#define bsd_init_kprintf(x...) /* kprintf("bsd_init: " x) */

	kernel_flock = funnel_alloc(KERNEL_FUNNEL);
	if (kernel_flock == (funnel_t *)0 ) {
		panic("bsd_init: Failed to allocate kernel funnel");
	}
        
	printf(copyright);
	
	bsd_init_kprintf("calling kmeminit\n");
	kmeminit();
	
	bsd_init_kprintf("calling parse_bsd_args\n");
	parse_bsd_args();

	/* Initialize kauth subsystem before instancing the first credential */
	bsd_init_kprintf("calling kauth_init\n");
	kauth_init();

	/* Initialize process and pgrp structures. */
	bsd_init_kprintf("calling procinit\n");
	procinit();

	kernproc = &proc0;

	p = kernproc;

	/* kernel_task->proc = kernproc; */
	set_bsdtask_info(kernel_task,(void *)p);
	p->p_pid = 0;
	p->p_ppid = 0;

	/* give kernproc a name */
	bsd_init_kprintf("calling process_name\n");
	process_name("kernel_task", p);

	/* allocate proc lock group attribute and group */
	bsd_init_kprintf("calling lck_grp_attr_alloc_init\n");
	proc_lck_grp_attr= lck_grp_attr_alloc_init();
	
	proc_lck_grp = lck_grp_alloc_init("proc",  proc_lck_grp_attr);

	/* Allocate proc lock attribute */
	proc_lck_attr = lck_attr_alloc_init();
#if 0
#if __PROC_INTERNAL_DEBUG
	lck_attr_setdebug(proc_lck_attr);
#endif
#endif

	proc_list_mlock = lck_mtx_alloc_init(proc_lck_grp, proc_lck_attr);
	proc_klist_mlock = lck_mtx_alloc_init(proc_lck_grp, proc_lck_attr);
	lck_mtx_init(&p->p_mlock, proc_lck_grp, proc_lck_attr);
	lck_mtx_init(&p->p_fdmlock, proc_lck_grp, proc_lck_attr);
	lck_spin_init(&p->p_slock, proc_lck_grp, proc_lck_attr);

	if (current_task() != kernel_task)
		printf("bsd_init: We have a problem, "
				"current task is not kernel task\n");
	
	bsd_init_kprintf("calling get_bsdthread_info\n");
	ut = (uthread_t)get_bsdthread_info(current_thread());

#if CONFIG_MACF
	/*
	 * Initialize the MAC Framework
	 */
	mac_policy_initbsd();
	p->p_mac_enforce = 0;
#endif /* MAC */

	/*
	 * Create process 0.
	 */
	proc_list_lock();
	LIST_INSERT_HEAD(&allproc, p, p_list);
	p->p_pgrp = &pgrp0;
	LIST_INSERT_HEAD(PGRPHASH(0), &pgrp0, pg_hash);
	LIST_INIT(&pgrp0.pg_members);
	lck_mtx_init(&pgrp0.pg_mlock, proc_lck_grp, proc_lck_attr);
	/* There is no other bsd thread this point and is safe without pgrp lock */
	LIST_INSERT_HEAD(&pgrp0.pg_members, p, p_pglist);
	p->p_listflag |= P_LIST_INPGRP;
	p->p_pgrpid = 0;

	pgrp0.pg_session = &session0;
	pgrp0.pg_membercnt = 1;

	session0.s_count = 1;
	session0.s_leader = p;
	session0.s_listflags = 0;
	lck_mtx_init(&session0.s_mlock, proc_lck_grp, proc_lck_attr);
	LIST_INSERT_HEAD(SESSHASH(0), &session0, s_hash);
	proc_list_unlock();

#if CONFIG_LCTX
	p->p_lctx = NULL;
#endif

	p->task = kernel_task;
	
	p->p_stat = SRUN;
	p->p_flag = P_SYSTEM;
	p->p_nice = NZERO;
	p->p_pptr = p;

	TAILQ_INIT(&p->p_uthlist);
	TAILQ_INSERT_TAIL(&p->p_uthlist, ut, uu_list);
	
	p->sigwait = FALSE;
	p->sigwait_thread = THREAD_NULL;
	p->exit_thread = THREAD_NULL;
	p->p_csflags = CS_VALID;

	/*
	 * Create credential.  This also Initializes the audit information.
	 * XXX It is not clear what the initial values should be for audit ID,
	 * XXX session ID, etc..
	 */
	bsd_init_kprintf("calling bzero\n");
	bzero(&temp_cred, sizeof(temp_cred));
	temp_cred.cr_ngroups = 1;

	bsd_init_kprintf("calling kauth_cred_create\n");
	p->p_ucred = kauth_cred_create(&temp_cred); 

	/* give the (already exisiting) initial thread a reference on it */
	bsd_init_kprintf("calling kauth_cred_ref\n");
	kauth_cred_ref(p->p_ucred);
	ut->uu_context.vc_ucred = p->p_ucred;
	ut->uu_context.vc_thread = current_thread();

	TAILQ_INIT(&p->aio_activeq);
	TAILQ_INIT(&p->aio_doneq);
	p->aio_active_count = 0;
	p->aio_done_count = 0;

	bsd_init_kprintf("calling file_lock_init\n");
	file_lock_init();

#if CONFIG_MACF
	mac_cred_label_associate_kernel(p->p_ucred);
	mac_task_label_update_cred (p->p_ucred, (struct task *) p->task);
#endif

	/* Create the file descriptor table. */
	filedesc0.fd_refcnt = 1+1;	/* +1 so shutdown will not _FREE_ZONE */
	p->p_fd = &filedesc0;
	filedesc0.fd_cmask = cmask;
	filedesc0.fd_knlistsize = -1;
	filedesc0.fd_knlist = NULL;
	filedesc0.fd_knhash = NULL;
	filedesc0.fd_knhashmask = 0;

	/* Create the limits structures. */
	p->p_limit = &limit0;
	for (i = 0; i < sizeof(p->p_rlimit)/sizeof(p->p_rlimit[0]); i++)
		limit0.pl_rlimit[i].rlim_cur = 
			limit0.pl_rlimit[i].rlim_max = RLIM_INFINITY;
	limit0.pl_rlimit[RLIMIT_NOFILE].rlim_cur = NOFILE;
	limit0.pl_rlimit[RLIMIT_NPROC].rlim_cur = maxprocperuid;
	limit0.pl_rlimit[RLIMIT_NPROC].rlim_max = maxproc;
	limit0.pl_rlimit[RLIMIT_STACK] = vm_initial_limit_stack;
	limit0.pl_rlimit[RLIMIT_DATA] = vm_initial_limit_data;
	limit0.pl_rlimit[RLIMIT_CORE] = vm_initial_limit_core;
	limit0.pl_refcnt = 1;

	p->p_stats = &pstats0;
	p->p_sigacts = &sigacts0;

	/*
	 * Charge root for two  processes: init and mach_init.
	 */
	bsd_init_kprintf("calling chgproccnt\n");
	(void)chgproccnt(0, 1);

	/*
	 *	Allocate a kernel submap for pageable memory
	 *	for temporary copying (execve()).
	 */
	{
		vm_offset_t	minimum;

		bsd_init_kprintf("calling kmem_suballoc\n");
		ret = kmem_suballoc(kernel_map,
				&minimum,
				(vm_size_t)BSD_PAGABLE_MAP_SIZE,
				TRUE,
				VM_FLAGS_ANYWHERE,
				&bsd_pageable_map);
		if (ret != KERN_SUCCESS) 
			panic("bsd_init: Failed to allocate bsd pageable map");
	}

	/*
	 * Initialize buffers and hash links for buffers
	 *
	 * SIDE EFFECT: Starts a thread for bcleanbuf_thread(), so must
	 *		happen after a credential has been associated with
	 *		the kernel task.
	 */
	bsd_init_kprintf("calling bsd_bufferinit\n");
	bsd_bufferinit();

	/* Initialize the execve() semaphore */
	bsd_init_kprintf("calling semaphore_create\n");
	ret = semaphore_create(kernel_task, &execve_semaphore,
			       SYNC_POLICY_FIFO, BSD_SIMUL_EXECS -1);
	if (ret != KERN_SUCCESS)
		panic("bsd_init: Failed to create execve semaphore");

	/*
	 * Initialize the calendar.
	 */
	bsd_init_kprintf("calling IOKitInitializeTime\n");
	IOKitInitializeTime();

	if (turn_on_log_leaks && !new_nkdbufs)
		new_nkdbufs = 200000;
	start_kern_tracing(new_nkdbufs);
	if (turn_on_log_leaks)
		log_leaks = 1;

	bsd_init_kprintf("calling ubc_init\n");
	ubc_init();

	/* Initialize the file systems. */
	bsd_init_kprintf("calling vfsinit\n");
	vfsinit();

#if SOCKETS
	/* Initialize per-CPU cache allocator */
	mcache_init();

	/* Initialize mbuf's. */
	bsd_init_kprintf("calling mbinit\n");
	mbinit();
#endif /* SOCKETS */

	/*
	 * Initializes security event auditing.
	 * XXX: Should/could this occur later?
	 */
#if AUDIT
	bsd_init_kprintf("calling audit_init\n");
 	audit_init();  
#endif

	/* Initialize kqueues */
	bsd_init_kprintf("calling knote_init\n");
	knote_init();

#if CONFIG_EMBEDDED
	/* Initialize kernel memory status notifications */
	bsd_init_kprintf("calling kern_memorystatus_init\n");
	kern_memorystatus_init();
#endif

	/* Initialize for async IO */
	bsd_init_kprintf("calling aio_init\n");
	aio_init();

	/* Initialize pipes */
	bsd_init_kprintf("calling pipeinit\n");
	pipeinit();

	/* Initialize SysV shm subsystem locks; the subsystem proper is
	 * initialized through a sysctl.
	 */
#if SYSV_SHM
	bsd_init_kprintf("calling sysv_shm_lock_init\n");
	sysv_shm_lock_init();
#endif
#if SYSV_SEM
	bsd_init_kprintf("calling sysv_sem_lock_init\n");
	sysv_sem_lock_init();
#endif
#if SYSV_MSG
	bsd_init_kprintf("sysv_msg_lock_init\n");
	sysv_msg_lock_init();
#endif
	bsd_init_kprintf("calling pshm_lock_init\n");
	pshm_lock_init();
	bsd_init_kprintf("calling psem_lock_init\n");
	psem_lock_init();

	pthread_init();
	/* POSIX Shm and Sem */
	bsd_init_kprintf("calling pshm_cache_init\n");
	pshm_cache_init();
	bsd_init_kprintf("calling psem_cache_init\n");
	psem_cache_init();
	bsd_init_kprintf("calling time_zone_slock_init\n");
	time_zone_slock_init();

	/* Stack snapshot facility lock */
	stackshot_lock_init();
	/*
	 * Initialize protocols.  Block reception of incoming packets
	 * until everything is ready.
	 */
	bsd_init_kprintf("calling sysctl_register_fixed\n");
	sysctl_register_fixed(); 
	bsd_init_kprintf("calling sysctl_mib_init\n");
	sysctl_mib_init();
#if NETWORKING
	bsd_init_kprintf("calling dlil_init\n");
	dlil_init();
	bsd_init_kprintf("calling proto_kpi_init\n");
	proto_kpi_init();
#endif /* NETWORKING */
#if SOCKETS
	bsd_init_kprintf("calling socketinit\n");
	socketinit();
	bsd_init_kprintf("calling domaininit\n");
	domaininit();
#endif /* SOCKETS */

	p->p_fd->fd_cdir = NULL;
	p->p_fd->fd_rdir = NULL;

#ifdef GPROF
	/* Initialize kernel profiling. */
	kmstartup();
#endif

	/* kick off timeout driven events by calling first time */
	thread_wakeup(&lbolt);
	timeout(lightning_bolt, 0, hz);

	bsd_init_kprintf("calling bsd_autoconf\n");
	bsd_autoconf();

#if CONFIG_DTRACE
	extern void dtrace_postinit(void);
	dtrace_postinit();
#endif

	/*
	 * We attach the loopback interface *way* down here to ensure
	 * it happens after autoconf(), otherwise it becomes the
	 * "primary" interface.
	 */
#include <loop.h>
#if NLOOP > 0
	bsd_init_kprintf("calling loopattach\n");
	loopattach();			/* XXX */
#endif
        
#if NETHER > 0
	/* Register the built-in dlil ethernet interface family */
	bsd_init_kprintf("calling ether_family_init\n");
	ether_family_init();
#endif /* ETHER */

#if NETWORKING
	/* Call any kext code that wants to run just after network init */
	bsd_init_kprintf("calling net_init_run\n");
	net_init_run();
#endif /* NETWORKING */

	bsd_init_kprintf("calling vnode_pager_bootstrap\n");
	vnode_pager_bootstrap();
#if 0
	/* XXX Hack for early debug stop */
	printf("\nabout to sleep for 10 seconds\n");
	IOSleep( 10 * 1000 );
	/* Debugger("hello"); */
#endif

	bsd_init_kprintf("calling inittodr\n");
	inittodr(0);

#if CONFIG_EMBEDDED
	{
		/* print out early VM statistics */
		kern_return_t kr1;
		vm_statistics_data_t stat;
		mach_msg_type_number_t count;

		count = HOST_VM_INFO_COUNT;
		kr1 = host_statistics(host_self(),
				      HOST_VM_INFO,
				      (host_info_t)&stat,
				      &count);
		kprintf("Mach Virtual Memory Statistics (page size of 4096) bytes\n"
			"Pages free:\t\t\t%u.\n"
			"Pages active:\t\t\t%u.\n"
			"Pages inactive:\t\t\t%u.\n"
			"Pages wired down:\t\t%u.\n"
			"\"Translation faults\":\t\t%u.\n"
			"Pages copy-on-write:\t\t%u.\n"
			"Pages zero filled:\t\t%u.\n"
			"Pages reactivated:\t\t%u.\n"
			"Pageins:\t\t\t%u.\n"
			"Pageouts:\t\t\t\%u.\n"
			"Object cache: %u hits of %u lookups (%d%% hit rate)\n",

			stat.free_count,
			stat.active_count,
			stat.inactive_count,
			stat.wire_count,
			stat.faults,
			stat.cow_faults,
			stat.zero_fill_count,
			stat.reactivations,
			stat.pageins,
			stat.pageouts,
			stat.hits,
			stat.lookups,
			(stat.hits == 0) ? 100 :
			                   ((stat.lookups * 100) / stat.hits));
	}
#endif /* CONFIG_EMBEDDED */
	
	/* Mount the root file system. */
	while( TRUE) {
		int err;

		bsd_init_kprintf("calling setconf\n");
		setconf();

		bsd_init_kprintf("vfs_mountroot\n");
		if (0 == (err = vfs_mountroot()))
			break;
		rootdevice[0] = '\0';
#if NFSCLIENT
		if (mountroot == netboot_mountroot) {
			printf("bsd_init: netboot_mountroot failed,"
			       " errno = %d\n", err);
			panic("bsd_init: failed to mount network root: %s", PE_boot_args());
		}
#endif
		printf("cannot mount root, errno = %d\n", err);
		boothowto |= RB_ASKNAME;
	}

	IOSecureBSDRoot(rootdevice);

	context.vc_thread = current_thread();
	context.vc_ucred = p->p_ucred;
	mountlist.tqh_first->mnt_flag |= MNT_ROOTFS;

	bsd_init_kprintf("calling VFS_ROOT\n");
	/* Get the vnode for '/'.  Set fdp->fd_fd.fd_cdir to reference it. */
	if (VFS_ROOT(mountlist.tqh_first, &rootvnode, &context))
		panic("bsd_init: cannot find root vnode: %s", PE_boot_args());
	rootvnode->v_flag |= VROOT;
	(void)vnode_ref(rootvnode);
	(void)vnode_put(rootvnode);
	filedesc0.fd_cdir = rootvnode;

#if NFSCLIENT
	if (mountroot == netboot_mountroot) {
		int err;
		/* post mount setup */
		if ((err = netboot_setup()) != 0) {
			panic("bsd_init: NetBoot could not find root, %d: %s", err, PE_boot_args());
		}
	}
#endif
	

#if CONFIG_IMAGEBOOT
	/*
	 * See if a system disk image is present. If so, mount it and
	 * switch the root vnode to point to it
	 */ 
  
	if(imageboot_needed()) {
		int err;

		/* An image was found */
		if((err = imageboot_setup())) {
			/*
			 * this is not fatal. Keep trying to root
			 * off the original media
			 */
			printf("%s: imageboot could not find root, %d\n",
				__FUNCTION__, err);
		}
	}
#endif /* CONFIG_IMAGEBOOT */
  
	microtime(&p->p_stats->p_start);	/* for compat sake */
	microtime(&p->p_start);
	p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;

#if DEVFS
	{
	    char mounthere[] = "/dev";	/* !const because of internal casting */

	    bsd_init_kprintf("calling devfs_kernel_mount\n");
	    devfs_kernel_mount(mounthere);
	}
#endif /* DEVFS */
	
	/* Initialize signal state for process 0. */
	bsd_init_kprintf("calling siginit\n");
	siginit(p);

	bsd_init_kprintf("calling bsd_utaskbootstrap\n");
	bsd_utaskbootstrap();

#if __i386__
	/* this should be done after the root filesystem is mounted */
	error = set_archhandler(p, CPU_TYPE_POWERPC);
	if (error) /* XXX make more generic */
		exec_archhandler_ppc.path[0] = 0;
#endif	

	bsd_init_kprintf("calling mountroot_post_hook\n");

	/* invoke post-root-mount hook */
	if (mountroot_post_hook != NULL)
		mountroot_post_hook();

#if 0 /* not yet */
	IOKitJettisonKLD();
	consider_zone_gc();
#endif
	
	bsd_init_kprintf("done\n");
}

/* Called with kernel funnel held */
void
bsdinit_task(void)
{
	proc_t p = current_proc();
	struct uthread *ut;
	thread_t thread;

	process_name("init", p);

	ux_handler_init();

	thread = current_thread();
	(void) host_set_exception_ports(host_priv_self(),
					EXC_MASK_ALL & ~(EXC_MASK_SYSCALL |
							 EXC_MASK_MACH_SYSCALL |
							 EXC_MASK_RPC_ALERT |
							 EXC_MASK_CRASH),
					(mach_port_t)ux_exception_port,
					EXCEPTION_DEFAULT| MACH_EXCEPTION_CODES,
					0);

	ut = (uthread_t)get_bsdthread_info(thread);

	bsd_init_task = get_threadtask(thread);
	init_task_failure_data[0] = 0;

#if CONFIG_MACF
	mac_cred_label_associate_user(p->p_ucred);
	mac_task_label_update_cred (p->p_ucred, (struct task *) p->task);
#endif
	load_init_program(p);
	lock_trace = 1;
}

void
lightning_bolt(__unused void *dummy)
{			
	boolean_t 	funnel_state;

	funnel_state = thread_funnel_set(kernel_flock, TRUE);

	thread_wakeup(&lbolt);
	timeout(lightning_bolt,0,hz);
	klogwakeup();

	(void) thread_funnel_set(kernel_flock, FALSE);
}

kern_return_t
bsd_autoconf(void)
{
	kprintf("bsd_autoconf: calling kminit\n");
	kminit();

	/* 
	 * Early startup for bsd pseudodevices.
	 */
	{
	    struct pseudo_init *pi;
	
	    for (pi = pseudo_inits; pi->ps_func; pi++)
		(*pi->ps_func) (pi->ps_count);
	}

	return( IOKitBSDInit());
}


#include <sys/disklabel.h>  /* for MAXPARTITIONS */

static void
setconf(void)
{	
	u_int32_t	flags;
	kern_return_t	err;

	/*
	 * calls into IOKit can generate networking registrations
	 * which needs to be under network funnel. Right thing to do
	 * here is to drop the funnel alltogether and regrab it afterwards
	 */
	err = IOFindBSDRoot( rootdevice, &rootdev, &flags );
	if( err) {
		printf("setconf: IOFindBSDRoot returned an error (%d);"
			"setting rootdevice to 'sd0a'.\n", err); /* XXX DEBUG TEMP */
		rootdev = makedev( 6, 0 );
		strlcpy(rootdevice, "sd0a", sizeof(rootdevice));
		flags = 0;
	}

#if NFSCLIENT
	if( flags & 1 ) {
		/* network device */
		mountroot = netboot_mountroot;
	} else {
#endif
		/* otherwise have vfs determine root filesystem */
		mountroot = NULL;
#if NFSCLIENT
	}
#endif

}

void
bsd_utaskbootstrap(void)
{
	thread_t thread;
	struct uthread *ut;

	thread = cloneproc(kernproc, 0);
	/* Hold the reference as it will be dropped during shutdown */
	initproc = proc_find(1);				
#if __PROC_INTERNAL_DEBUG
	if (initproc == PROC_NULL)
		panic("bsd_utaskbootstrap: initproc not set\n");
#endif
	/* Set the launch time for init */
	microtime(&initproc->p_start);
	microtime(&initproc->p_stats->p_start);	/* for compat sake */
	

	ut = (struct uthread *)get_bsdthread_info(thread);
	ut->uu_sigmask = 0;
	act_set_astbsd(thread);
	(void) thread_resume(thread);
}

static void
parse_bsd_args(void)
{
	char namep[16];
	int msgbuf;

	if (PE_parse_boot_arg("-s", namep))
		boothowto |= RB_SINGLE;

	if (PE_parse_boot_arg("-b", namep))
		boothowto |= RB_NOBOOTRC;

	if (PE_parse_boot_arg("-x", namep)) /* safe boot */
		boothowto |= RB_SAFEBOOT;

	if (PE_parse_boot_arg("-l", namep)) /* leaks logging */
		turn_on_log_leaks = 1;

	PE_parse_boot_arg("srv", &srv);
	PE_parse_boot_arg("ncl", &ncl);
	if (PE_parse_boot_arg("nbuf", &max_nbuf_headers)) {
		customnbuf = 1;
	}
#if !defined(SECURE_KERNEL)
	PE_parse_boot_arg("kmem", &setup_kmem);
#endif
	PE_parse_boot_arg("trace", &new_nkdbufs);

	if (PE_parse_boot_arg("msgbuf", &msgbuf)) {
		log_setsize(msgbuf);
	}
}

#if !NFSCLIENT
int 
netboot_root(void)
{
	return(0);
}
#endif
