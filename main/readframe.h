#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "table_frame.h"
#include "channel_info.h"

/* 添加全局变量声明 */
extern uint8_t g_of_num;
extern uint8_t g_strip_num;
extern uint8_t *g_led_num;

ch_info_t get_channel_info(void);

esp_err_t frame_system_init(const char *control_path,
                            const char *frame_path);

esp_err_t read_frame(table_frame_t *playerbuffer);
esp_err_t read_frame_ts(table_frame_t *playerbuffer, uint64_t ts);

esp_err_t frame_reset(void);
void      frame_system_deinit(void);

uint8_t frame_get_of_num(void);
uint8_t frame_get_strip_num(void);
uint8_t frame_get_led_num(uint8_t strip_index);
uint32_t frame_get_frame_num(void);
uint32_t frame_get_timestamp(uint32_t frame_index);