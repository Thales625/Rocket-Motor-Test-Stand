#include "sdcard.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

#include <dirent.h>

#define SD_MOUNT_POINT "/sdcard"

static const char *TAG = "sdcard";

static sdmmc_card_t *card;

esp_err_t sdcard_init(int mosi_pin, int miso_pin, int sclk_pin, int cs_pin) {
	esp_err_t ret;

	ESP_LOGI(TAG, "Initializing SD card");

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = SPI2_HOST;

	spi_bus_config_t bus_cfg = {
		.mosi_io_num = mosi_pin,
		.miso_io_num = miso_pin,
		.sclk_io_num = sclk_pin,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 4000,
	};
	ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to init SPI bus: %s", esp_err_to_name(ret));
		return ret;
	}

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = cs_pin;
	slot_config.host_id = host.slot;

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024
	};

	// sdmmc_card_t *card;
	ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to mount filesystem: %s", esp_err_to_name(ret));
		return ret;
	}

	sdmmc_card_print_info(stdout, card);

	return ESP_OK;
}

esp_err_t sdcard_list_files(void) {
	DIR *dir = opendir(SD_MOUNT_POINT);
	if (!dir) {
		printf("Failed to open directory mount path\n");
		return ESP_FAIL;
	}

	printf("Listing files\n");

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		printf("  %s\n", entry->d_name);
	}
	closedir(dir);

	return ESP_OK;
}

esp_err_t sdcard_open_file(const char *path, const char *mode, FILE **file_out) {
	if (file_out == NULL) return ESP_FAIL;

	char full_path[128];
	snprintf(full_path, sizeof(full_path), SD_MOUNT_POINT"/%s", path);

	*file_out = fopen(full_path, mode);

	if ((*file_out) == NULL) return ESP_FAIL;
	return ESP_OK;
}

esp_err_t sdcard_read_file(const char *path) {
	FILE *f_ptr;
	if (sdcard_open_file(path, "r", &f_ptr) == ESP_FAIL) {
		ESP_LOGE(TAG, "Failed to open file: %s", path);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "Reading file: %s", path);

	char line[128];
	while (fgets(line, sizeof(line), f_ptr)) {
		printf("%s", line);
	}

	if (sdcard_close_file(f_ptr) == ESP_FAIL) {
		ESP_LOGE(TAG, "Failed to close file: %s", path);
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t sdcard_write(const char *data, FILE *file_ptr) {
	return fprintf(file_ptr, "%s\n", data) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t sdcard_clear_file(const char *path) {
	FILE *f_ptr;
	if (sdcard_open_file(path, "w", &f_ptr) == ESP_FAIL) {
		ESP_LOGE(TAG, "Failed to open file: %s", path);
		return ESP_FAIL;
	}

	if (sdcard_close_file(f_ptr) == ESP_FAIL) {
		ESP_LOGE(TAG, "Failed to close file: %s", path);
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t sdcard_delete_file(const char *path) {
	char full_path[128];
	snprintf(full_path, sizeof(full_path), SD_MOUNT_POINT"/%s", path);

	if (remove(full_path) == 0) return ESP_OK;

	// perror("Error removing file");
	return ESP_FAIL;
}

esp_err_t sdcard_close_file(FILE* file_ptr) {
	return fclose(file_ptr) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t sdcard_umount(void) {
	return esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
}