/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

// Rotary Dial Switch 10 Positions RM3HAF-10R Definition
#define BIN_SWITCH_COM_0 6
#define BIN_SWITCH_COM_1 26
#define BIN_SWITCH_1 15
#define BIN_SWITCH_2 5
#define BIN_SWITCH_4 27
#define BIN_SWITCH_8 7

// maximum number of operating systems, in case you have multiple images in your grub to boot
#define SO_MAX_NUM 3

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};


static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void cdc_task(void);

uint8_t read_switch_value()
{
  uint8_t bit_1 = (uint8_t)(gpio_get(BIN_SWITCH_1) ? 0 : 1);
  uint8_t bit_2 = (uint8_t)(gpio_get(BIN_SWITCH_2) ? 0 : 2);
  uint8_t bit_4 = (uint8_t)(gpio_get(BIN_SWITCH_4) ? 0 : 4);
  uint8_t bit_8 = (uint8_t)(gpio_get(BIN_SWITCH_8) ? 0 : 8);
  uint8_t sum = bit_1 + bit_2 + bit_4 + bit_8;
  if (SO_MAX_NUM <= 1) sum = 0;
  sum = (sum % SO_MAX_NUM);
  return (uint8_t)(48 + sum);
}

/*------------- MAIN -------------*/
void configure_gpio(uint pin)
{
  gpio_init(pin);                 // Inicializa el GPIO
  gpio_set_dir(pin, GPIO_IN);      // Configura como entrada
  gpio_pull_up(pin);               // Activa la resistencia pull-up interna
}

int main(void)
{
  configure_gpio(BIN_SWITCH_1);
  configure_gpio(BIN_SWITCH_2);
  configure_gpio(BIN_SWITCH_4);
  configure_gpio(BIN_SWITCH_8);
  gpio_init(BIN_SWITCH_COM_0);
  gpio_init(BIN_SWITCH_COM_1);
  gpio_set_dir(BIN_SWITCH_COM_0, GPIO_OUT);
  gpio_set_dir(BIN_SWITCH_COM_1, GPIO_OUT);
  gpio_put(BIN_SWITCH_COM_0, 0);
  gpio_put(BIN_SWITCH_COM_1, 0);

  board_init();
  tusb_init();

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();
    cdc_task();
  }

  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void)
{
  // connected() check for DTR bit
  // Most but not all terminal client set this when making connection
  // if ( tud_cdc_connected() )
  {
    // connected and there are data available
    if ( tud_cdc_available() )
    {
      // read datas
      char buf[64];
      uint32_t count = tud_cdc_read(buf, sizeof(buf));
      (void) count;

      // Echo back
      // Note: Skip echo by commenting out write() and write_flush()
      // for throughput test e.g
      //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
      tud_cdc_write(buf, count);
      tud_cdc_write_flush();
    }
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;
  (void) rts;

  // TODO set some indicator
  if ( dtr )
  {
    // Terminal connected
  }else
  {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
