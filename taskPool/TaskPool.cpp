#include "TaskPool.h"
#include <time.h>
#include <stdio.h>
namespace LYW_CODE
{
    TaskPool::TaskPool() : m_threadPool(), m_taskNodeQueue()
    {
        m_threadPool.Config();
        m_threadPool.SetWorkFunction(Function1<void(void *)>(&TaskPool::WorkThreadFunc, this), NULL);

        m_threadPool.SetThreadControlFunc(Function1<ThreadPool::ThreadControlOpt_e(void *)>(&TaskPool::ThreadControl, this), NULL);
        m_taskNodeQueue.Config();
    }

    TaskPool::~TaskPool()
    {

    }


    ThreadPool::ThreadControlOpt_e TaskPool::ThreadControl(void * param)
    {
        int busyCount = 0;
        int freeCount = 0;
        int iLoop = 0;

        while (iLoop < 10) 
        {
            iLoop++;
            if (m_taskNodeQueue.IsTaskFull() > 0)
            {
                freeCount = 0;
                busyCount++;
            }

            if (m_taskNodeQueue.IsTaskEmpty() > 0)
            {
                busyCount = 0;
                freeCount ++;
            }
            
            if (freeCount > 3)
            {
                return ThreadPool::THREAD_DEC;
            }
            else if (busyCount > 3)
            {
                return ThreadPool::THREAD_ADD;
            }

            usleep(100000);
        }

        return ThreadPool::THREAD_KEEP;
    }

    void TaskPool::WorkThreadFunc(void * param)
    {
        struct timespec tp;
        ::memset(&tp, 0x00, sizeof(tp));
        clock_gettime(CLOCK_MONOTONIC, &tp);

        tp.tv_sec += 1;

        TaskNodeQueue::TaskHandle_t handle = -1;
        TaskNodeQueue::TaskInfo_t * taskInfo = NULL;
        TaskPoolDefine::TaskError_t taskError;
        taskError.errCode = TaskPoolDefine::NO_ERROR;

        Task_t * task = NULL;
        if ((handle = m_taskNodeQueue.GetBusyTaskNode(&tp)) >= 0)
        {
            taskInfo = m_taskNodeQueue.GetTaskInfo(handle);
            if (taskInfo != NULL && taskInfo->param != NULL)
            {
                Task_t * task = (Task_t *)taskInfo->param;
                task->func(&taskError, task->param, task->lenOfParam);
            }

            m_taskNodeQueue.AddTaskNodeToFreeQueue(handle);
        }
    }

    int TaskPool::ConfigTaskPool()
    {
        //m_threadPool.Config();
        //m_taskNodeQueue.Config();
        return 0;
    }

    int TaskPool::CreateTaskPool()
    {
        m_threadPool.Start();

        m_taskNodeQueue.CreateTaskQueue();

        return 0;
    }

    int TaskPool::DestroyTaskPool()
    {
        m_threadPool.Stop();

        m_taskNodeQueue.DestroyTaskQueue();

        return 0;
    }

    int TaskPool::ExcuteTask(const TaskInstance_t & taskInstance)
    {
        unsigned int uuid = 0;
        Task_t * task = NULL;
        struct timespec timeout_tp;
        TaskNodeQueue::TaskInfo_t * taskInfo = NULL;

        taskInfo = m_taskNodeQueue.GetTaskInfo(taskInstance.handle);

        if (taskInfo != NULL && taskInfo->param != NULL)
        {
            task = (Task_t *)(taskInfo->param);

            if (m_taskNodeQueue.AddTaskNodeToBusyQueue(taskInstance.handle) < 0)
            {
                m_taskNodeQueue.AddTaskNodeToFreeQueue(taskInstance.handle);
            }

            return 0;
        }

        return -1;
    }


    int TaskPool::ExcuteTask(TaskInstance_t * taskInstance, Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, const struct timespec & timeout_tp)
    {
        int sizeOfTask = sizeof(Task_t) + lenParam;
        unsigned int uuid = 0;

        TaskNodeQueue::TaskHandle_t handle = -1;
        TaskNodeQueue::TaskInfo_t * taskInfo = NULL;
        Task_t * task = NULL;

        handle = m_taskNodeQueue.GetFreeTaskNode(sizeOfTask, &timeout_tp, &uuid);

        if (handle >= 0)
        {
            taskInstance->handle = handle;
            taskInstance->uuid = uuid;
            taskInfo = m_taskNodeQueue.GetTaskInfo(handle);

            if (taskInfo != NULL && taskInfo->param != NULL)
            {
                task = (Task_t *)taskInfo->param;
                task->func = func;
                task->lenOfParam = lenParam;
                if (param != NULL)
                {
                    ::memcpy(task->param, param, lenParam);
                }
            }

            if (m_taskNodeQueue.AddTaskNodeToBusyQueue(handle) < 0)
            {
                m_taskNodeQueue.AddTaskNodeToFreeQueue(handle);
            }
            
            return 0;
        }

        return -1;
    }


    int TaskPool::WaitTask(const TaskInstance_t & taskInstance)
    {
        return m_taskNodeQueue.WaitTaskFinished(taskInstance.handle, taskInstance.uuid, &taskInstance.timeout_tp);
    }


    int TaskPool::ExcuteAsyncTask(const TaskInstance_t & taskInstance)
    {
        return ExcuteTask(taskInstance);
    }

    int TaskPool::ExcuteSyncTask(Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout)
    {
        TaskPool::TaskInstance_t taskInstance;

        struct timespec timeout_tp;

        clock_gettime(CLOCK_MONOTONIC, &timeout_tp);

        if (timeout > 0) 
        {
            timeout_tp.tv_sec += timeout / 1000;
            timeout_tp.tv_nsec += (timeout % 1000) * 1000000;
            timeout_tp.tv_sec += timeout_tp.tv_nsec / 1000000000;
            timeout_tp.tv_nsec %= 1000000000;
        }
        else
        {
            timeout_tp.tv_sec = 0;
            timeout_tp.tv_nsec = 0;
        }

        memcpy(&taskInstance.timeout_tp, &timeout_tp, sizeof(struct timespec));

        ExcuteTask(&taskInstance, func, param, lenParam, timeout_tp);

        return WaitTask(taskInstance);

    }

    int TaskPool::ExcuteAsyncTask(TaskInstance_t * taskInstance, Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout)
    {

        if (taskInstance == NULL)
        {
            return -1;
        }
        struct timespec timeout_tp;

        clock_gettime(CLOCK_MONOTONIC, &timeout_tp);

        if (timeout > 0) 
        {
            timeout_tp.tv_sec += timeout / 1000;
            timeout_tp.tv_nsec += (timeout % 1000) * 1000000;
            timeout_tp.tv_sec += timeout_tp.tv_nsec / 1000000000;
            timeout_tp.tv_nsec %= 1000000000;
        }
        else
        {
            timeout_tp.tv_sec = 0;
            timeout_tp.tv_nsec = 0;
        }

        memcpy(&taskInstance->timeout_tp, &timeout_tp, sizeof(struct timespec));

        return ExcuteTask(taskInstance, func, param, lenParam, timeout_tp);
    }


    int TaskPool::AllocateTaskNode (TaskInstance_t * taskInstance, Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout, void ** paramPtr)
    {
        if (taskInstance == NULL)
        {
            return -1;
        }

        Task_t * task = NULL;
        int sizeOfTask = sizeof(Task_t) + lenParam;
        struct timespec timeout_tp;
        clock_gettime(CLOCK_MONOTONIC, &timeout_tp);

        if (timeout > 0) 
        {
            timeout_tp.tv_sec += timeout / 1000;
            timeout_tp.tv_nsec += (timeout % 1000) * 1000000;
            timeout_tp.tv_sec += timeout_tp.tv_nsec / 1000000000;
            timeout_tp.tv_nsec %= 1000000000;
        }
        else
        {
            timeout_tp.tv_sec = 0;
            timeout_tp.tv_nsec = 0;
        }

        unsigned int uuid = 0;

        TaskPool::TaskNodeHandle_t handle = -1;
        TaskNodeQueue::TaskInfo_t * taskInfo = NULL;
        memcpy(&taskInstance->timeout_tp, &timeout_tp, sizeof(struct timespec));

        handle = m_taskNodeQueue.GetFreeTaskNode(sizeOfTask, &timeout_tp, &uuid);

        if (handle >= 0)
        {
            taskInfo = m_taskNodeQueue.GetTaskInfo(handle);
            taskInstance->handle = handle;
            taskInstance->uuid = uuid;

            if (taskInfo != NULL && taskInfo->param != NULL)
            {
                task = (Task_t *)taskInfo->param;
                task->func = func;
                task->lenOfParam = lenParam;

                if (paramPtr != NULL)
                {
                    *paramPtr = task->param;
                }

                if (param != NULL)
                {
                    ::memcpy(task->param, param, lenParam);
                }
            }
        }
        
        return handle;
    }

}

