/**
 * Oregon Scientific Library.
 * (c)SESRIAULT Christophe
 * 02/09/2017
 */

#include "Oregon.h"

#include "Arduino.h"

byte oregonMessageBuffer[13];  //  OWL180

/**
 * \brief    Send logical "1" over RF
 * \details  a one bit be represented by an on-to-off transition
 * \         of the RF signal at the middle of a clock period.
 * \         Remenber, the Oregon v2.1 protocol add an inverted bit first
 */
inline void sendOne(void) {
	SEND_HIGH();
	delayMicroseconds(TIME);
	SEND_LOW();
	delayMicroseconds(TIME);
}

/**
 * \brief    Send logical "0" over RF
 * \details  azero bit be represented by an off-to-on transition
 * \         of the RF signal at the middle of a clock period.
 * \         Remenber, the Oregon v2.1 protocol add an inverted bit first
 */
inline void sendZero(void) {
	SEND_LOW();
	delayMicroseconds(TIME);
	SEND_HIGH();
	delayMicroseconds(TIME);
}

/**
 * \brief    Send preamble
 * \details  The preamble consists of 10 X "1" bits (minimum)
 */
inline void sendPreamble(void) {
	for (byte i = 0; i < 10; ++i) { // OWL CM180
		sendOne();
	}
}

/**
 * \brief    Send a buffer over RF
 * \param    data   Data to send
 * \param    size   size of data to send
 */
void sendData(byte *data, byte size) {
	for (byte i = 0; i < size; ++i) {
		bitRead(data[i], 0) ? sendOne() : sendZero();
		bitRead(data[i], 1) ? sendOne() : sendZero();
		bitRead(data[i], 2) ? sendOne() : sendZero();
		bitRead(data[i], 3) ? sendOne() : sendZero();
		bitRead(data[i], 4) ? sendOne() : sendZero();
		bitRead(data[i], 5) ? sendOne() : sendZero();
		bitRead(data[i], 6) ? sendOne() : sendZero();
		bitRead(data[i], 7) ? sendOne() : sendZero();
	}
}

/**
 * \brief    Send postamble
 */
inline void sendPostamble(void) {
	for (byte i = 0; i < 4; ++i) { // OWL CM180
		sendZero();
	}
	SEND_LOW();
	delayMicroseconds(TIME);
}

/**
 * \brief    Send an Oregon message
 * \param    data   The Oregon message
 */
void sendOregon(byte *data, byte size) {
	sendPreamble();
	sendData(data, size);
	sendPostamble();
}

void encodeOregon_OWL_CM180(char tarif, unsigned long long totalValue, unsigned long instantValue) {
	unsigned long instant = (255L + (long(7 + instantValue) / 16L) * 509L) / 512L;
	unsigned long long total = totalValue * 230LL / 1000LL;

	oregonMessageBuffer[0] = 0x62; // imposé
	oregonMessageBuffer[1] = 0x80; // GH   G= non décodé par RFXCOM,  H = Count

	// Si Heure Creuse compteur 3D, sinon Heure Pleine compteur 3C
	oregonMessageBuffer[2] = tarif == 'C' ? 0x3D : 0x3C;
	// Puis les valeurs 'instant' et 'total'
	oregonMessageBuffer[3] = (instant & 0x0F) << 4;
	oregonMessageBuffer[4] = (instant >> 4) & 0xFF;
	oregonMessageBuffer[5] = ((instant >> 12) & 0x0F) + ((total & 0x0F) << 4);
	oregonMessageBuffer[6] = (total >> 4) & 0xFF;
	oregonMessageBuffer[7] = (total >> 12) & 0xFF;
	oregonMessageBuffer[8] = (total >> 20) & 0xFF;
	oregonMessageBuffer[9] = (total >> 28) & 0xFF;
	oregonMessageBuffer[10] = (total >> 36) & 0xFF;

	unsigned long checksum = 0;
	for (byte i = 0; i < 11; i++) {
		checksum += long(oregonMessageBuffer[i] & 0x0F) + long(oregonMessageBuffer[i] >> 4);
	}
	checksum -= 2; // = =b*16^2 + d*16+ a ou [b d a]

	// Enfin la checksum
	oregonMessageBuffer[11] = ((checksum & 0x0F) << 4) + ((checksum >> 8) & 0x0F);
	oregonMessageBuffer[12] = (checksum >> 4) & 0x0F;
}

void sendOregon() {
	sendOregon(oregonMessageBuffer, sizeof(oregonMessageBuffer));
}
