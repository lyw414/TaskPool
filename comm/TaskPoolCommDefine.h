#ifndef __LYW_CODE_TASK_POOL_COMM_DEFINE_H_FILE__
#define __LYW_CODE_TASK_POOL_COMM_DEFINE_H_FILE__
namespace LYW_CODE
{
    namespace TaskPoolDefine
    {
        typedef enum _TaskErrCode {
            NO_ERROR,
            TIMEOUT
        } TaskErrCode_e;

        typedef struct _TaskError {
            TaskErrCode_e errCode;
        } TaskError_t;
    }
}

#endif
