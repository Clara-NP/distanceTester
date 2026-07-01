#ifndef __IO_MODULE_H__
#define __IO_MODULE_H__

#include "common/generic.h"


/**
 * @brief 指示灯实例定义
 * 
 */
typedef struct ioModuleManage ioModuleManage_t;

ioModuleManage_t *ioModuleNew(uint8_t id, const char *name, bool highActive, int defaultState);

#endif // __IO_MODULE_H__