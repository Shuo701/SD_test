# SD_test

## 目的
測試以下api
(1)esp_err_t control_reader_load(const char path, control_info_tout); (在frame_system_init裡面完成)
(2)esp_err_t read_frame(table_frame_t playerbuffer)

## 改掉的部分
### 1. frame_reader.c
frame_reader_init 補加上讀取前兩個byte

### 2. readframe.c
多下面三個全域變數＆函式讓main抓
'''
uint8_t g_of_num = 0;
uint8_t g_strip_num = 0;
uint8_t *g_led_num = NULL;

uint8_t frame_get_of_num(void);
uint8_t frame_get_strip_num(void);
uint8_t frame_get_led_num(uint8_t strip_index);
uint32_t frame_get_frame_num(void);
uint32_t frame_get_timestamp(uint32_t frame_index);
'''
然後多明確聲明函數
'''
static void sd_reader_task(void *arg);
static uint32_t find_idx_by_ts(uint64_t ts);
'''
xTaskCreate的stack開更大
4096 --> 16384