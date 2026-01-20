# SD_test

## 目的
測試以下api
(1)esp_err_t control_reader_load(const char path, control_info_tout); (在frame_system_init裡面完成)
(2)esp_err_t read_frame(table_frame_t playerbuffer)

## 改掉的部分
### 1. frame_reader.c
frame_reader_init 補加上讀取前兩個byte

### 2. readframe.c