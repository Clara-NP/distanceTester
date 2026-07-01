#include "common/generic.h"
#include <gpio/gpio.h>
#include "io_module.h"

#define TRACE_TAG "io-module"
#define TRACE_LEVEL T_DEBUG
#define TRACE_ENABLE

#include <common/trace.h>

/**
 * @brief 定义一个IO模块管理器
 * 
 */
struct ioModuleManage
{
    /// @brief gpio id
    uint8_t id;
    /// @brief 名称
    const char *name;
    /// @brief 是否为高电平有效
    bool highActive;
    /// @brief 默认状态
    //int defaultState;

    /// 闪烁控制 
    uint32_t flashCount;
    /// 闪烁间隔，ms
    uint32_t flashInterval;
    /// 下次执行时间
    uint32_t flashTick;
    /// 当前状态
    int currentState;
    /// 结束状态
    int endState;
};

/**
 * @brief IO控制
 * 
 */
#define ioModuleOutputSet(_m, _state)        (_m->highActive ? gpioSet(_m->id, (_state)) : gpioSet(_m->id, !(_state)) )

/**
 * @brief 创建一个IO模块管理器
 * 
 * @param id gpio id
 * @param name 名称
 * @param highActive 是否为高有效
 * @param defaultState 默认状态
 * @return ioModuleManage_t* 
 */
ioModuleManage_t *ioModuleNew(uint8_t id, const char *name, bool highActive, int defaultState)
{
    ioModuleManage_t *p;

    p = (ioModuleManage_t *)osMalloc(sizeof(*p));
    if(!p) {
        elog("indicator lamp new failed,not memory");
        return NULL;
    }

    osMemset(p , 0, sizeof(*p));

    p->id = id;
    p->name = name;
    p->highActive = highActive;
    p->currentState = defaultState;

    // 设置默认状态
    ioModuleOutputSet(p, p->currentState);

    ilog("io module(id=%d %s) new success", id, name);

    return p;
}


