/**
 * X10 Radio Library.
 * (c)SESRIAULT Christophe
 * 02/09/2017
 */

#include "X10.h"

#include "Arduino.h"

byte x10buffer[5];

void sendX10RfBit(unsigned int databit) {
	digitalWrite(X10_RADIO_TX_PIN, HIGH);
	delayMicroseconds(X10_RF_BIT_SHORT);
	digitalWrite(X10_RADIO_TX_PIN, LOW);
	delayMicroseconds(X10_RF_BIT_SHORT);
	if (databit)
		delayMicroseconds(X10_RF_BIT_LONG);
}

void sendX10RfByte(byte data) {
	for (int i = 7; i >= 0; i--) { // send bits from byte
		sendX10RfBit((bitRead(data,i) == 1));
	}
}

void SEND_HIGH() {
	digitalWrite(X10_RADIO_TX_PIN, HIGH);
}

void SEND_LOW() {
	digitalWrite(X10_RADIO_TX_PIN, LOW);
}

void sendCommand(byte size1) {
	digitalWrite(X10_RADIO_TX_PIN, HIGH);
	delayMicroseconds(X10_RF_SB_LONG);
	digitalWrite(X10_RADIO_TX_PIN, LOW);
	delayMicroseconds(X10_RF_SB_SHORT);

	for (int u = 0; u <= size1; u++) {
		sendX10RfByte(x10buffer[u]);
	}

	sendX10RfBit(1);
}

void x10Switch(char house_code, byte unit_code, byte command) {
	switch (house_code) {
	case 'A':
		x10buffer[0] = B0110;
		break;
	case 'B':
		x10buffer[0] = B0111;
		break;
	case 'C':
		x10buffer[0] = B0100;
		break;
	case 'D':
		x10buffer[0] = B0101;
		break;
	case 'E':
		x10buffer[0] = B1000;
		break;
	case 'F':
		x10buffer[0] = B1001;
		break;
	case 'G':
		x10buffer[0] = B1010;
		break;
	case 'H':
		x10buffer[0] = B1011;
		break;
	case 'I':
		x10buffer[0] = B1110;
		break;
	case 'J':
		x10buffer[0] = B1111;
		break;
	case 'K':
		x10buffer[0] = B1100;
		break;
	case 'L':
		x10buffer[0] = B1101;
		break;
	case 'M':
		x10buffer[0] = B0000;
		break;
	case 'N':
		x10buffer[0] = B0001;
		break;
	case 'O':
		x10buffer[0] = B0010;
		break;
	case 'P':
		x10buffer[0] = B0011;
		break;
	default:
		x10buffer[0] = 0;
		break;
	}
	x10buffer[0] = x10buffer[0] << 4; // House code goes into the upper nibble

	switch (command) {
	case 0:
		x10buffer[2] = (x10buffer[2] | 0x20);
		break;
	case 1:
		x10buffer[2] = (x10buffer[2] & 0x20);
		break;
	}

	// Set unit number
	unit_code = unit_code - 1;
	bitWrite(x10buffer[2], 6, bitRead(unit_code,2));
	bitWrite(x10buffer[2], 3, bitRead(unit_code,1));
	bitWrite(x10buffer[2], 4, bitRead(unit_code,0));
	bitWrite(x10buffer[0], 2, bitRead(unit_code,3));

	// Set parity
	x10buffer[1] = ~x10buffer[0];
	x10buffer[3] = ~x10buffer[2];

	sendCommand(3);
}

void x10Security(byte address, byte command) {
	//byte x10buff[3]; // Set message buffer 4 bytes
	x10buffer[0] = address;
	x10buffer[1] = (~x10buffer[0] & 0xF) + (x10buffer[0] & 0xF0); // Calculate byte1 (byte 1 complement
	x10buffer[2] = command;
	x10buffer[3] = ~x10buffer[2];

	// x10buff[4] = code; // Couldn't get 48 bit security working.
	// if((x10buff[4] % 2) == 0) { x10buff[5] = 0;} //Calc even parity
	// else { x10buff[5] = 0x80;}
	sendCommand(3);
}

/**
 *
 */
void rfx_meter(byte rfxm_address, byte rfxm_packet_type, long rfxm_value) {

	x10buffer[0] = rfxm_address;

	x10buffer[1] = (~x10buffer[0] & 0xF0) + (x10buffer[0] & 0xF); // Calculate byte1 (byte 1 complement upper nibble of byte0)

	if (rfxm_value > 0xFFFFFF) {
		rfxm_value = 0;
	} // We only have 3 byte for data. Is overflowed set to 0
		// Packet type goed into MSB nibble of byte 5. Max 15 (B1111) allowed
		// Use switch case to filter invalid data types

	switch (rfxm_packet_type) {
	case 0x00: //Normal. Put counter values in byte 4,2 and 3
		x10buffer[4] = (byte) ((rfxm_value >> 16) & 0xff);
		x10buffer[2] = (byte) ((rfxm_value >> 8) & 0xff);
		x10buffer[3] = (byte) (rfxm_value & 0xff);
		break;

	case 0x01: // New interval time set. Byte 2 should be filled with interval
		switch (rfxm_value) {
		case 0x01:
			break;	// 30sec
		case 0x02:
			break;	// 01min
		case 0x04:
			break;	// 06min (RFXpower = 05min)
		case 0x08:
			break;	// 12min (RFXpower = 10min)
		case 0x10:
			break; 	// 15min
		case 0x20:
			break; 	// 30min
		case 0x40:
			break; 	// 45min
		case 0x80:
			break; 	// 60min
		default:
			rfxm_value = 0x01; // Set to 30 sec if no valid option is found
		}
		x10buffer[2] = rfxm_value;
		break;

	case 0x02: // calibrate value in <counter value> in µsec.
		x10buffer[4] = (byte) ((rfxm_value >> 16) & 0xff);
		x10buffer[2] = (byte) ((rfxm_value >> 8) & 0xff);
		x10buffer[3] = (byte) (rfxm_value & 0xff);
		break;

	case 0x03:
		break; // new address set
	case 0x04:
		break; // counter value reset to zero
	case 0x0B: // counter value set
		x10buffer[4] = (byte) ((rfxm_value >> 16) & 0xff);
		x10buffer[2] = (byte) ((rfxm_value >> 8) & 0xff);
		x10buffer[3] = (byte) (rfxm_value & 0xff);
		break;
	case 0x0C:
		break; // set interval mode within 5 seconds
	case 0x0D:
		break; // calibration mode within 5 seconds
	case 0x0E:
		break; // set address mode within 5 seconds
	case 0x0F: // identification packet (byte 2 = address, byte 3 = interval)
		switch (rfxm_value) {
		case 0x01:
			break;	//30sec
		case 0x02:
			break;	//01min
		case 0x04:
			break;	//06min (RFXpower = 05min)
		case 0x08:
			break;	//12min (RFXpower = 10min)
		case 0x10:
			break; // 15min1
		case 0x20:
			break; // 30min
		case 0x40:
			break; // 45min
		case 0x80:
			break; // 60min
		default:
			rfxm_value = 0x01; // Set to 30 sec if no valid option is found
		}
		x10buffer[2] = rfxm_address;
		x10buffer[3] = rfxm_value;
		break;
	default: //Unknown packet type. Set packet type to zero and set counter to rfxm_value
		rfxm_packet_type = 0;
		x10buffer[4] = (byte) ((rfxm_value >> 16) & 0xff);
		x10buffer[2] = (byte) ((rfxm_value >> 8) & 0xff);
		x10buffer[3] = (byte) (rfxm_value & 0xff);
	}
	x10buffer[5] = (rfxm_packet_type << 4); 		// Packet type goes into byte 5's upper nibble.

	// Calculate parity which
	byte parity = ~(((x10buffer[0] & 0XF0) >> 4) + (x10buffer[0] & 0XF) + ((x10buffer[1] & 0XF0) >> 4) + (x10buffer[1] & 0XF)
			+ ((x10buffer[2] & 0XF0) >> 4) + (x10buffer[2] & 0XF) + ((x10buffer[3] & 0XF0) >> 4) + (x10buffer[3] & 0XF)
			+ ((x10buffer[4] & 0XF0) >> 4) + (x10buffer[4] & 0XF) + ((x10buffer[5] & 0XF0) >> 4));
	x10buffer[5] = (x10buffer[5] & 0xf0) + (parity & 0XF);

	sendCommand(5); // Send byte to be broadcasted
}

