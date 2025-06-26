#pragma once

#include "esp_err.h"

esp_err_t sdcard_init(int mosi_pin, int miso_pin, int sclk_pin, int cs_pin);
esp_err_t sdcard_list_files(void);
esp_err_t sdcard_open_file(const char *path, const char *mode, FILE **file_out);
esp_err_t sdcard_read_file(const char *path);
esp_err_t sdcard_write(const char *data, FILE *file_ptr);
esp_err_t sdcard_clear_file(const char *path);
esp_err_t sdcard_delete_file(const char *path);
esp_err_t sdcard_close_file(FILE* file_ptr);
esp_err_t sdcard_umount(void);