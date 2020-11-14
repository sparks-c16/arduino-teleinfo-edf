/**
 * Oregon Scientific Library.
 * (c)SESRIAULT Christophe
 * 02/09/2017
 */

#ifndef Oregon_h
#define Oregon_h

// OREGON DEFINITONS
#define OREGON_RADIO_TX_PIN 4 // Emetteur 433 MHZ

#define TIME 488L
#define TWOTIME (TIME * 2L)

#define SEND_HIGH() digitalWrite(OREGON_RADIO_TX_PIN, HIGH)
#define SEND_LOW() digitalWrite(OREGON_RADIO_TX_PIN, LOW)

void encodeOregon_OWL_CM180(char tarif, unsigned long long totalValue, unsigned long instantValue);

void sendOregon();

#endif
