#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#define LED_PIN 4

static TaskHandle_t xBlinkTask = NULL;
static volatile bool blinking = false;


static void blink_task (void *pvParameters) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << LED_PIN;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (blinking) {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(1 / portTICK_RATE_MS);
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(300 / portTICK_RATE_MS);
        }
    }
}

void led_blink_start() {
    if (!xBlinkTask) {
        xTaskCreate(blink_task, "Blink", 1024 * 8, NULL, 1, &xBlinkTask);
    }
    blinking = true;
    xTaskNotifyGive(xBlinkTask);
}

void led_blink_stop() {
    blinking = false;
}
