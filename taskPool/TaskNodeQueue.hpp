#ifndef __LYW_CODE_TASK_NODE_QUEUE_FILE_HPP__
#define __LYW_CODE_TASK_NODE_QUEUE_FILE_HPP__
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "TaskPoolCommDefine.h"

namespace LYW_CODE
{
    class TaskNodeQueue
    {
    public:
        typedef int TaskHandle_t;

        typedef struct _TaskInfo {
            void * param;
            int lenOfParam;
        } TaskInfo_t;

    private:
        typedef enum _TaskNodeState {
            BUSY_NODE,
            FREE_NODE,
            ALLOCATE_NODE
        } TaskNodeState_e;


        typedef struct _TaskNode {
            TaskInfo_t taskInfo;
            
            //任务超时结束时间
            struct timespec endTime;
            
            //动态参数缓存 -- 默认参数缓存不够时使用
            void * dynamicParam;
            int sizeOfDynamicParam;
            
            //默认参数缓存
            void * defaultParam;
            int sizeOfDefaultParam;
            
            //0 freeNode 1 occupyNode 其余未知节点
            TaskNodeState_e nodeState;

            unsigned int uuid;

            TaskHandle_t handle;

        } TaskNode_t;


        typedef struct _TaskListNode {
            struct _TaskListNode * pre;
            struct _TaskListNode * next;
            TaskNode_t taskNode;
        } TaskListNode_t;

    private:
        int m_maxSize;
        
        int m_defaultParamCacheSize;

        //全节点
        TaskListNode_t * m_taskNodeArray;
        pthread_mutex_t m_lock;
        
        //闲链表
        TaskListNode_t * m_freeTaskNodeListHead;
        TaskListNode_t * m_freeTaskNodeListEnd;
        pthread_mutex_t m_freeTaskNodeListLock;
        
        //忙链表
        TaskListNode_t * m_busyTaskNodeListHead;
        TaskListNode_t * m_busyTaskNodeListEnd;
        pthread_mutex_t m_busyTaskNodeListLock;
 
        //闲链表新增通知
        pthread_cond_t m_freeTaskNodeListAddNotifyCond ;
        pthread_condattr_t m_freeTaskNodeListAddNotifyCondAttr;
        
        //忙链表新增通知
        pthread_cond_t m_busyTaskNodeListAddNotifyCond ;
        pthread_condattr_t m_busyTaskNodeListAddNotifyCondAttr;


    private:
        void showList(TaskListNode_t * head)
        {
            TaskListNode_t * current = head;
            int iLoop = 0;
            printf("*************** List Node ***************\n");
            while (current != NULL)
            {
                printf("%d index [%d] pre [%p] next [%p]\n", iLoop, current->taskNode.handle, current->pre, current->next);
                current = current->next;
                iLoop++;
            }
        }
 

    public:

        TaskNodeQueue()
        {
            m_maxSize = 0;
        
            m_defaultParamCacheSize = 0;

            //全节点
            m_taskNodeArray = NULL;
            pthread_mutex_init(&m_lock, NULL);
        
            //闲链表
            m_freeTaskNodeListHead = NULL;
            m_freeTaskNodeListEnd = NULL;
            pthread_mutex_init(&m_freeTaskNodeListLock, NULL);
        
            //忙链表
            m_busyTaskNodeListHead = NULL;
            m_busyTaskNodeListEnd = NULL;
            pthread_mutex_init(&m_busyTaskNodeListLock, NULL);
 
            //闲链表新增通知
            
            pthread_condattr_setclock(&m_freeTaskNodeListAddNotifyCondAttr, CLOCK_MONOTONIC); 
            pthread_cond_init(&m_freeTaskNodeListAddNotifyCond, &m_freeTaskNodeListAddNotifyCondAttr);
        
            //忙链表新增通知
            pthread_condattr_setclock(&m_busyTaskNodeListAddNotifyCondAttr, CLOCK_MONOTONIC); 
            pthread_cond_init(&m_busyTaskNodeListAddNotifyCond, &m_busyTaskNodeListAddNotifyCondAttr);
 
        }

        ~TaskNodeQueue()
        {
            DestroyTaskQueue();
        }


        int CreateTaskQueue()
        {
            DestroyTaskQueue();

            m_taskNodeArray = (TaskListNode_t *)::malloc(sizeof(TaskListNode_t) * m_maxSize);

            for (int iLoop = 0; iLoop < m_maxSize; iLoop++)
            {
                m_taskNodeArray[iLoop].taskNode.handle = iLoop;
                m_taskNodeArray[iLoop].taskNode.defaultParam = ::malloc(m_defaultParamCacheSize);
                m_taskNodeArray[iLoop].taskNode.sizeOfDefaultParam = m_defaultParamCacheSize;
                m_taskNodeArray[iLoop].taskNode.dynamicParam = NULL;
                m_taskNodeArray[iLoop].taskNode.sizeOfDynamicParam = 0;
                m_taskNodeArray[iLoop].taskNode.endTime.tv_sec = 0;
                m_taskNodeArray[iLoop].taskNode.endTime.tv_nsec = 0;
                m_taskNodeArray[iLoop].taskNode.nodeState = FREE_NODE;
                m_taskNodeArray[iLoop].taskNode.taskInfo.lenOfParam = 0;
                m_taskNodeArray[iLoop].taskNode.taskInfo.param = NULL;
                //pre指针初始化
                if (iLoop != 0)
                {
                    m_taskNodeArray[iLoop].pre = &m_taskNodeArray[iLoop - 1];
                }
                else
                {
                    //头节点
                    m_taskNodeArray[iLoop].pre = NULL;
                    m_freeTaskNodeListHead = &m_taskNodeArray[iLoop];
                }

                //next指针初始化
                if (iLoop + 1 != m_maxSize)
                {
                    m_taskNodeArray[iLoop].next = &m_taskNodeArray[iLoop + 1];
                }
                else
                {
                    //尾节点
                    m_taskNodeArray[iLoop].next = NULL;
                    m_freeTaskNodeListEnd = &m_taskNodeArray[iLoop];
                }

            }
           

            return 0;
        }


        int DestroyTaskQueue()
        {
            if (m_taskNodeArray != NULL)
            {
                for (int iLoop = 0; iLoop < m_maxSize; iLoop++)
                {
                    if (m_taskNodeArray[iLoop].taskNode.defaultParam != NULL)
                    {
                        free(m_taskNodeArray[iLoop].taskNode.defaultParam);
                    }

                    if (m_taskNodeArray[iLoop].taskNode.dynamicParam != NULL)
                    {
                        free(m_taskNodeArray[iLoop].taskNode.dynamicParam);
                    }
                }

                free(m_taskNodeArray);
                m_taskNodeArray = NULL;
            }

            return 0;
        }


        int Config()
        {
            m_maxSize = 1024;
            m_defaultParamCacheSize = 128;
            return 0;
        }

        TaskHandle_t GetFreeTaskNode(int lenOfParam, const struct timespec * endTime, unsigned int * uuid)
        {
            TaskHandle_t handle = -1;

            int flg = 1;

            while (true)
            {
                ::pthread_mutex_lock(&m_freeTaskNodeListLock);
                if (m_freeTaskNodeListHead != NULL)
                {
                    //取到节点 
                    handle = m_freeTaskNodeListHead->taskNode.handle;
                    if (m_freeTaskNodeListHead->next != NULL)
                    {
                        m_freeTaskNodeListHead = m_freeTaskNodeListHead->next;
                        m_freeTaskNodeListHead->pre = NULL;
                    }
                    else
                    {
                        m_freeTaskNodeListHead = NULL;
                        m_freeTaskNodeListEnd = NULL;
                    }
                    ::pthread_mutex_unlock(&m_freeTaskNodeListLock);
                    m_taskNodeArray[handle].taskNode.nodeState = ALLOCATE_NODE;
                    //m_taskNodeArray[handle].taskNode.uuid++;
                    *uuid = m_taskNodeArray[handle].taskNode.uuid;
                    //参数
                    if (lenOfParam <= m_taskNodeArray[handle].taskNode.sizeOfDefaultParam)
                    {
                        //使用默认参数缓存
                        if (m_taskNodeArray[handle].taskNode.dynamicParam != NULL)
                        {

                            free(m_taskNodeArray[handle].taskNode.dynamicParam);
                            m_taskNodeArray[handle].taskNode.dynamicParam = NULL;
                            m_taskNodeArray[handle].taskNode.sizeOfDynamicParam = 0;
                        }

                        m_taskNodeArray[handle].taskNode.taskInfo.param = m_taskNodeArray[handle].taskNode.defaultParam;
                        m_taskNodeArray[handle].taskNode.taskInfo.lenOfParam = lenOfParam;
                    }
                    else
                    {
                        if (m_taskNodeArray[handle].taskNode.sizeOfDynamicParam < lenOfParam || lenOfParam * 2 < m_taskNodeArray[handle].taskNode.sizeOfDynamicParam)
                        {
                            //动态参数缓存过下 或 超过参数的两倍 则重新分配
                            free(m_taskNodeArray[handle].taskNode.dynamicParam);
                            m_taskNodeArray[handle].taskNode.dynamicParam = malloc(lenOfParam);
                            m_taskNodeArray[handle].taskNode.sizeOfDynamicParam = lenOfParam;
                        }

                        m_taskNodeArray[handle].taskNode.taskInfo.param = m_taskNodeArray[handle].taskNode.dynamicParam;
                        m_taskNodeArray[handle].taskNode.taskInfo.lenOfParam = lenOfParam;

                    }

                    //printf("GET FREE %d\n", handle);

                    return handle;
                }
                else
                {
                    //::pthread_mutex_unlock(&m_freeTaskNodeListLock);
                    //printf("WAIT FREE\n");
                    if (flg == 0)
                    {
                        ::pthread_mutex_unlock(&m_freeTaskNodeListLock);
                        //超时唤醒后需要再尝试获取一次
                        return handle;
                    }
                    
                    //阻塞等待闲节点归还
                    if (NULL == endTime || (endTime->tv_sec == 0 && endTime->tv_nsec == 0))
                    {
                        struct timespec curTime;
                        clock_gettime(CLOCK_MONOTONIC, &curTime);
                        curTime.tv_sec += 2;
                        pthread_mutex_lock(&m_freeTaskNodeListLock);
                        pthread_cond_timedwait(&m_freeTaskNodeListAddNotifyCond, &m_freeTaskNodeListLock, &curTime);
                        pthread_mutex_unlock(&m_freeTaskNodeListLock);

                    }
                    else
                    {
                        //pthread_mutex_lock(&m_freeTaskNodeListLock);
                        //if (pthread_cond_timedwait(&m_freeTaskNodeListAddNotifyCond, &m_freeTaskNodeListLock, endTime) != 0)

                        if (pthread_cond_timedwait(&m_freeTaskNodeListAddNotifyCond, &m_freeTaskNodeListLock, endTime) != 0)
                        {
                            //异常唤醒 尝试获取一次后退出
                            printf("GET Free TIMEOUT\n");
                            flg = 0;
                        }
                        else
                        {
                            //printf("Free WAKE\n");
                        }
                        //pthread_mutex_unlock(&m_freeTaskNodeListLock);
                        pthread_mutex_unlock(&m_freeTaskNodeListLock);
                    }
                }

            }

            return 0;
        }

        TaskHandle_t GetBusyTaskNode(const struct timespec * endTime)
        {
            TaskHandle_t handle = -1;

            int flg = 1;

            while (true)
            {
                ::pthread_mutex_lock(&m_busyTaskNodeListLock);
                if (m_busyTaskNodeListHead != NULL)
                {
                    //取到节点 
                    handle = m_busyTaskNodeListHead->taskNode.handle;
                    if (m_busyTaskNodeListHead->next != NULL)
                    {
                        m_busyTaskNodeListHead = m_busyTaskNodeListHead->next;
                        m_busyTaskNodeListHead->pre = NULL;
                    }
                    else
                    {
                        m_busyTaskNodeListHead = NULL;
                        m_busyTaskNodeListEnd = NULL;
                    }

                    ::pthread_mutex_unlock(&m_busyTaskNodeListLock);
                    m_taskNodeArray[handle].taskNode.nodeState = ALLOCATE_NODE;

                    //printf("GET BUSY %d\n", handle);
                    return handle;
                }
                else
                {
                    //::pthread_mutex_unlock(&m_busyTaskNodeListLock);
                    if (flg == 0)
                    {
                        ::pthread_mutex_unlock(&m_busyTaskNodeListLock);
                        //超时唤醒后需要再尝试获取一次
                        return handle;
                    }
                    
                    //阻塞等待闲节点归还
                    if (NULL == endTime || (endTime->tv_sec == 0 && endTime->tv_nsec == 0))
                    {
                        struct timespec curTime;
                        clock_gettime(CLOCK_MONOTONIC, &curTime);
                        curTime.tv_sec += 2;
                        pthread_mutex_lock(&m_busyTaskNodeListLock);
                        pthread_cond_timedwait(&m_busyTaskNodeListAddNotifyCond, &m_busyTaskNodeListLock, &curTime);
                        pthread_mutex_unlock(&m_busyTaskNodeListLock);
                    }
                    else
                    {

                        //printf("WAIT BUSY\n");
                        //pthread_mutex_lock(&m_busyTaskNodeListLock);
                        //if (pthread_cond_timedwait(&m_busyTaskNodeListAddNotifyCond, &m_busyTaskNodeListLock, endTime) != 0)
                        if (pthread_cond_timedwait(&m_busyTaskNodeListAddNotifyCond, &m_busyTaskNodeListLock, endTime) != 0)
                        {
                            //异常唤醒 尝试获取一次后退出
                            printf("BUSY TIMEOUT\n");
                            flg = 0;
                        }
                        else
                        {
                            //printf("BUSY WAKE\n");
                        }
                        //pthread_mutex_unlock(&m_busyTaskNodeListLock);
                        pthread_mutex_unlock(&m_busyTaskNodeListLock);
                    }
                }

            }

            return 0;
        }

        int AddTaskNodeToBusyQueue(TaskHandle_t handle)
        {
            if (m_taskNodeArray == NULL)
            {
                return -3;
            }

            if (handle < 0 || handle >= m_maxSize)
            {
                return -1;
            }

            if (m_taskNodeArray[handle].taskNode.nodeState != ALLOCATE_NODE)
            {
                return -2;
            }

            pthread_mutex_lock(&m_busyTaskNodeListLock);
            m_taskNodeArray[handle].taskNode.nodeState = BUSY_NODE;
            if (m_busyTaskNodeListHead != NULL) 
            {
                m_busyTaskNodeListEnd->next = &m_taskNodeArray[handle];

                m_taskNodeArray[handle].pre = m_busyTaskNodeListEnd;
                m_taskNodeArray[handle].next = NULL;
                m_busyTaskNodeListEnd = &m_taskNodeArray[handle];
            }
            else
            {
                m_taskNodeArray[handle].pre = NULL;
                m_taskNodeArray[handle].next = NULL;
                
                m_busyTaskNodeListEnd = &m_taskNodeArray[handle];
                m_busyTaskNodeListHead = &m_taskNodeArray[handle];
            }
            pthread_mutex_unlock(&m_busyTaskNodeListLock);

            //printf("ADD BUSY %d\n", handle);
            pthread_cond_broadcast(&m_busyTaskNodeListAddNotifyCond);

            return 0;
        }


        int AddTaskNodeToFreeQueue(TaskHandle_t handle)
        {
            if (m_taskNodeArray == NULL)
            {
                return -3;
            }

            if (handle < 0 || handle >= m_maxSize)
            {
                return -1;
            }

            if (m_taskNodeArray[handle].taskNode.nodeState != ALLOCATE_NODE)
            {
                return -2;
            }

            pthread_mutex_lock(&m_freeTaskNodeListLock);
            m_taskNodeArray[handle].taskNode.nodeState = FREE_NODE;
            m_taskNodeArray[handle].taskNode.uuid++;
            if (m_freeTaskNodeListHead != NULL) 
            {
                m_freeTaskNodeListEnd->next = &m_taskNodeArray[handle];

                m_taskNodeArray[handle].pre = m_freeTaskNodeListEnd;
                m_taskNodeArray[handle].next = NULL;
                m_freeTaskNodeListEnd = &m_taskNodeArray[handle];
            }
            else
            {
                m_taskNodeArray[handle].pre = NULL;
                m_taskNodeArray[handle].next = NULL;
                
                m_freeTaskNodeListEnd = &m_taskNodeArray[handle];
                m_freeTaskNodeListHead = &m_taskNodeArray[handle];
            }
            pthread_mutex_unlock(&m_freeTaskNodeListLock);

            //printf("ADD FREE %d\n", handle);
            pthread_cond_broadcast(&m_freeTaskNodeListAddNotifyCond);

            return 0;
        }


        int WaitTaskFinished(TaskHandle_t handle, unsigned int uuid, const struct timespec * endTime)
        {
            int flg = 1;
            if (m_taskNodeArray == NULL)
            {
                return -3;
            }

            if (handle < 0 || handle >= m_maxSize)
            {
                return -1;
            }

            while (true)
            {
                pthread_mutex_lock(&m_freeTaskNodeListLock);
                if (uuid != m_taskNodeArray[handle].taskNode.uuid ||  m_taskNodeArray[handle].taskNode.nodeState == FREE_NODE)
                {
                    pthread_mutex_unlock(&m_freeTaskNodeListLock);
                    return 0;
                }
                
                if (flg == 0) 
                {
                    pthread_mutex_unlock(&m_freeTaskNodeListLock);
                    return -1;
                }
                //阻塞等待闲节点归还
                if (NULL == endTime || (endTime->tv_sec == 0 && endTime->tv_nsec == 0))
                {
                    struct timespec curTime;
                    clock_gettime(CLOCK_MONOTONIC, &curTime);
                    curTime.tv_sec += 2;
                    pthread_mutex_lock(&m_freeTaskNodeListLock);
                    pthread_cond_timedwait(&m_freeTaskNodeListAddNotifyCond, &m_freeTaskNodeListLock, &curTime);
                    pthread_mutex_unlock(&m_freeTaskNodeListLock);
                }
                else
                {
                    //pthread_mutex_lock(&m_freeTaskNodeListLock);
                    //if (pthread_cond_timedwait(&m_freeTaskNodeListAddNotifyCond, &m_freeTaskNodeListLock, endTime) != 0)

                    if (pthread_cond_timedwait(&m_freeTaskNodeListAddNotifyCond, &m_freeTaskNodeListLock, endTime) != 0)
                    {
                        //异常唤醒 尝试检测一次后退出
                        flg = 0;
                        printf("WAIT FINISH TIMEOUT\n");
                    }
                    else
                    {
                        //printf("FINISHED\n");
                    }

                    //pthread_mutex_unlock(&m_freeTaskNodeListLock);
                    pthread_mutex_unlock(&m_freeTaskNodeListLock);
                }

            }
            return 0;

        }

        TaskInfo_t * GetTaskInfo(TaskHandle_t handle)
        {
            if (m_taskNodeArray == NULL)
            {
                return NULL;
            }

            if (handle < 0 || handle >= m_maxSize)
            {
                return NULL;
            }

            return &m_taskNodeArray[handle].taskNode.taskInfo;
        }


        int IsTaskFull()
        {
            if (m_freeTaskNodeListHead == NULL)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        int IsTaskEmpty()
        {
            if (m_busyTaskNodeListHead == NULL)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    };
}

#endif
