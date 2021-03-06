/**
 * @file IxOsalOsThread.c (linux)
 *
 * @brief OS-specific thread implementation.
 * 
 * 
 * @par
 * IXP400 SW Release version 2.1
 * 
 * -- Copyright Notice --
 * 
 * @par
 * Copyright (c) 2001-2005, Intel Corporation.
 * All rights reserved.
 * 
 * @par
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * 
 * @par
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * 
 * @par
 * -- End of Copyright Notice --
 */

#include "IxOsal.h"

#include <linux/sched.h>

#ifdef IX_OSAL_OS_LINUX_VERSION_2_6
#include <linux/list.h>
#include <linux/kthread.h>
#endif /* IX_OSAL_OS_LINUX_VERSION_2_6 */

struct IxOsalOsThreadData
{
    IxOsalVoidFnVoidPtr entryPoint;
    void *arg;
};

#ifdef IX_OSAL_OS_LINUX_VERSION_2_6
struct IxOsalOsThreadInfo
{
    struct IxOsalOsThreadData data;
    IxOsalThread ptid;
    struct list_head list;
};

PRIVATE LIST_HEAD(threadList);
#else 
struct IxOsalOsThreadData thread_data;
struct task_struct *kill_task = NULL;
#endif /* IX_OSAL_OS_LINUX_VERSION_2_6 */

/* declaring mutex */
DECLARE_MUTEX (IxOsalThreadMutex);
DECLARE_MUTEX (IxOsalThreadStopMutex);

/* Thread attribute is ignored */
#ifdef IX_OSAL_OS_LINUX_VERSION_2_6

PUBLIC IX_OSAL_INLINE BOOL
ixOsalThreadStopCheck()
{
    return (kthread_should_stop());
}

static int thread_internal(void *data)
{
    IxOsalVoidFnVoidPtr entryPoint = NULL;
    struct IxOsalOsThreadInfo *threadInfo;

    /*
     * Search for the task entry point from the thread info list
     */
    down (&IxOsalThreadMutex);

    list_for_each_entry(threadInfo, &threadList, list)
    {
    	/*
	 * Check if the thread handler in the node matches this thread handler
	 */
    	if (current == (struct task_struct*)(threadInfo->ptid))
	{
	    entryPoint = threadInfo->data.entryPoint;

	    /*
	     * Record found. Delete and free the threadInfo node, then break the
	     * loop.
	     */
	    list_del(&threadInfo->list);

	    ixOsalMemFree(threadInfo);
	    break;
	}
    }

    up (&IxOsalThreadMutex);

    if (entryPoint)
    {
	entryPoint(data);
    }

    return 0;
}

PUBLIC IX_STATUS
ixOsalThreadCreate (IxOsalThread * ptrTid,
    IxOsalThreadAttr * threadAttr, IxOsalVoidFnVoidPtr entryPoint, void *arg)
{
    struct IxOsalOsThreadInfo *threadInfo;

    threadInfo = (struct IxOsalOsThreadInfo*) ixOsalMemAlloc (
    			sizeof(struct IxOsalOsThreadInfo));

    if (unlikely(NULL == threadInfo))
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadCreate(): Failed to allocate memory for threadInfo\n",
            0, 0, 0, 0, 0, 0);

	return IX_FAIL;
    }
    	
    threadInfo->data.entryPoint = entryPoint;

    /* 
     * Create the thread with the given thread name, but do not start it
     */
    *ptrTid = kthread_create(thread_internal, arg, "%s", 
    		(NULL != threadAttr && NULL != threadAttr->name)
		? threadAttr->name: "OSAL");

    if (unlikely(NULL == *ptrTid))
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadCreate(): fail to create kthread \n",
            0, 0, 0, 0, 0, 0);

	/*
	 * Thread create failed, so free the threadInfo node allocated earlier
	 */
	ixOsalMemFree(threadInfo);
    	return IX_FAIL;
    }
	
    /*
     * Save thread handler of the created thread in the threadInfo node
     */
    threadInfo->ptid = *ptrTid;

    down (&IxOsalThreadMutex);

    /* 
     * We assumes that threads will be started according to the creation
     * order, hence we add latest thread on the tail.
     */
    list_add_tail(&threadInfo->list, &threadList);

    up(&IxOsalThreadMutex);

    return IX_SUCCESS;
}

/* 
 * Start the thread
 */
PUBLIC IX_STATUS
ixOsalThreadStart (IxOsalThread * tId)
{
    if (unlikely(NULL == *tId))
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadStart(): Invalid Thread ID!\n",
            0, 0, 0, 0, 0, 0);

	return IX_FAIL;
    }

    wake_up_process(*tId);

    return IX_SUCCESS;
}
	
/*
 * Kill the kernel thread. This shall not be used if the thread function
 * implements do_exit()
 */
PUBLIC IX_STATUS
ixOsalThreadKill (IxOsalThread * tid)
{
    struct task_struct *task = (struct task_struct*)*tid;

    /* Can't kill defunc thread */
    if (EXIT_DEAD == task->exit_state || EXIT_ZOMBIE == task->exit_state)
    	return IX_FAIL;

    if (-EINTR == kthread_stop(task))
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadKill(): Failed to kill thread\n",
            0, 0, 0, 0, 0, 0);

	return IX_FAIL;
    }

    return IX_SUCCESS;
}

#else

inline BOOL
ixOsalThreadStopCheck()
{
    if (current == kill_task)
    {
    	kill_task = NULL;
    	up(&IxOsalThreadStopMutex);
	return TRUE;
    }

    return FALSE;
}
	

static int
thread_internal (void *unused)
{
    IxOsalVoidFnVoidPtr entryPoint = thread_data.entryPoint;
    void *arg = thread_data.arg;
    static int seq = 0;

    daemonize();
    reparent_to_init ();
    exit_files (current);

    snprintf(current->comm, sizeof(current->comm), "IxOsal %d", ++seq);

    up (&IxOsalThreadMutex);

    entryPoint (arg);
    return 0;
}

/* Thread attribute is ignored */
PUBLIC IX_STATUS
ixOsalThreadCreate (IxOsalThread * ptrTid,
    IxOsalThreadAttr * threadAttr, IxOsalVoidFnVoidPtr entryPoint, void *arg)
{
    down (&IxOsalThreadMutex);
    thread_data.entryPoint = entryPoint;
    thread_data.arg = arg;

    /*
     * kernel_thread takes: int (*fn)(void *)  as the first input.
     */
    *ptrTid = kernel_thread (thread_internal, NULL, CLONE_SIGHAND);

    if (*ptrTid < 0)
    {
        up (&IxOsalThreadMutex);
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadCreate(): fail to generate thread \n",
            0, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }

    return IX_SUCCESS;

}

/* 
 * Start thread after given its thread handle
 */
PUBLIC IX_STATUS
ixOsalThreadStart (IxOsalThread * tId)
{
    /* Thread already started upon creation */
    return IX_SUCCESS;
}


PUBLIC IX_STATUS
ixOsalThreadKill (IxOsalThread * tid)
{
    down(&IxOsalThreadStopMutex);
    kill_task = find_task_by_pid(*tid);

    if (kill_task)
    {
	wake_up_process(kill_task);

	return IX_SUCCESS;
    }

    ixOsalLog (IX_OSAL_LOG_LVL_ERROR, IX_OSAL_LOG_DEV_STDOUT,
	"ixOsalThreadKill: Task %d was dead\n", *tid, 0, 0, 0, 0, 0);

    /* Kill failed, remove the mutex */
    up(&IxOsalThreadStopMutex);
    return IX_FAIL;
}

#endif /* IX_OSAL_OS_LINUX_VERSION_2_6 */

PUBLIC void
ixOsalThreadExit (void)
{
    ixOsalLog (IX_OSAL_LOG_LVL_MESSAGE,
        IX_OSAL_LOG_DEV_STDOUT,
        "ixOsalThreadExit(): not implemented \n", 0, 0, 0, 0, 0, 0);
}

PUBLIC IX_STATUS
ixOsalThreadPrioritySet (IxOsalOsThread * tid, UINT32 priority)
{
    return IX_SUCCESS;
}

PUBLIC IX_STATUS
ixOsalThreadSuspend (IxOsalThread * tId)
{
    ixOsalLog (IX_OSAL_LOG_LVL_WARNING,
        IX_OSAL_LOG_DEV_STDOUT,
        "ixOsalThreadSuspend(): not implemented in linux \n",
        0, 0, 0, 0, 0, 0);
    return IX_SUCCESS;

}

PUBLIC IX_STATUS
ixOsalThreadResume (IxOsalThread * tId)
{
    ixOsalLog (IX_OSAL_LOG_LVL_WARNING,
        IX_OSAL_LOG_DEV_STDOUT,
        "ixOsalThreadResume(): not implemented in linux \n",
        0, 0, 0, 0, 0, 0);
    return IX_SUCCESS;

}
