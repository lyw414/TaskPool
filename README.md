# 功能说明
* 同步、异步执行任务
* 线程池支持注册资源检测 动态调整线程数
* 支持任务节点申请与节点参数保存 用于扩展其他任务类型

# 使用说明
```c
#include "TaskPool.h"
#include <stdio.h>

void workFunc(LYW_CODE::TaskPoolDefine::TaskError_t * taskInfo, void * param, int lenOfParam)
{
    usleep(1000);
    printf("%d\n", *(int *)param);

    return;
}

int main()
{
    LYW_CODE::TaskPool taskPool;
    taskPool.CreateTaskPool();
    int param = 111;

    LYW_CODE::TaskPool::TaskInstance_t taskInstance;

    //异步任务 -- 执行与阻塞
    taskPool.ExcuteAsyncTask(&taskInstance, workFunc, &iLoop, 4, 1000);
    taskPool.WaitTask(taskInstance);

    //同步任务 -- 阻塞模式
    taskPool.ExcuteSyncTask(workFunc, &iLoop, 4, 1000);

    //任务清任务节点与任务添加
    //taskPool.AllocateTaskNode(&taskInstance, workFunc, &param, 4, 1000);
    taskPool.AllocateTaskNode(&taskInstance, workFunc, NULL, 4, 1000, (void **)&ptr);
    memcpy(ptr, &param, 4);
    taskPool.ExcuteAsyncTask(taskInstance);

    taskPool.DestroyTaskPool();
    return 0;
}

```
# 非标准任务扩展
    参考 SerialTask，可实现串行任务、定时任务等模式
