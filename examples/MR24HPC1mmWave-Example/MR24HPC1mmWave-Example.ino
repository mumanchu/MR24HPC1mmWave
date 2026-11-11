///////////////////////////////////////////////////////////////////////////////
// Example for mumanchu's Seeed Studio 24GHz mmWave Sensor MR24HPC1 Library
// Developed using Visual Studio 2026 with the Visual Micro Extension
// Copyright (C) https://muman.ch + https://github.com/mumanchu, 2026.11.11
// If you re-use this software please include the above copyright notice
//
// For details see
// https://github.com/mumanchu/MR24HPC1mmWave
// https://muman.ch/muman/index.htm?muman-mr24hpc1-mmwave-sensor.htm
// https://www.visualmicro.com/


// Comment this out to hide the debug output
#define DEBUG


#include "MR24HPC1mmWave.h"
MR24HPC1mmWave sensor;


// The usual 'Serial' is for the Serial Monitor so you can see the data

// 'Serial1' is used for the mmWave sensor's UART, but the pins may be 
// different on each microcontroller board. Here are some examples...

#if defined(ARDUINO_SAM_DUE)
// The Arduino Due has Serial1 on TX1 (18) and RX1 (19), connect these to RX 
// and TX on the sensor
// !!!NOTE!!!  
// If you select "No Optimisation" for the Arduino Due, then the build may
// fail with this message:
//  "HardwareSerial.h:26: undefined reference to vtable for HardwareSerial"
// It's a problem with the boards package code. Set it to "Default Optimisation",
// or use the fix described here:
// https://learn.adafruit.com/proper-step-debugging-atsamd21-arduino-zero-m0/paths-and-optimizations#fixing-some-core-files-2469468-17
#endif

#if defined(ARDUINO_ARCH_STM32)
// On the STM32F103 Nucleo board, 'Serial' is the USB port
// so we use USART1, TX=D8 (PA9), RX=D2 (PA10) for the mmWave sensor
//HardwareSerial Serial1(USART1);
Uart Serial1(USART1);
#endif

#if defined(ARDUINO_ARCH_AVR)
// SoftwareSerial will not work at 115200 baud on AVR processors
//TODO
#endif

#if defined(ARDUINO_SAMD_ZERO)
// Arduino Zero has 'Serial1' on pin 0 (RX) and pin 1 (TX), so you don't need 
// to do anything. 'Serial' is the USB port, so connect your Serial Monitor to 
// the port called 'Atmel Corp. EDBG CMSIS-DAP'
#endif

#if defined(ARDUINO_ARCH_ESP32)
// For the ESP32s, Serial1's pins can be defined
// see the code in setup() below
#endif


// Get ready...
void setup() 
{
	// I use different pins for Serial logging on the STM32 Nucleo-64 boards,
	// else Serial is the USB port which is also used for download
	#ifdef ARDUINO_NUCLEO_64
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);
	#endif

	// 'Serial' is the port for the Serial Monitor
	Serial.begin(115200);
	delay(1000);

	// PuTTY clear screen and scrollback
	// comment this out if not using PuTTY
	Serial.print("\033[2J\033[H\033[3J");

	Serial.println("MR24HPC1mmWave Started...\r\n");
	Serial.flush();

	// 'Serial1' is the port for the mmWave sensor
	Serial1.begin(115200);

	#if defined(ARDUINO_ARCH_ESP32)
	// for the ESP32s, Serial1's pins can be defined
	Serial1.setPins(6 /*rxPin*/, 7 /*txPin*/);
	#endif

	if (!sensor.begin(&Serial1)) {
		Serial.println("sensor.begin() failed");
		Serial.flush();
		while (1) yield();
	}

	if (!sensor.setOpenFunctionMode(0)) {
		Serial.println("sensor.setOpenFunctionMode() failed");
		Serial.flush();
		while (1) yield();
	}
	sensor.resetModule();

	Serial.println();
	#ifdef DEBUG
	sensor.printParameters();
	#endif
}


// Go!
void loop() 
{
	if (sensor.receiveNextMessage()) {
		bool changed;
		if (sensor.processReportMessage(&changed) && changed) {
			Serial.println(sensor.presence ? "Occupied" : "Unoccupied");
			// 0 = unoccupied/unknown; 1 = static person; 2 = moving person
			if (sensor.motion > 0)
				Serial.println(sensor.motion == 1 ? "Static" : "Moving");
		}
		Serial.flush();
	}

	// check we have received a message from the sensor at least every minute
	// the 'heartbeat' message is sent every minute when there are no reports
	static bool notResponding = false;
	if ((millis() - sensor.lastReportTime) > 65000) {
		// sensor off line
		if (!notResponding) {
			notResponding = true;
			Serial.println("*** Sensor not responding ***");
			Serial.flush();
		}
	}
	else {
		// sensor back on line
		notResponding = false;
	}
}


