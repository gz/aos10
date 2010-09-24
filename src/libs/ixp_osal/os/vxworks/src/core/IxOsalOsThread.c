/**
 * @file IxOsalOsThread.c
 *
 * @brief vxWorks-specific implementation for thread.
 *
 * Design Notes:
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

/* INCLUDES */

#include <taskLib.h>

#include "IxOsal.h"

/*
 * Two defines only used in this file.
 */

/* Default thread attribute */
IxOsalThreadAttr ixOsalDefaultThreadAttr;

/**********************************************
 *  Public Functions
 *********************************************/
IX_STATUS
ixOsalThreadCreate (IxOsalThread * ptrTid,
    IxOsalThreadAttr * threadAttr,
    IxOsalVoidFnVoidPtr startRoutine, void *arg)
{
    int retVxWorksVal;
    WIND_TCB *pNewTcb;
    char *pTaskMem;
    char *pStackBase;
    UINT32 stackSize;

    /*
     * Some error checking 
     */
    if (startRoutine == NULL)
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadCreate(): NULL startRoutine \n", 0, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }

    if (threadAttr == NULL)
    {
        ixOsalLog (IX_OSAL_LOG_LVL_WARNING,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadCreate(): NULL threadAttr, use default value \n",
            0, 0, 0, 0, 0, 0);

        threadAttr = &ixOsalDefaultThreadAttr;
        threadAttr->stackSize = IX_OSAL_THREAD_DEFAULT_STACK_SIZE;
        threadAttr->priority = IX_OSAL_DEFAULT_THREAD_PRIORITY;
				threadAttr->name = "tUnknown";
    }
    else /* threadAttr is not NULL */
    {
        /*
         * If user provide stack size 0, set it to default 
         */
        if (0 == threadAttr->stackSize)
        {
            threadAttr->stackSize = IX_OSAL_THREAD_DEFAULT_STACK_SIZE;
        }

        if ((threadAttr->stackSize > IX_OSAL_THREAD_MAX_STACK_SIZE))
        {
            ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
                IX_OSAL_LOG_DEV_STDOUT,
                "ixOsalThreadCreate(): max stack size =  %d \n",
                IX_OSAL_THREAD_MAX_STACK_SIZE, 0, 0, 0, 0, 0);
            return IX_FAIL;
        }

        if ((threadAttr->priority > IX_OSAL_MAX_THREAD_PRIORITY))
        {
            ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
                IX_OSAL_LOG_DEV_STDOUT,
                "ixOsalThreadCreate(): threadAttr priority is within [0, %d]  \n",
                IX_OSAL_MAX_THREAD_PRIORITY, 0, 0, 0, 0, 0);
            return IX_FAIL;
        }

	if (0 == threadAttr->name)
	{
	    threadAttr->name = "tUnknown";
	}

    }
    /* Total Stack Size */
    stackSize = threadAttr->stackSize;    
    
    /* objAllocExtra is passed the taskClassId which indicates it needs to allocate the WIND_TCB + 16 bytes.
       taskDestroy routine we call free() which would clobber the TCB & Stack, so we had to allocate an additional 16bytes to keep track of the free'd tasks. */
    pTaskMem = objAllocExtra(taskClassId,(unsigned) (stackSize), (void **) NULL);
    
    if (NULL == pTaskMem)
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadCreate(): NULL pStack \n", 0, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }

	/* Define Stack Base and TCB Base accordingly*/
#if (_STACK_DIR == _STACK_GROWS_DOWN)
	pStackBase = (char*)( (UINT32)pTaskMem + stackSize);
	pNewTcb = (WIND_TCB*) pStackBase;
#else
	pNewTcb = (WIND_TCB*) (pTaskMem + 16);
	pStackBase = STACK_ROUND_UP (pTaskMem + 16 + sizeof(WIND_TCB));
#endif

    retVxWorksVal = taskInit (pNewTcb,  /* address of new task's TCB */
        threadAttr->name,       	/* name of new task */
        threadAttr->priority,   	/* priority of new task */
        VX_DEALLOC_STACK,           	/* task option word */
        pStackBase,                     /* Stack Base pointer */
        stackSize,            		/* size in bytes of stack */
        (FUNCPTR) startRoutine, 	/* entry point */
        (int) arg,              	/* first of the ten arguments to pass to func */
        0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (OK != retVxWorksVal)
    {
        objFree (taskClassId, pTaskMem);
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadCreate(): taskInit error code = %d \n",
            retVxWorksVal, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }

    *ptrTid = (IxOsalThread) pNewTcb;
    return IX_SUCCESS;
}

/* Added this only for vxworks */
IX_STATUS
ixOsalThreadIdGet (IxOsalThread * ptrTid)
{

    *ptrTid = taskIdSelf ();

    /*
     * No errors returned 
     */
    return IX_SUCCESS;
}

IX_STATUS
ixOsalThreadKill (IxOsalThread * tid)
{
    int retVxWorksVal;
    retVxWorksVal = taskDelete (*tid);

    if (OK != retVxWorksVal)
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadKill(): ERROR - taskDelete, return error code = %d \n",
            retVxWorksVal, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }
    return IX_SUCCESS;
}

/* This function never returns */
void
ixOsalThreadExit (void)
{

    ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
        IX_OSAL_LOG_DEV_STDOUT,
        "ixOsalThreadExit(): not supported in vxworks \n", 0, 0, 0, 0, 0, 0);
}

IX_STATUS
ixOsalThreadPrioritySet (IxOsalOsThread * tid, UINT32 priority)
{
    int retVal;

    if (priority > IX_OSAL_MAX_THREAD_PRIORITY)
    {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadPrioritySet(): ERROR - priority is within (0, %d] \n",
            IX_OSAL_MAX_THREAD_PRIORITY, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }
    retVal = taskPrioritySet (*((int *) tid), priority);

    if (OK != retVal)
    {
        return IX_FAIL;
    }
    return IX_SUCCESS;
}

/* 
 * Start thread after given its thread handle
 */
IX_STATUS
ixOsalThreadStart (IxOsalThread * tId)
{
    int retVal = 0;
    retVal = taskActivate (*tId);
    if (retVal != OK)
    {

        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadStart(): ERROR vxworks return code = %d \n",
            retVal, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }
    return IX_SUCCESS;

}

IX_STATUS
ixOsalThreadSuspend (IxOsalThread * tId)
{
    int retVal = 0;
    retVal = taskSuspend (*tId);
    if (retVal != OK)
    {

        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadSuspend(): ERROR vxworks return code = %d \n",
            retVal, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }
    return IX_SUCCESS;

}

IX_STATUS
ixOsalThreadResume (IxOsalThread * tId)
{
    int retVal = 0;
    retVal = taskResume (*tId);
    if (retVal != OK)
    {

        ixOsalLog (IX_OSAL_LOG_LVL_ERROR,
            IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalThreadResume(): ERROR vxworks return code = %d \n",
            retVal, 0, 0, 0, 0, 0);
        return IX_FAIL;
    }
    return IX_SUCCESS;

}

BOOL
ixOsalThreadStopCheck()
{
    /* Yet to be implemented in vxWorks */
    return FALSE;
}
