#include "common/generic.h"
#include "display.h"
#include "esp_err.h"
// #include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd/lcd_display.h"

#include "lvgl.h"

#define TRACE_TAG "display"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>

#ifndef CONFIG_LVGL_TASK_STACK_SIZE
#define CONFIG_LVGL_TASK_STACK_SIZE 4096
#endif

#ifndef CONFIG_LVGL_TASK_PRIORITY
#define CONFIG_LVGL_TASK_PRIORITY 5
#endif

#define DISP_HOR_RES    240
#define DISP_VER_RES    240
#define DRAW_BUF_LINES      40   
/* 单显示缓冲大小(像素数), 取 1/6 屏幕左右, 双 buffer 让 LVGL 可并行绘制 */
#define DISP_BUF_PIXELS (DISP_HOR_RES * 40)
/* 双缓冲, partial 模式; 每个 240*40*2 = 19200 字节 */
static uint8_t s_buf1[DISP_HOR_RES * DRAW_BUF_LINES * 2];
static uint8_t s_buf2[DISP_HOR_RES * DRAW_BUF_LINES * 2];

struct displayManage
{
    /// 逻辑ID
    //uint8_t id;
    /// UART ID
    uint8_t bus;
    /// 超时计数
    uint8_t timeoutCount;
    // /// 空调类型
    // uint8_t type;
    // /// 协议是否已经确认
    // uint8_t typeConfirm;
    /// 配置
    const lcdDisplayConfig_t *config;
    /// 状态
    // coilManageState_t state;
    // /// 空调信息
    // acManageInfo_t info;

    // /// 下次探测时间
    // sysTick_t expiredDetection;
    // /// 离线时间
    // sysTick_t expiredOffline;
    // /// 一次性读出25个寄存器的数据，所以这里只取p0优先级
    // sysTick_t expiredP0;
    // /// 运行时间1秒超时
    // sysTick_t expiredSecond;
};
/* ===================== 前置声明 ===================== */
static void lvglInit(void);
static void lvglFlushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static uint32_t lvglTickCb(void);

static displayManage_t *s_display = NULL;
static lv_obj_t *lblStatus;
static lv_obj_t *lblDirection;
static lv_obj_t *lblSpeedLevel;

displayManage_t *displayNew(uint8_t bus, const char *name, const lcdDisplayConfig_t *config)
{
    displayManage_t *m = osMalloc(sizeof(displayManage_t));
    osMemset(m, 0, sizeof(displayManage_t));
    m->bus = bus;
    // m->name = name;
    m->config = config;

    /* 先初始化底层 ST7789 */
    lcdDisplayInit(config);
    s_display = m;

    /* 启动 LVGL */
    lvglInit();

    return m;
}



/* ===================== LVGL 集成 ===================== */
static void lvglInit(void)
{
    /* 1. LVGL 核心 */
    lv_init();

    /* 2. tick: 直接复用 esp_timer 高精度时钟, 无需额外定时器任务 */
    lv_tick_set_cb(lvglTickCb);

    /* 3. 显示对象 */
    lv_display_t *disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_buf1, s_buf2, sizeof(s_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, lvglFlushCb);

    // test
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* 状态 */
    lblStatus = lv_label_create(scr);
    lv_label_set_text(lblStatus, "STOP");
    lv_obj_set_style_text_color(lblStatus, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_40, 0);
    lv_obj_align(lblStatus, LV_ALIGN_CENTER, 0, 10);

    /* 方向 */
    lblDirection = lv_label_create(scr);
    lv_label_set_text(lblDirection, "  -             +  ");
    lv_obj_set_style_text_color(lblDirection, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_text_font(lblDirection, &lv_font_montserrat_40, 0);
    lv_obj_align(lblDirection, LV_ALIGN_CENTER, 0, 75);

    /* 当前电机速度 */
    lblSpeedLevel = lv_label_create(scr);
    lv_obj_set_style_text_color(lblSpeedLevel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lblSpeedLevel, &lv_font_montserrat_40, 0);
    lv_obj_align(lblSpeedLevel, LV_ALIGN_CENTER, 0, 75);
    lv_label_set_text(lblSpeedLevel, "0");
}


static void lvglFlushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    if (w <= 0 || h <= 0) {
        lv_display_flush_ready(disp);
        return;
    }

    /* LVGL 的 RGB565 在内存里是小端(低字节在前), 而 ST7789 要高字节在前,
       在此原地交换字节后再下发。缓冲在 flush_ready 后会被 LVGL 复用,
       原地修改是安全的。 */
    uint32_t n = (uint32_t)w * h;
    for (uint32_t i = 0; i < n; ++i) {
        uint8_t t = px_map[i * 2 + 0];
        px_map[i * 2 + 0] = px_map[i * 2 + 1];
        px_map[i * 2 + 1] = t;
    }
    lcdDisplayBlit(s_display->config->id, area->x1, area->y1, w, h, px_map);

    /* 同步刷屏完成, 通知 LVGL */
    lv_display_flush_ready(disp);
}


static uint32_t lvglTickCb(void)
{
    /* esp_timer_get_time 返回 us, LVGL 需要 ms */
    /* uptime() 返回 ms */
    return (uint32_t)(upTime());
}

void displayManageSchedule(displayManage_t *display)
{
    /* LVGL 主循环: 单线程驱动, 不需要锁 */
    lv_timer_handler();
}


void displaySetSpeedLevel(int speedLevel)
{
    // lv_label_set_text(lbl_secs, buf);
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", speedLevel);
    lv_label_set_text(lblSpeedLevel, buf);
}

void displaySetIsPressed(bool isPressed)
{
    lv_label_set_text(lblStatus, isPressed ? "START" : "STOP");
}