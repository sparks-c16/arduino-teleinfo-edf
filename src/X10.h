/**
 * X10 Radio Library.
 * (c)SESRIAULT Christophe
 * 02/09/2017
 */

#ifndef X10_h
#define X10_h

#include "Arduino.h"

#define X10_RF_SB_LONG          8960 	// Start burts (leader) = 9ms
#define X10_RF_SB_SHORT         4500 	//Start silecence (leader) = 4,5 ms
#define X10_RF_BIT_LONG         1120 	// Bit 1 pulse length
#define X10_RF_BIT_SHORT         560 	// Bit 1 pulse length

// X10 DEFINITONS
#define X10_RADIO_TX_PIN 4 // Emetteur 433 MHZ

void rfx_meter(byte rfxm_address, byte rfxm_packet_type, long rfxm_value);

#endif
