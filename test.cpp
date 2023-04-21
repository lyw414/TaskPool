//#include "TaskPool.h"
#include <stdio.h>
#include "SerialTask.hpp"


void workFunc(LYW_CODE::TaskPoolDefine::TaskError_t * taskInfo, void * param, int lenOfParam)
{
    usleep(1000);
    printf("%d\n", *(int *)param);

    return;
}


int main()
{
    struct timespec begin;
    struct timespec end;

    int count = 4096;

    LYW_CODE::TaskPool taskPool;
    taskPool.ConfigTaskPool();
    taskPool.CreateTaskPool();

    clock_gettime(CLOCK_MONOTONIC, &begin);

    LYW_CODE::TaskPool::TaskInstance_t taskInstance;

    LYW_CODE::SerialTask serialTask(&taskPool,1024);

    char * ptr;

    for (int iLoop = 0; iLoop < count; iLoop++)
    {
        //taskPool.ExcuteSyncTask(workFunc, &iLoop, 4, 1000);
        taskPool.ExcuteAsyncTask(&taskInstance, workFunc, &iLoop, 4, 1000);
        //workFunc(NULL, NULL, 0);
        //taskPool.AllocateTaskNode(&taskInstance, workFunc, NULL, 10, 1000, (void **)&ptr);
        //strcpy(ptr, "1111111");
        //taskPool.ExcuteAsyncTask(taskInstance);
        //taskPool.ExcuteAsyncTask(&taskInstance, workFunc, NULL, 0, 1000);
        //serialTask.ExcuteSyncTask(workFunc, &iLoop, 4, 1000);
    } 

    int endx = 111;
    taskPool.ExcuteSyncTask(workFunc, &endx, 4, 1000);

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("[ %ld.%ld - %ld.%ld ] [%ld]\n ",begin.tv_sec, begin.tv_nsec, end.tv_sec, end.tv_nsec, ((end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_nsec - begin.tv_nsec) / 1000) / count);

    taskPool.DestroyTaskPool();
    return 0;
}
