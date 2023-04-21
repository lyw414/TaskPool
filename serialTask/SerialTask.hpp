#ifndef __LYW_CODE_SERIAL_TASK_FILE_HPP__
#define __LYW_CODE_SERIAL_TASK_FILE_HPP__
#include "TaskPool.h"
#include <list>
namespace LYW_CODE
{
    class SerialTask
    {
    private:
       typedef struct _Task {
            Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func;
            int lenParam;
            char param[0];
       } Task_t;


    private:
        TaskPool * m_taskPool;
        int m_maxSize;
        std::list <LYW_CODE::TaskPool::TaskInstance_t> m_taskList;

        pthread_mutex_t m_lock;

    private:
        void SerialTaskEnter(TaskPoolDefine::TaskError_t * error, void * param, int lenOfParam)
        {
            Task_t * task = (Task_t *)param;
            if (task != NULL)
            {
                task->func(error, task->param, task->lenParam);
            }
            
            pthread_mutex_lock(&m_lock);
            if (!m_taskList.empty())
            {
                TaskPool::TaskInstance_t instance = m_taskList.front();
                m_taskList.pop_front();
                m_taskPool->ExcuteAsyncTask(instance);
            }
            pthread_mutex_unlock(&m_lock);

        }

    public:
        SerialTask(TaskPool * taskPool, int maxSize)
        {
            m_taskList.clear();
            m_taskPool = taskPool;
            pthread_mutex_init(&m_lock, NULL);
        }

        ~SerialTask()
        {
            m_taskList.clear();
        }

        int ExcuteSyncTask(Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout)
        {
            Task_t * task;
            LYW_CODE::TaskPool::TaskInstance_t instance;

            if (m_taskPool != NULL)
            {
                m_taskPool->AllocateTaskNode(&instance, Function3<void (TaskPoolDefine::TaskError_t *, void *, int)>(&SerialTask::SerialTaskEnter, this), param, lenParam, timeout, (void **)&task);
                task->func = func;
                task->lenParam = lenParam;
                if (param != NULL)
                {
                    ::memcpy(task->param, param, lenParam);
                }
                pthread_mutex_lock(&m_lock);
                if (m_taskList.empty())
                {
                    m_taskPool->ExcuteAsyncTask(instance);
                }
                else
                {
                    m_taskList.push_back(instance);
                }

                pthread_mutex_unlock(&m_lock);
            }

            return 0;
        }


        int ExcuteAsyncTask(Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout)
        {
            Task_t * task;
            LYW_CODE::TaskPool::TaskInstance_t instance;

            if (m_taskPool != NULL)
            {
                m_taskPool->AllocateTaskNode(&instance, func, param, lenParam, timeout, (void **)&task);
                task->func = func;
                task->lenParam = lenParam;
                if (param != NULL)
                {
                    ::memcpy(task->param, param, lenParam);
                }
                pthread_mutex_lock(&m_lock);
                if (m_taskList.empty())
                {
                    m_taskPool->ExcuteAsyncTask(instance);
                }
                else
                {
                    m_taskList.push_back(instance);
                }

                pthread_mutex_unlock(&m_lock);
            }

            return m_taskPool->WaitTask(instance);
        }

    };
}
#endif
