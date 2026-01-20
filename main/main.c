#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "control_reader.h"
#include "readframe.h"
#include "table_frame.h"

static const char *TAG = "frame_test";

#define MOUNT_POINT "/sd"
#define CONTROL_PATH "0:/control.dat"
#define FRAME_PATH "0:/frame.dat"

#define PIN_CMD  15
#define PIN_CLK  14
#define PIN_D0   2
#define PIN_D1   4
#define PIN_D2   12
#define PIN_D3   13

void frame_test_task(void *pvParameters)
{
    esp_err_t ret;
    table_frame_t frame_buf;
    
    ESP_LOGI(TAG, "Starting frame API test task");
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 1. 初始化 frame system（會內部呼叫 control_reader_load）
    if (frame_system_init(CONTROL_PATH, FRAME_PATH) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init frame system");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Frame system initialized");

    uint8_t of_num = frame_get_of_num();
    uint8_t strip_num = frame_get_strip_num();
    uint32_t frame_num = frame_get_frame_num();

    
    // 2. 順序讀取所有 frames
    ESP_LOGI(TAG, "Reading frames sequentially...");
    uint32_t frame_count = 0;

    ESP_LOGI(TAG, "Configuration: OF=%d, Strips=%d, Total frames=%lu", 
             of_num, strip_num, frame_num);
    
    while (1) {
        ret = read_frame(&frame_buf);
        if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGI(TAG, "Reached end of frames");
            break;
        }
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read frame %lu", frame_count);
            break;
        }
        
        frame_count++;

        // 每 100 frames 印一次 log
        if (frame_count == 3000){
            ESP_LOGI(TAG, "Read frame %lu, ts=%llu, fade=%d", 
                     frame_count, frame_buf.timestamp, frame_buf.fade);
            ESP_LOGI(TAG, "  Optical Fibers (%d):", of_num);
            for (int i = 0; i < of_num; i++) {
                ESP_LOGI(TAG, "    OF[%d]: G=%03d, R=%03d, B=%03d", 
                        i,
                        frame_buf.data.pca9955b[i].g,
                        frame_buf.data.pca9955b[i].r,
                        frame_buf.data.pca9955b[i].b);
                
                if (i % 10 == 9) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            }
            // 輸出 LED 燈條 (WS2812B) 顏色
            ESP_LOGI(TAG, "  LED Strips (%d):", strip_num);
            for (int strip = 0; strip < strip_num; strip++) {
                uint8_t led_num = frame_get_led_num(strip);
                ESP_LOGI(TAG, "    Strip %d (%d LEDs):", strip, led_num);
                
                for (int led = 0; led < led_num; led++) {
                    ESP_LOGI(TAG, "      LED[%d][%d]: G=%03d, R=%03d, B=%03d", 
                            strip, led,
                            frame_buf.data.ws2812b[strip][led].g,
                            frame_buf.data.ws2812b[strip][led].r,
                            frame_buf.data.ws2812b[strip][led].b);
                    if (led % 5 == 4) {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    ESP_LOGI(TAG, "Total frames read: %lu", frame_count);
    
    // 清理
    frame_system_deinit();
    
    ESP_LOGI(TAG, "Frame API test completed");
    
    // 任務完成，自我刪除
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing SD card");
    
    // 初始化 SDMMC host
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    
    // 配置 SD卡插槽
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;  // 使用 4-line 模式
    slot_config.clk = PIN_CLK;
    slot_config.cmd = PIN_CMD;
    slot_config.d0 = PIN_D0;
    slot_config.d1 = PIN_D1;
    slot_config.d2 = PIN_D2;
    slot_config.d3 = PIN_D3;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    // 配置 FAT 檔案系統掛載選項
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_card_t *card;
    
    // 掛載 SD 卡
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    ESP_LOGI(TAG, "SD card mounted successfully");
    
    // 印出 SD 卡資訊
    sdmmc_card_print_info(stdout, card);
    
    // 檢查檔案是否存在
    ESP_LOGI(TAG, "Checking files...");
    
    FILE *f = fopen("/sd/control.dat", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "/sd/control.dat not found!");
        
        // 列出 SD 卡根目錄內容
        DIR *dir = opendir("/sd");
        if (dir) {
            struct dirent *entry;
            ESP_LOGI(TAG, "Files in /sd:");
            while ((entry = readdir(dir)) != NULL) {
                ESP_LOGI(TAG, "  - %s", entry->d_name);
            }
            closedir(dir);
        }
    } else {
        ESP_LOGI(TAG, "/sd/control.dat found");
        fclose(f);
    }
    
    f = fopen("/sd/frame.dat", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "/sd/frame.dat not found!");
    } else {
        ESP_LOGI(TAG, "/sd/frame.dat found");
        fclose(f);
    }
    
    // 建立測試任務（pinned to Core 0）
    xTaskCreatePinnedToCore(
        frame_test_task,    // 任務函數
        "frame_test",       // 任務名稱
        16384,               // 堆疊大小
        NULL,               // 參數
        5,                  // 優先級
        NULL,               // 任務句柄
        0                   // Core 0
    );
    
    ESP_LOGI(TAG, "Frame test task created");
}
