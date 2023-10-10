/*------------------------------------------------------------------------------
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates).
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   1.Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   2.Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   3.Neither the name of Arm nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *------------------------------------------------------------------------------
 * Name:    Blinky.c
 * Purpose: LED Flasher
 *----------------------------------------------------------------------------*/

#include "cmsis_os2.h"                  // ARM::CMSIS:RTOS:Keil RTX5
#include "Board_LED.h"                  // Board Support:LED
#include "Board_Buttons.h"              // Board Support:Buttons
#include "rl_net.h"
#include <stdio.h>
#include <string.h>

static osThreadId_t tid_thrLED;         // Thread id of thread: LED
static osThreadId_t tid_thrBUT;         // Thread id of thread: BUT
int32_t udp_sock;

/*------------------------------------------------------------------------------
  thrLED: blink LED
 *----------------------------------------------------------------------------*/
__NO_RETURN void thrLED (void *argument) {
  uint32_t active_flag = 1U;

  for (;;) {
    if (osThreadFlagsWait (1U, osFlagsWaitAny, 0U) == 1U) {
      active_flag ^=1U; 
    }

    if (active_flag == 1U){
      //LED_On (0U);                                // Switch LED on
      osDelay (500U);                             // Delay 500 ms
      //LED_Off (0U);                               // Switch off
      osDelay (500U);                             // Delay 500 ms
    }
    else {
      osDelay (500U);                             // Delay 500 ms
    }
  }
}

/*------------------------------------------------------------------------------
  thrBUT: check button state
 *----------------------------------------------------------------------------*/
__NO_RETURN static void thrBUT(void *argument) {
  uint32_t last;
  uint32_t state;

  for (;;) {
    state = (Buttons_GetState () & 1U);           // Get pressed button state
    if (state != last){
      if (state == 1){
        osThreadFlagsSet (tid_thrLED, 1U);        // Set flag to thrLED
      }
    }
    last = state;
    osDelay (100U);
  }
}

static void print_address (void) {
  uint8_t ip4_addr[NET_ADDR_IP4_LEN];
  char    ip4_ascii[16];

  if (netIF_GetOption (NET_IF_CLASS_ETH, netIF_OptionIP4_Address,
                                         ip4_addr, sizeof(ip4_addr)) == netOK) {
  /* Print IPv4 network address */
    netIP_ntoa (NET_ADDR_IP4, ip4_addr, ip4_ascii, sizeof(ip4_ascii));
    printf ("IP4=%-15s\n", ip4_ascii);    
  }
}

/* IP address change notification */
void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {

  (void)if_num;
  (void)val;
  (void)len;

  if (option == NET_DHCP_OPTION_IP_ADDRESS) {
    /* IP address change, trigger LCD update */
    print_address ();
  }
}

// Notify the user application about UDP socket events.
uint32_t udp_cb_func (int32_t socket, const NET_ADDR *addr, const uint8_t *buf, uint32_t len) {
 
  // Data received
  if ((buf[0] == 0x01) && (len == 2)) {
    // Switch LEDs on and off
     LED_SetOut(buf[1]);
  }
  return (0);
}

// Send UDP data to destination client.
void send_udp_data (void) {
	
uint8_t buffer[]="Hello Vives";
	
 
  if (udp_sock > 0) {
    // Example
    // IPv4 address: 192.168.137.1
    NET_ADDR addr = { NET_ADDR_IP4, 1001, 192, 168, 137, 1 };
    
    uint8_t *sendbuf;
 
    sendbuf = netUDP_GetBuffer (sizeof(buffer));
    memcpy(sendbuf,buffer,sizeof(buffer));
 
    netUDP_Send (udp_sock, &addr, sendbuf, sizeof(buffer));
    
	}
}


/*------------------------------------------------------------------------------
 * Application main thread
 *----------------------------------------------------------------------------*/
void app_main (void *argument) {

  LED_Initialize ();                    // Initialize LEDs
  Buttons_Initialize ();                // Initialize Buttons
	netInitialize();
	print_address();
	
	// Initialize UDP socket and open port 1001
  udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) {
    netUDP_Open (udp_sock, 1001);
  }

	

  tid_thrLED = osThreadNew (thrLED, NULL, NULL);  // Create LED thread
  if (tid_thrLED == NULL) { /* add error handling */ }

  tid_thrBUT = osThreadNew (thrBUT, NULL, NULL);  // Create BUT thread
  if (tid_thrBUT == NULL) { /* add error handling */ }
	
	osDelay(6000);
	send_udp_data();

  osThreadExit();
}
