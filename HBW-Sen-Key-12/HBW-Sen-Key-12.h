/*
 * HBW-Sen-Key-12.h
 *
 *  Created on: 11.12.2017
 *      Author: Felix
 */

#ifndef HBW_SEN_KEY_12_H_
#define HBW_SEN_KEY_12_H_

// HBW Device id
#define HMW_DEVICETYPE 0x95
#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0100

// HBW Config
#define NUM_CHANNELS 12
#define NUM_LINKS 36		//TODO Anzahl links könnte aus EEProm berechnet werden
#define LINKADDRESSSTART 0x40

// PIN def
#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED 13        // Signal-LED
// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define PIN_ARRAY uint8_t pins[NUM_CHANNELS] = {A0, A1, A2, A3, A4, A5, 5, 6, 7, 9, 10, 11};

// Structs
// Config as C++ structure (without direct links)
struct hbw_config_key {
  uint8_t input_locked:1;      // 0x07:0    0=LOCKED, 1=UNLOCKED
  uint8_t inverted:1;          // 0x07:1
  uint8_t pullup:1;            // 0x07:2
  uint8_t       :5;            // 0x07:3-7
  byte long_press_time;       // 0x08
};

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_key keys[NUM_CHANNELS]; // 0x07-0x1E
} hbwconfig;

//
#endif /* HBW_SEN_KEY_12_H_ */
