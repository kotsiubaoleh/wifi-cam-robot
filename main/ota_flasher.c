#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "errno.h"
#include "esp_wifi.h"

#include "led.h"

//#define CHECK_OTA_APP_VERSION
#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */
#define FIRMWARE_UPDATE_URL "https://192.168.0.102:8000/build/wifi-cam-robot.bin"

static const char *TAG = "ota_flasher";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    led_blink_stop();
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

static void __attribute__((noreturn)) terminate_task(void)
{
    ESP_LOGI(TAG, "Termination OTA flasher task...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

static void ota_task(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA task");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = {
        .url = FIRMWARE_UPDATE_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 8000,
        .keep_alive_enable = true,
    };

    config.skip_cert_common_name_check = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    ESP_LOGI(TAG, "Requesting %s...", config.url);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {

                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    
                    esp_app_desc_t running_app_info;
                    esp_ota_get_partition_description(running, &running_app_info);
                    
                    #ifdef CHECK_OTA_APP_VERSION
                        // check current version with downloading
                        
                        ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);

                        const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                        esp_app_desc_t invalid_app_info;
                        if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                            ESP_LOGI(TAG, "Last invalid firmwar e version: %s", invalid_app_info.version);
                        }

                        // check current version with last invalid partition
                        if (last_invalid_app != NULL) {
                            if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                                ESP_LOGW(TAG, "New version is the same as invalid version.");
                                ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                                ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                                http_cleanup(client);
                                terminate_task();
                            }
                        }

                        // compare current version with new version
                        if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                            http_cleanup(client);
                            terminate_task();
                        }
                    #endif
                    #ifndef CHECK_OTA_APP_VERSION
                        if (memcmp(new_app_info.app_elf_sha256, running_app_info.app_elf_sha256, 32) == 0) {
                            ESP_LOGW(TAG, "Current running app sha256 is the same as a new. We will not continue the update.");
                            http_cleanup(client);
                            terminate_task();
                        }
                    #endif

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        esp_ota_abort(update_handle);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                    led_blink_start();
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                    http_cleanup(client);
                    esp_ota_abort(update_handle);
                    task_fatal_error();
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                esp_ota_abort(update_handle);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
	    /*
	     * As esp_http_client_read never returns negative error code, we rely on
	     * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        esp_ota_abort(update_handle);
        task_fatal_error();
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    led_blink_stop();
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return;
}

void init_ota_flasher(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    // skip diagnostic and mark active firmware as valid
    esp_ota_mark_app_valid_cancel_rollback();

    // esnsure that wifi powes saving is not activated
    esp_wifi_set_ps(WIFI_PS_NONE);

    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
}
