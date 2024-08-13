#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "./garageState.h"

#define GPIO_INPUT_IO_0     CONFIG_GARAGE_GPIO_REED_0
#define GPIO_INPUT_IO_1     CONFIG_GARAGE_GPIO_REED_1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

#define GPIO_OUTPUT_IO_0     CONFIG_GARAGE_GPIO_DOOR_SIGNAL_0
#define GPIO_OUTPUT_IO_1     CONFIG_GARAGE_GPIO_DOOR_SIGNAL_1
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

#define GARAGE_SPEED_SECONDS 12

garage_state_t garage_state[GARAGE_COUNT];
void (*garage_state_changed)(int garage, garage_door_state_t state);

static const char *TAG = "garageState";

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void garage_switch_changed(int garage, uint8_t newReedState)
{
    garage_state[garage].reedSwitchState = newReedState;

    if (newReedState == SWITCH_GARAGE_CLOSED) {
        if (garage_state[garage].state == GARAGE_STATE_CLOSED) {
            return;
        }
        garage_state[garage].state = GARAGE_STATE_CLOSED;
        garage_state[garage].secondsActivated = 0;
    } else {
        if (garage_state[garage].state == GARAGE_STATE_OPENING) {
            return;
        }
        garage_state[garage].state = GARAGE_STATE_OPENING;
        garage_state[garage].secondsActivated = GARAGE_SPEED_SECONDS;
    }
    if (garage_state_changed != NULL) {
        garage_state_changed(garage, garage_state[garage].state);
    }
}

static void gpio_queue_worker(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            uint8_t state = gpio_get_level(io_num);
            if (io_num == GPIO_INPUT_IO_0) {
                if (garage_state[0].reedSwitchState != state){
                    garage_switch_changed(0, state);
                    ESP_LOGI(TAG, "Garage 0 state changed");
                }
            } else if (io_num == GPIO_INPUT_IO_1) {
                if (garage_state[1].reedSwitchState != state){
                    garage_switch_changed(1, state);
                    ESP_LOGI(TAG, "Garage 1 state changed");
                }
            } else {
                // ESP_LOGI(TAG, "Unknown GPIO state change " io_num);
            }
            // printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

static void timer_callback(void* arg);

void init_garage_state(void){
    
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_queue_worker, "gpio_queue_worker", 2048, NULL, 10, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000000));
    
    // initial state
    for (int i = 0; i < GARAGE_COUNT; i++) {
        uint8_t gpio = GPIO_INPUT_IO_0;
        switch(i) {
            case 0: gpio = GPIO_INPUT_IO_0; break;
            case 1: gpio = GPIO_INPUT_IO_1; break;
            default: break;
        }
        uint8_t state = gpio_get_level(gpio);
        garage_state[i].state = state == 0 ? GARAGE_STATE_CLOSED : GARAGE_STATE_OPEN;
        garage_state[i].reedSwitchState = state;
    }
}

static void timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    for (int i = 0; i < GARAGE_COUNT; i++) {
        if (garage_state[i].secondsActivated > 0) {
            garage_state[i].secondsActivated--;
            if (garage_state[i].secondsActivated == 0) {
                garage_state[i].state = garage_state[i].reedSwitchState == SWITCH_GARAGE_CLOSED ? GARAGE_STATE_CLOSED : GARAGE_STATE_OPEN;
                
                if (garage_state_changed != NULL) {
                    garage_state_changed(i, garage_state[i].state);
                }
            }
        }
    }
}

void garage_trigger_door_signal(int garage) {
    
    if (garage_state[garage].state == GARAGE_STATE_CLOSING || garage_state[garage].state == GARAGE_STATE_OPENING) {
        return;
    }

    uint8_t gpio = GPIO_OUTPUT_IO_0;
    switch(garage) {
        case 0: gpio = GPIO_OUTPUT_IO_0; break;
        case 1: gpio = GPIO_OUTPUT_IO_1; break;
        default: break;
    }

    gpio_set_level(gpio, 1);
    
    usleep(.5 * 1000000UL);

    gpio_set_level(gpio, 0);

    
    garage_state[garage].secondsActivated = GARAGE_SPEED_SECONDS;
    if (garage_state[garage].state == GARAGE_STATE_OPEN) {
        garage_state[garage].state = GARAGE_STATE_CLOSING;
        garage_state_changed(garage, garage_state[garage].state);
    }
    if (garage_state[garage].state == GARAGE_STATE_CLOSED) {
        garage_state[garage].state = GARAGE_STATE_OPENING;
        garage_state_changed(garage, garage_state[garage].state);
    }
}