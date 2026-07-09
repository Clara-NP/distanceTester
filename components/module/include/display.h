#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "common/generic.h"
#include "lcd/lcd_display.h"

typedef struct displayManage displayManage_t;

typedef struct
{
    /// @brief 地址
    uint8_t address;
    /// @brief 通道使能，bit0-3为通道0-3的使能，其他保留
    uint8_t channelEna;

    // /// @brief 探测周期，ms
    // uint16_t detectionPeriod;
    // /// @brief P0级别的采样周期，ms
    // uint16_t samplePeriodP0;
    // /// @brief P1级别的采样周期，ms
    // uint16_t samplePeriodP1;
    // /// @brief P2级别的采样周期，ms
    // uint16_t samplePeriodP2;


    // /// @brief 电压互感器
    // uint16_t pt;
    // /// @brief 电流互感器
    // uint16_t ct;
}displayConfig_t;



displayManage_t *displayNew(uint8_t bus, const char *name, const lcdDisplayConfig_t *config);
void displayManageSchedule(displayManage_t *display);
void displaySetSpeedLevel(int speedLevel);
void displaySetIsPressed(bool isPressed);
#endif