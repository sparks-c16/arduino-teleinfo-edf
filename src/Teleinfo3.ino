/**
 * Ce programme a été élaboré à partir du code de M. Olivier LEBRUN réutilisé en grande partie pour réaliser un encodeur OWL CM180 (Micro+)
 * ainsi qu'à partir du code (+ carte électronique à relier au compteur) de M. Pascal CARDON pour la partie téléinfo
 * Onlinux a fourni des trames du OWL CM180 me permettant de faire les algo d'encodage (il a développer un code de décodage des trames)
 * Je remercie les auteurs. Ci-dessous les liens vers leur site internet.
 *=======================================================================================================================
 *ONLINUX :   Decode and parse the Oregon Scientific V3 radio data transmitted by OWL CM180 Energy sensor (433.92MHz)
 *References :
 *http://blog.onlinux.fr
 *https://github.com/onlinux/OWL-CMR180
 *http://domotique.web2diz.net/?p=11
 *http://www.domotique-info.fr/2014/05/recuperer-teleinformation-arduino/
 *http://connectingstuff.net/blog/encodage-protocoles-oregon-scientific-sur-arduino/
 *=======================================================================================================================
 * connectingStuff, Oregon Scientific v2.1 Emitter
 * http://connectingstuff.net/blog/encodage-protocoles-oregon-scientific-sur-arduino/
 * Copyright (C) 2013 olivier.lebrun@gmail.com
 *=======================================================================================================================
 *                                                        my_teleinfo
 *                                                 (c) 2012-2013 by
 *                                                  Script name : my_teleinfo
 *                                 http://www.domotique-info.fr/2014/05/recuperer-teleinformation-arduino/
 *
 *                VERSIONS HISTORY
 *	Version 1.00	30/11/2013	+ Original version
 *	Version 1.10    03/05/2015      + Manu : Small ajustment to variabilise the PIN numbers for Transmiter and Teleinfo
 *					See ref here : http://domotique.web2diz.net/?p=11#
 *
 * montage électronique conforme à http://www.domotique-info.fr/2014/05/recuperer-teleinformation-arduino/
 *=======================================================================================================================
 * Version 1.2.0	11/08/2017 by Christophe SESRIAULT
 * 		Ajustement des constantes pour le calcul d'instant et total
 * 		Refactoring
 * Version 1.3.0
 *    Emission (consommation) 'total' en kWh
 *    Transmission 'instant' à zéro au changement de tarif horaire
 *
 */

#include "Arduino.h"
#include "SoftwareSerial.h"

// #define OREGON
#define X10

#ifdef OREGON
#include "Oregon.h"
#endif

#ifdef X10
#include "X10.h"
#endif

#define TELEINFO_RX_PIN 2 // Pin réception téléinfo
#define TELEINFO_TX_PIN 5 // Pas utilisé (lecture seule)

// LED
#define LED_PIN 3

// TELEINFO DEFINITONS
#define MAX_FRAME_LENGTH 280
#define BUFFER_SIZE 25
#define START_FRAME 0x02
#define END_FRAME 0x03
#define START_LINE 0x0A
#define END_LINE 0x0D

#define HC 'C'
#define HP 'P'

char HHPHC;							// horaire période tarifaires (code)
int ISOUSC;             // intensité souscrite
int IINST;              // intensité instantanée en A
int IMAX;               // intensité maxi en A
int PAPP;               // puissance apparente en VA
unsigned long HCHC;     // compteur Heures Creuses en Wh
unsigned long HCHP;     // compteur Heures Pleines en Wh
String PTEC;            // Régime actuel : HPJB, HCJB, HPJW, HCJW, HPJR, HCJR
String ADCO;            // adresse compteur
String OPTARIF;         // option tarifaire
String MOTDETAT;        // status word

SoftwareSerial serialTeleinfo(TELEINFO_RX_PIN, TELEINFO_TX_PIN);

boolean teleInfoReceived;

boolean handleBuffer(char *bufferTeleinfo, int sequenceNumber);

char chksum(char *buffer, uint8_t len);

char ancienTarifHoraire = 0x00;   // ancien tarif horaire (détection changement)
char tarifHoraire;					      // tarif horaire en cours

//=================================================================================================================
void setupTeleinfo() {
	serialTeleinfo.begin(1200);

	ADCO = "------------";
	OPTARIF = "----";
	ISOUSC = 0;
	HCHC = 0L;  // compteur Heures Creuses en W
	HCHP = 0L;  // compteur Heures Pleines en W
	PTEC = "----";    // Régime actuel : HPJB, HCJB, HPJW, HCJW, HPJR, HCJR
	HHPHC = '-';
	IINST = 0;        // intensité instantanée en A
	IMAX = 0;         // intensité maxi en A
	PAPP = 0;         // puissance apparente en VA
	MOTDETAT = "------";
}

//=================================================================================================================
// Capture des trames de Teleinfo
//=================================================================================================================
boolean readTeleInfo() {
	int count = 0; // variable de comptage des caractères reçus
	char car = 0; // variable de mémorisation du caractère courant en réception

	char buffer[BUFFER_SIZE] = "";
	int bufferCount = 0;
	int checkSum;

	int sequenceNumber = 0;    // number of information group

	//--- wait for starting frame character
	while (car != START_FRAME) { // "Start Text" STX (002 h) is the beginning of the frame
		if (serialTeleinfo.available())
			car = serialTeleinfo.read() & 0x7F; // Serial.read() vide buffer au fur et à mesure
	} // fin while (tant que) pas caractère 0x02

	//  while (charIn != endFrame and comptChar<=maxFrameLen)
	while (car != END_FRAME) { // tant que des octets sont disponibles en lecture : on lit les caractères
														 // if (Serial.available())
		if (serialTeleinfo.available()) {
			car = serialTeleinfo.read() & 0x7F;
			// incrémente le compteur de caractère reçus
			count++;
			if (car == START_LINE) {
				bufferCount = 0;
			}
			buffer[bufferCount] = car;
			// on utilise une limite max pour éviter String trop long en cas erreur de réception
			// ajoute le caractère reçu au String pour les n premiers caractères
			if (car == END_LINE) {
				checkSum = buffer[bufferCount - 1];
				if (chksum(buffer, bufferCount) == checkSum) { // we clear the 1st character
					strncpy(&buffer[0], &buffer[1], bufferCount - 3);
					buffer[bufferCount - 3] = 0x00;
					sequenceNumber++;
					if (!handleBuffer(buffer, sequenceNumber)) {
						Serial.println(F("Sequence error ..."));
						return false;
					}
				} else {
					Serial.println(F("Checksum error ..."));
					return false;
				}
			} else
				bufferCount++;
		}
		if (count > MAX_FRAME_LENGTH) {
			Serial.println(F("Overflow error ..."));
			return false;
		}
	}

	return true;
}

boolean handleBuffer(char *bufferTeleinfo, int sequenceNumber) {
	// create a pointer to the first char after the space
	char* resultString = strchr(bufferTeleinfo, ' ') + 1;
	boolean isSequence;

	switch (sequenceNumber) {
	case 1:
		isSequence = bufferTeleinfo[0] == 'A';
		if (isSequence) {
			ADCO = String(resultString);
		}
		break;
	case 2:
		isSequence = bufferTeleinfo[0] == 'O';
		if (isSequence) {
			OPTARIF = String(resultString);
		}
		break;
	case 3:
		isSequence = bufferTeleinfo[1] == 'S';
		if (isSequence) {
			ISOUSC = atol(resultString);
		}
		break;
	case 4:
		isSequence = bufferTeleinfo[3] == 'C';
		if (isSequence) {
			HCHC = atol(resultString);
		}
		break;
	case 5:
		isSequence = bufferTeleinfo[3] == 'P';
		if (isSequence) {
			HCHP = atol(resultString);
		}
		break;
	case 6:
		isSequence = bufferTeleinfo[1] == 'T';
		if (isSequence) {
			PTEC = String(resultString);
		}
		break;
	case 7:
		isSequence = bufferTeleinfo[1] == 'I';
		if (isSequence) {
			IINST = atol(resultString);
		}
		break;
	case 8:
		isSequence = bufferTeleinfo[1] == 'M';
		if (isSequence) {
			IMAX = atol(resultString);
		}
		break;
	case 9:
		isSequence = bufferTeleinfo[1] == 'A';
		if (isSequence) {
			PAPP = atol(resultString);
		}
		break;
	case 10:
		isSequence = bufferTeleinfo[1] == 'H';
		if (isSequence) {
			HHPHC = resultString[0];
		}
		break;
	case 11:
		isSequence = bufferTeleinfo[1] == 'O';
		if (isSequence) {
			MOTDETAT = String(resultString);
		}
		break;
	}
#ifdef debug
	if(!isSequence)
	{
		Serial.print(F("Out of sequence ..."));
		Serial.println(bufferTeleinfo);
	}
#endif
	return isSequence;
}

//=================================================================================================================
// Calculates teleinfo Checksum
//=================================================================================================================
char chksum(char *buffer, uint8_t len) {
	char sum = 0;
	for (int i = 1; i < len - 2; i++) {
		sum = sum + buffer[i];
	}
	sum = (sum & 0x3F) + 0x20;
	return sum;
}

//=================================================================================================================
// This function displays the TeleInfo Internal counters
// It's usefull for debug purpose
//=================================================================================================================
void displayTeleInfo() {
	// TODO Lire l'état d'un pin muni d'un cavalier pour tracer ou pas...

	/*
	 ADCO 270622224349 B
	 OPTARIF HC.. <
	 ISOUSC 30 9
	 HCHC 014460852 $
	 HCHP 012506372 -
	 PTEC HP..
	 IINST 002 Y
	 IMAX 035 G
	 PAPP 00520 (
	 HHPHC C .
	 MOTDETAT 000000 B
	 */

	Serial.print(F("ADCO "));
	Serial.println(ADCO);
	Serial.print(F("OPTARIF "));
	Serial.println(OPTARIF);
	Serial.print(F("ISOUSC "));
	Serial.println(ISOUSC);
	Serial.print(F("HCHC "));
	Serial.println(HCHC);
	Serial.print(F("HCHP "));
	Serial.println(HCHP);
	Serial.print(F("PTEC "));
	Serial.println(PTEC);
	Serial.print(F("IINST "));
	Serial.println(IINST);
	Serial.print(F("IMAX "));
	Serial.println(IMAX);
	Serial.print(F("PAPP "));
	Serial.println(PAPP);
	Serial.print(F("HHPHC "));
	Serial.println(HHPHC);
	Serial.print(F("MOTDETAT "));
	Serial.println(MOTDETAT);
	Serial.println();
}

/**
 * SETUP.
 */
void setup() {
	// LED OFF
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	Serial.begin(115200);

	setupTeleinfo();
}

#ifdef OREGON
/**
 * MAIN LOOP OREGON.
 */
void loop_oregon() {
	teleInfoReceived = readTeleInfo();
	if (teleInfoReceived) {
		// LED allumée le temps d'émettre...
		digitalWrite(LED_PIN, HIGH);

		serialTeleinfo.end(); // NECESSAIRE !! Arrête les interruptions de SoftwareSerial (lecture du port téléinfo) pendant l'émission des trames OWL

		tarifHoraire = PTEC.charAt(1);

		if (tarifHoraire != ancienTarifHoraire) {
			// Lors du changement de tarif horaire, on envoi une valeur de consommation instantannée nulle sur l'ancien tarif
			encodeOregon_OWL_CM180(tarifHoraire == HC ? HP : HC, tarifHoraire == HC ? HCHP : HCHC, 0);
			sendOregon();
			digitalWrite(LED_PIN, LOW);
			delay(5000);

			// Deux fois, en cas de non réception...
			digitalWrite(LED_PIN, HIGH);
			encodeOregon_OWL_CM180(tarifHoraire == HC ? HP : HC, tarifHoraire == HC ? HCHP : HCHC, 0);
			sendOregon();
			digitalWrite(LED_PIN, LOW);
			delay(5000);

			digitalWrite(LED_PIN, HIGH);
		}

		// Envoi de la consommation courante et de l'index correspondant au tarif horaire en cours
		encodeOregon_OWL_CM180(tarifHoraire, tarifHoraire == HC ? HCHC : HCHP, PAPP);
		sendOregon();

		ancienTarifHoraire = tarifHoraire;

		// Trace
		displayTeleInfo();  // Console pour voir les trames téléinfo

		digitalWrite(LED_PIN, LOW);

		// Ajout d'un delai après chaque trame envoyée pour éviter d'envoyer en permanence des informations
		// de créer des interférences TODO Prévoir une mise en veille du µProcesseur
		delay(30000);

		serialTeleinfo.begin(1200); // Relance les interuptions pour la lecture du port téléinfo
	}
}
#endif


#ifdef X10
/**
 * MAIN LOOP X10.
 */
void loop_x10() {

}
#endif

/**
 * MAIN LOOP.
 */
void loop() {
#ifdef OREGON
	loop_oregon();
#endif
#ifdef X10
	loop_x10();
#endif
}

