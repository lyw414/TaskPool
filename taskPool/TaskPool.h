#ifndef __LYW_CODE_TASK_POOL_FILE_H__
#define __LYW_CODE_TASK_POOL_FILE_H__

#include "ThreadPool.hpp"
#include "TaskNodeQueue.hpp"

namespace LYW_CODE
{
    class TaskPool
    {
    public:
        typedef LYW_CODE::TaskNodeQueue::TaskHandle_t TaskNodeHandle_t;
        
        typedef struct _TaskInstance {
            TaskNodeHandle_t handle;
            unsigned int uuid;
            struct timespec timeout_tp;
        } TaskInstance_t;

    private:

        typedef struct _Task {
            Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func;
            int lenOfParam;
            unsigned char param[0];
        } Task_t;


    private:
        LYW_CODE::ThreadPool m_threadPool;
        LYW_CODE::TaskNodeQueue m_taskNodeQueue;

    private:
        void WorkThreadFunc(void * param);

        ThreadPool::ThreadControlOpt_e ThreadControl(void * param);
        int ExcuteTask(TaskInstance_t * taskInstance, Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, const struct timespec & timeout_tp);

        int ExcuteTask(const TaskInstance_t & taskInstance);



    public:
        TaskPool();

        ~TaskPool();

        int ConfigTaskPool();

        int CreateTaskPool();

        int DestroyTaskPool();

        //常规任务
        int ExcuteAsyncTask(TaskInstance_t * taskInstance, Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout);

        int ExcuteSyncTask(Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout);

        int AllocateTaskNode(TaskInstance_t * taskInstance, Function3<void (TaskPoolDefine::TaskError_t *, void *, int)> func, void * param, int lenParam, int timeout, void ** paramPtr = NULL);


        int ExcuteAsyncTask(const TaskInstance_t & taskInstance);


        int WaitTask(const TaskInstance_t & taskInstance);


        int UnusedTaskINodeForFree(const TaskInstance_t & taskInstance);

    };
}
#endif
