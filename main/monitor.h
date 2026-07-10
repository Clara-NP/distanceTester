#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <common/generic.h>
#include "motor.h"
#include "coil.h"


typedef struct dataMonitorManage dataMonitorInstance_t;
dataMonitorInstance_t* dataMonitorNew(const char* name);
int dataMonitorSetMotorState(dataMonitorInstance_t *m, const motorState_t *state, uint16_t timeout);
int dataMonitorSetCoilState(dataMonitorInstance_t *m, const coilManageState_t *state, uint16_t timeout);
#endif