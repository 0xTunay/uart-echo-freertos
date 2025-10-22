# UART ECHO FREERTOS  

*You can read the full documentation in [Doxygen](https://0xtunay.github.io/uart-echo-freertos/files.html)*  

---

## Overview  
This project demonstrates a **simple UART echo server** built using **FreeRTOS** on the **ESP-IDF v5.4.1** framework.  
It shows how to work with UART events in an interrupt-driven manner, handle received data, and execute commands received over UART to control a GPIO pin.

The example is minimal, easy to extend, and can serve as a base for more advanced communication or control applications.

---

## Features  
- Uses **UART0** for serial communication.  
- Implements **event-driven reception** with the UART driver queue.  
- Echoes any received data back to the sender.  
- Supports simple text commands:
  - `ON` — turns on the GPIO pin.  
  - `OFF` — turns off the GPIO pin.  
- Fully asynchronous operation based on **FreeRTOS tasks and queues**.  
- Uses ESP-IDF logging system (`ESP_LOGI`, `ESP_LOGE`, etc.) for structured debugging output.  

---

## Hardware Requirements  
- **ESP32** or compatible board  
- USB connection for UART communication  
- **GPIO2** connected to an LED (optional, for testing command control)  

---

## Software Requirements  
- **ESP-IDF v5.4.1** or newer  
- **CMake** and **Ninja** (included in ESP-IDF)  
- FreeRTOS (included in ESP-IDF)  

---

## Build and Flash  

```bash
idf.py set-target esp32
idf.py build
idf.py flash
idf.py monitor
```

After flashing, open the monitor and try sending `ON` or `OFF` through UART (baud rate **115200**).  
The board will echo your message and control the GPIO pin accordingly.

---



### Tasks  
- **uart_event_task**  
  Handles UART events, reads received data, echoes it back, and sends commands to the processing queue.  

- **command_processor_task**  
  Waits for commands from the queue and processes them.  
  Currently supports turning a GPIO pin ON or OFF.  

### Queues  
- **uart_event_queue** — managed by UART driver for event-driven reception.  
- **command_queue** — used to pass received commands to the processing task.  

---

## Configuration  

| Macro | Description | Default |
|--------|--------------|----------|
| `UART_PORT` | UART port number | `UART_NUM_0` |
| `BUF_SIZE` | UART RX buffer size | `1024` bytes |
| `GPIO_CONTROL` | GPIO pin for ON/OFF control | `GPIO_NUM_2` |
| `UART_QUEUE_SIZE` | UART event queue size | `20` |