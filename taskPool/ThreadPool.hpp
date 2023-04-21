#ifndef __LYW_CODE_THREAD_POOL_FILE_HPP__
#define __LYW_CODE_THREAD_POOL_FILE_HPP__
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "TaskPoolCommDefine.h"
#include "SimpleFunction.hpp"


namespace LYW_CODE
{
    class ThreadPool 
    {
    public:
        typedef enum _ThreadControlOpt{
            THREAD_ADD,
            THREAD_KEEP,
            THREAD_DEC
        } ThreadControlOpt_e;

    private:

        typedef enum _ThreadState {
            RUNNING,
            STOPPING,
            STOPPED
        } ThreadState_e;

        typedef struct _ThreadNode {
            pthread_t handle;
            pthread_attr_t attr;
            ThreadState_e threadState;
            ThreadPool * self;
        } ThreadNode_t;


    private:
        Function1<void(void *)> m_workFunc;
        Function1<ThreadControlOpt_e(void *)> m_threadControlFunc;
        void * m_userParam;
        void * m_userParamThreadControlFunc;

        pthread_mutex_t m_lock;

        int m_maxThreadNum;

        ThreadNode_t ** m_threadArray;
        
        int m_threadPoolState;

        int m_threadNum;

        pthread_t m_threadManageHandle;


    private:
        static void * ThreadEnter(void * ptr)
        {
            ThreadNode_t * threadNode = (ThreadNode_t *)ptr;

            
            if (NULL != threadNode)
            {
                threadNode->self->ThreadEnterRun(threadNode);
            }
            return NULL;
        }

        void ThreadEnterRun(ThreadNode_t * threadNode)
        {
            while (threadNode->threadState == RUNNING) 
            {
                if (m_workFunc != NULL)
                {
                    threadNode->self->m_workFunc(m_userParam);
                    sched_yield();
                }
                else
                {
                    sleep(1);
                }
            }
            free(threadNode);
        }

        
        static void * ThreadManagerEnter(void * ptr)
        {
            ThreadPool * self = (ThreadPool *)ptr;

            if (self != NULL)
            {
                self->ThreadManagerEnterRun();
            }
            return NULL;
        }


        void ThreadManagerEnterRun()
        {
            ThreadControlOpt_e opt;
            while (m_threadPoolState == 1) 
            {
                if (m_threadControlFunc != NULL)
                {
                    opt = m_threadControlFunc(m_userParamThreadControlFunc);
                    ThreadControl(opt);
                }
                ::sleep(1);
            }
        }

        int ThreadControl (ThreadControlOpt_e opt)
        {
            if (NULL == m_threadArray || 0 == m_threadPoolState)
            {
                return -1;
            }

            switch (opt)
            {
                case THREAD_ADD:
                {
                    pthread_mutex_lock(&m_lock);
                    for (int iLoop = 0; iLoop < m_maxThreadNum; iLoop++)
                    {
                        if (m_threadArray[iLoop] == NULL)
                        {
                            m_threadNum++;
                            m_threadArray[iLoop] = (ThreadNode_t *)::malloc(sizeof(ThreadNode_t));
                            m_threadArray[iLoop]->threadState = RUNNING;
                            m_threadArray[iLoop]->self = this;
                            pthread_attr_init(&m_threadArray[iLoop]->attr);
                            pthread_attr_setdetachstate(&m_threadArray[iLoop]->attr, PTHREAD_CREATE_DETACHED);

                            pthread_create(&m_threadArray[iLoop]->handle, NULL, ThreadPool::ThreadEnter, m_threadArray[iLoop]);
                            printf("ADD THREAD\n");
                            break;
                        }
                    }
                    pthread_mutex_unlock(&m_lock);
                    break;
                }
                case THREAD_DEC:
                {
                    pthread_mutex_lock(&m_lock);
                    if (m_threadNum <= 1)
                    {
                        break;
                    }

                    for (int iLoop = 0; iLoop < m_maxThreadNum; iLoop++)
                    {
                        if (m_threadArray[iLoop] != NULL)
                        {
                            m_threadArray[iLoop]->threadState = STOPPING;
                            m_threadArray[iLoop] = NULL;
                            m_threadNum--;
                        }

                        printf("DEC THREAD\n");
                    }
                    pthread_mutex_unlock(&m_lock);
                    break;
                }
                default:
                break;
            }

            return 0;
        }


    public:
        ThreadPool()
        {
            m_workFunc = NULL;

            m_userParam = NULL;

            m_threadArray = NULL;

            m_threadControlFunc = NULL;

            m_userParamThreadControlFunc = NULL;

            m_workFunc = NULL;

            m_userParam = NULL;

            pthread_mutex_init(&m_lock, NULL);
        }

        ~ThreadPool()
        {
            Stop();
        }

        int Config()
        {
            m_maxThreadNum = 8;
            return 0;
        }

        int SetWorkFunction (Function1<void (void *)> workFunc, void * userParam)
        {
            m_workFunc = workFunc;
            m_userParam = userParam;
            return 0;
        }

        int SetThreadControlFunc(Function1<ThreadControlOpt_e (void *)> controlFunc, void * userParam)
        {
            m_threadControlFunc = controlFunc;
            m_userParamThreadControlFunc = userParam;
            return 0;
        }



        int Start()
        {
            Stop();
            m_threadArray = (ThreadNode_t **)malloc(m_maxThreadNum * sizeof(ThreadNode_t *));
            ::memset(m_threadArray, 0x00, m_maxThreadNum * sizeof(ThreadNode_t *));

            m_threadPoolState = 1;
            
            m_threadNum = 1;
            
            //暂时先启动一个
            for (int iLoop = 0; iLoop < 1; iLoop++)
            {
                m_threadArray[iLoop] = (ThreadNode_t *)::malloc(sizeof(ThreadNode_t));
                m_threadArray[iLoop]->threadState = RUNNING;
                m_threadArray[iLoop]->self = this;
                pthread_attr_init(&m_threadArray[iLoop]->attr);
                pthread_attr_setdetachstate(&m_threadArray[iLoop]->attr, PTHREAD_CREATE_DETACHED);

                pthread_create(&m_threadArray[iLoop]->handle, NULL, ThreadPool::ThreadEnter, m_threadArray[iLoop]);
            }

            pthread_create(&m_threadManageHandle, NULL, ThreadPool::ThreadManagerEnter, this);

            return 0;
        }

        int Stop()
        {
            void * res;
            m_threadPoolState = 0;
            if (m_threadArray != NULL)
            {
                pthread_join(m_threadManageHandle, &res);
                for (int iLoop = 0; iLoop < m_maxThreadNum; iLoop++)
                {
                    if (m_threadArray[iLoop] != NULL)
                    {
                        m_threadArray[iLoop]->threadState = STOPPING;
                    }
                }
                ::free(m_threadArray);
                m_threadArray = NULL;
            }
            return 0;
        }
    };
}
#endif
