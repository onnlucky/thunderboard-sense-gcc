#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <native_gecko.h>
#include <gecko_configuration.h>
#include <em_device.h>
#include <em_gpio.h>
#include <bg_dfu.h>
#include <aat.h>

#include "InitDevice.h"
#include "gatt_db.h"

#define MAX_CONNECTIONS 1
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];

// Gecko configuration parameters, see gecko_configuration.h
static const gecko_configuration_t config = {
    .config_flags = 0,
    .sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
    .bluetooth.max_connections = MAX_CONNECTIONS,
    .bluetooth.heap = bluetooth_stack_heap,
    .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
    .gattdb = &bg_gattdb_data,
};

// LED pin defines for thunderboard sense kit (D11, D12 for leds)
#define LED0_bank       gpioPortD
#define LED0_pin        11
#define LED1_bank       gpioPortD
#define LED1_pin        12

#define TIMER_LED_BLINK 1

static void hardware_init() {
    // configure LED pins as outputs
    GPIO_PinModeSet(LED0_bank, LED0_pin, gpioModePushPull, 1);
    GPIO_PinModeSet(LED1_bank, LED1_pin, gpioModePushPull, 1);

    // initial state: both LEDs off
    GPIO_PinOutSet(LED0_bank, LED0_pin);
    GPIO_PinOutSet(LED1_bank, LED1_pin);
}

static void services_init() { }

static void ledBlink() {
    static int toggle = 0;

    if (toggle == 0) {
        GPIO_PinOutSet(LED0_bank, LED0_pin);
        GPIO_PinOutClear(LED1_bank, LED1_pin);
        toggle = 1;
    } else {
        GPIO_PinOutClear(LED0_bank, LED0_pin);
        GPIO_PinOutSet(LED1_bank, LED1_pin);
        toggle = 0;
    }
}

int main() {
    // Initialize Blue Gecko module
    enter_DefaultMode_from_RESET();
    gecko_init(&config);

    // Hardware initialization
    //hardware_init();

    // Services initialization
    services_init();

    while (1) {
        /* Event pointer for handling events */
        struct gecko_cmd_packet* evt;

        /* Check for stack event. */
        evt = gecko_wait_event();

        /* Handle events */
        switch (BGLIB_MSG_ID(evt->header)) {

            /* This boot event is generated when the system boots up after reset.
             * Here the system is set to start advertising immediately after boot procedure. */
            case gecko_evt_system_boot_id:
                hardware_init();
                /* Set advertising parameters. 100ms advertisement interval. All channels used.
                 * The first two parameters are minimum and maximum advertising interval, both in
                 * units of (milliseconds * 1.6). The third parameter '7' sets advertising on all channels. */
                gecko_cmd_le_gap_set_adv_parameters(160, 160, 7);

                /* Start general advertising and enable connections. */
                gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

                /* start 500ms timer for LED blinking */
                gecko_cmd_hardware_set_soft_timer(32768/2, TIMER_LED_BLINK, 0);

            break;

            case gecko_evt_le_connection_closed_id:
                /* Restart advertising after client has disconnected */
                gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
                break;

            case gecko_evt_hardware_soft_timer_id:
                switch (evt->data.evt_hardware_soft_timer.handle) {
                    case TIMER_LED_BLINK: ledBlink(); break;
                    default: break;
                }
                break;

            default: break;
        }
    }
}
