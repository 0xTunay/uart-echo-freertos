/**
 * @file uart-echo-freertos.c
 * @brief Simple UART echo server with FreeRTOS and event handling (ESP-IDF v5.4.1 example).
 *
 * This example demonstrates:
 *  - Configuring UART with interrupt-driven reception using the event queue.
 *  - Echoing incoming data back to the sender.
 *  - Processing basic text commands ("ON"/"OFF") to control a GPIO pin.
 *
 * @author Tunay
 * @date 2025-10-21
 *
 * @note for ESP-IDF v5.4.1, UART0 by default.
 */

/**
 * @def UART_PORT
 * @brief UART port number used for echo (default: UART0)
 */

/**
 * @def BUF_SIZE
 * @brief Size of UART RX buffer in bytes
 */

/**
 * @def GPIO_CONTROL
 * @brief GPIO pin used for ON/OFF control via UART commands
 */

/**
 * @def UART_QUEUE_SIZE
 * @brief Size of the UART event queue
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT       UART_NUM_0
#define BUF_SIZE        1024
#define GPIO_CONTROL    GPIO_NUM_2
#define UART_QUEUE_SIZE 20

/** Tag for ESP log */
static const char *TAG = "UART_ECHO";

/** Queue for commands received via UART */
static QueueHandle_t command_queue;

/** UART event queue provided by driver */
static QueueHandle_t uart_event_queue;

/**
 * @brief Task that handles UART events
 *
 * This task receives events from the UART driver and processes them:
 *  - Reads incoming data
 *  - Echoes data back to sender
 *  - Sends command strings to the command processor task
 *
 * @param pvParameters Task parameters (not used)
 */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate memory for UART buffer");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (xQueueReceive(uart_event_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: {
                    size_t rx_size = event.size;
                    if (rx_size == 0) break;
                    if (rx_size >= BUF_SIZE) rx_size = BUF_SIZE - 1;

                    int len = uart_read_bytes(UART_PORT, data, rx_size, pdMS_TO_TICKS(20));
                    if (len > 0) {
                        data[len] = '\0';
                        ESP_LOGI(TAG, "Received: %s", (char *)data);

                        /* Echo back */
                        uart_write_bytes(UART_PORT, (const char *)data, len);

                        /* Send command copy to processor */
                        char *cmd_copy = strdup((char *)data);
                        if (cmd_copy) {
                            if (xQueueSend(command_queue, &cmd_copy, pdMS_TO_TICKS(10)) != pdPASS) {
                                free(cmd_copy);
                                ESP_LOGE(TAG, "Failed to send command to queue");
                            }
                        } else {
                            ESP_LOGE(TAG, "Failed to duplicate command string");
                        }
                    }
                    break;
                }

                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_event_queue);
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART ring buffer full");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_event_queue);
                    break;

                case UART_FRAME_ERR:
                case UART_PARITY_ERR:
                    ESP_LOGW(TAG, "UART error event: %d", event.type);
                    break;

                default:
                    ESP_LOGI(TAG, "Unhandled UART event: %d", event.type);
                    break;
            }
        }
    }

    free(data);
    vTaskDelete(NULL);
}

/**
 * @brief Task that processes UART commands
 *
 * Supported commands:
 *  - "ON": turns GPIO_CONTROL pin ON
 *  - "OFF": turns GPIO_CONTROL pin OFF
 *
 * @param pvParameters Task parameters (not used)
 */
static void command_processor_task(void *pvParameters)
{
    char *cmd = NULL;

    while (1) {
        if (xQueueReceive(command_queue, &cmd, portMAX_DELAY) == pdPASS) {
            /* Trim trailing newline/carriage return */
            char *end = cmd + strlen(cmd) - 1;
            while (end >= cmd && (*end == '\n' || *end == '\r')) *end-- = '\0';

            ESP_LOGI(TAG, "Processing command: %s", cmd);

            if (strcmp(cmd, "ON") == 0) {
                gpio_set_level(GPIO_CONTROL, 1);
                ESP_LOGI(TAG, "GPIO turned ON");
            } else if (strcmp(cmd, "OFF") == 0) {
                gpio_set_level(GPIO_CONTROL, 0);
                ESP_LOGI(TAG, "GPIO turned OFF");
            } else {
                ESP_LOGI(TAG, "Unknown command: %s", cmd);
            }

            free(cmd);
        }
    }

    vTaskDelete(NULL);
}

/**
 * @brief Entry point of the application
 *
 * Initializes GPIO, configures UART, installs driver, creates queues, and starts tasks.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting UART Echo Example");

    /* GPIO setup */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_CONTROL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(GPIO_CONTROL, 0);

    /* UART parameters */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

    /* Use default pins for UART0 */
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    /* Install driver */
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE * 2, BUF_SIZE * 2, UART_QUEUE_SIZE, &uart_event_queue, 0));

    /* Create command queue */
    command_queue = xQueueCreate(5, sizeof(char *));
    if (!command_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        return;
    }

    /* Start tasks */
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL);
    xTaskCreate(command_processor_task, "command_processor_task", 4096, NULL, 9, NULL);

    ESP_LOGI(TAG, "UART Echo server started");
}