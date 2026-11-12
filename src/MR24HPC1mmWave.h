#pragma once

///////////////////////////////////////////////////////////////////////////////
// Arduino Library for MR24HPC1 Seeed Studio 24GHz mmWave Sensor
// Human Static Presence Module Lite 
// Copyright (C) https://muman.ch + https://github.com/mumanchu, 2026.11.13
// If you re-use this software, please include the above copyright notice
// Released under the terms of the BSD 3-Clause License
// 
// For details see github
// https://github.com/mumanchu/MR24HPC1mmWave


#include <Stream.h>
#include <string.h>
#include <stdio.h>


// For debug output
#ifdef DEBUG
#define MR24HPC1_SERIAL Serial
#define MR24HPC1_ASSERT(b)  \
	if (!(b)) { MR24HPC1_SERIAL.println("ASSERT failed"); MR24HPC1_SERIAL.flush(); return false; }
#define MR24HPC1_LOGERROR(s) \
	MR24HPC1_SERIAL.print("Error: "); MR24HPC1_SERIAL.println(s); MR24HPC1_SERIAL.flush()

// Binary message logging
void mr24hpc1_logMsg(bool tx, const byte * msg, uint msglen)
{
	MR24HPC1_SERIAL.print(tx ? ">>> " : "<<< ");
	char hex[5];
	for (uint i = 0; i < msglen; ++i) {
		sprintf(hex, "%02X ", msg[i]);
		MR24HPC1_SERIAL.print(hex);
	}
	MR24HPC1_SERIAL.println();
	MR24HPC1_SERIAL.flush();
}
#define MR24HPC1_LOGMSG(tx, msg, msglen) mr24hpc1_logMsg(tx, msg, msglen)
#define MR24HPC1_LOG(s) MR24HPC1_SERIAL.println(s); MR24HPC1_SERIAL.flush()
#else
#define MR24HPC1_SERIAL
#define MR24HPC1_ASSERT(b)
#define MR24HPC1_LOGERROR(s)
#define MR24HPC1_LOGMSG(tx, msg, msglen)
#define MR24HPC1_LOG(s)
#endif

typedef unsigned int uint;
typedef unsigned long ulong;


class MR24HPC1mmWave
{
protected:
	Stream* stream;

	// maintained by receiveNextMessage()
	byte rxMsg[100];
	uint rxMsgLen;
	uint rxDataLen;
	bool rxGotMsg;
	uint rxTimestamp;

	int rxState;
	uint rxIndex;
	uint rxLen;

public:
	// Initialised in begin(), normally 1
	uint protocolVersion;	// can be used if there are differences in the protocol

	// Data from Standard Mode reports
	uint presence;			// 0 = unoccupied/unknown; 1 = occupied
	uint motion;			// 0 = unoccupied/unknown; 1 = static person; 2 = moving person
	uint motionAmplitude;	// 0 = unoccupied; 1 = static person; 2..100 motion amplitude
	uint motionDirection;	// 0 = static; 1 = moving closer; 2 = moving away

	// Data from Underlying Open Function Mode mode reports
	uint staticEnergy;		// 0..250
	uint staticDistance;	// 0..8 in 0.5m steps (0..4m)
	uint motionEnergy;		// 0..250
	uint motionDistance;	// 0..6 in 0.5m steps (0..3m)
	uint motionSpeed;		// 0x00 = none, 0x01..0x09 = approaching, 0x0A = static, 
							// 0x0B..0x14 = moving away, 0.5m/s steps
	ulong lastReportTime;	// time of last report message, including 'heartbeat'

	bool begin(Stream* stream);
	bool getProductInformation(char* productModel, char* productId, 
		char* hardwareModel, char* firmwareVersion, uint* protocolVersion);
	bool resetModule();
	bool getInitStatus(uint* initComplete);

	// Messages
	bool processReportMessage(bool* changed);
	bool receiveNextMessage(const byte* cmd = NULL);

	// The following methods are only needed for one-time configuration 
	#ifdef MR24HPC1_ENABLE_CONFIG

	// Standard Mode, also known as Scene Mode

	// Presets
	// note: new document has AREA and BEDROOM swapped AREA = 1, BEDROOM = 4
	// which version is correct?
	// sorted by 4/3.5/3/2.5? swap BATHROON & AREA, not BEDROOM & AREA
	// sets the 'detection radius'
	enum SCENE : uint
	{
		SCENE_NONE = 0,			// scene not set
		SCENE_LIVING_ROOM = 1,	// 4 .. 4.5m
		SCENE_BEDROOM = 2,		// 3.5 .. 4mm
		SCENE_BATHROOM = 3,		// 2.5 .. 3m
		SCENE_AREA = 4			// 3 .. 3.5m
	};
	bool setScene(SCENE scene);
	bool getScene(SCENE* scene);
	bool setSensitivity(uint sensitivity);
	bool getSensitivity(uint* sensitivity);

	bool setUnoccupiedDelay(uint unoccupiedDelay);
	bool getUnoccupiedDelay(uint* unoccupiedDelay);
	bool getPresence(uint* presence);
	bool getMotion(uint* motion);
	bool getMotionAmplitude(uint* motionAmplitude);
	bool getMotionDirection(uint* motionDirection);

	// Underlying Open Function Mode (i.e. Advanced mode)
	bool setOpenFunctionMode(bool openFunctionMode);
	bool getOpenFunctionMode(bool* openFunctionMode);
	bool getStaticEnergy(uint* staticEnergy);
	bool getMotionEnergy(uint* motionEnergy);
	bool getStaticDistance(uint* staticDistance);
	bool getMotionDistance(uint* motionDisatnce);
	bool getMotionSpeed(uint* motionSpeed);

	// Custom Mode, allows a more detailed configuration
	bool setCustomMode(uint customMode);
	bool getCustomMode(uint* customMode);
	bool saveCustomMode();
	bool setStaticEnergyThreshold(uint threshold);
	bool getStaticEnergyThreshold(uint* threshold);
	bool setStaticBoundary(uint boundary);
	bool getStaticBoundary(uint* boundary);
	bool setMotionEnergyThreshold(uint threshold);
	bool getMotionEnergyThreshold(uint* threshold);
	bool setMotionBoundary(uint boundary);
	bool getMotionBoundary(uint* boundary);
	bool setMotionDuration(ulong milliseconds);
	bool getMotionDuration(ulong* milliseconds);
	bool setMotionToStaticDelay(ulong milliseconds);
	bool getMotionToStaticDelay(ulong* milliseconds);
	bool setCustomUnoccupiedDelay(ulong milliseconds);
	bool getCustomUnoccupiedDelay(ulong* milliseconds);

	// Conversion functions
	// centimetres (cm) are used so we don't need float
	uint convertUnoccupiedDelayToSeconds(uint unoccupiedDelay);
	uint convertSensitivityToCm(uint sensitivity);
	uint convertDistanceToCm(uint distance);
	int convertMotionToCmPerSecond(uint motion);

#ifdef DEBUG
	void printParameters();
#endif
	#endif		// #ifdef MR24HPC1_ENABLE_CONFIG

protected:
	bool sendMessageAndGetResponse(const byte* msg, uint msglen);
	byte checksumMsg(const byte* msg, uint msglen);
	bool read1Byte(const byte* msg, uint msglen, uint* value);

	#ifdef MR24HPC1_ENABLE_CONFIG
	bool write1Byte(byte* msg, uint msglen, uint value);
	bool write2Bytes(byte* msg, uint msglen, uint value);
	bool write4Bytes(byte* msg, uint msglen, ulong value);
	bool read2Bytes(const byte* msg, uint msglen, uint* value);
	bool read4Bytes(const byte* msg, uint msglen, ulong* value);
	#endif
};


// Initialise
bool MR24HPC1mmWave::begin(Stream* stream)
{
	this->stream = stream;

	protocolVersion = 0;
	presence = 0;
	motion = 0;
	motionAmplitude = 0;
	motionDirection = 0;
	staticEnergy = 0;
	staticDistance = 0;
	motionEnergy = 0;
	motionDistance = 0;
	motionSpeed = 0;
	lastReportTime = millis();
	
	// get the protocolVersion
	return getProductInformation(NULL, NULL, NULL, NULL, &protocolVersion);
}

// Read the board's model and version information 
// - Each char* buffer MUST be 16 bytes long, e.g. 
//   char productModel[16], productId[16], hardwareModel[16], firmwareVersion[16];
//   getProductInformation(productModel, productId, hardwareModel, firmwareVersion);
// - Pointers may be NULL if the data is not needed, e.g.
//   getProductInformation(productModel, NULL, hardwareModel, NULL);
// TODO In old FW versions, some of these may be binary values, not ASCII.
bool MR24HPC1mmWave::getProductInformation(char* productModel, char* productId, 
	char* hardwareModel, char* firmwareVersion,	uint* protocolVersion)
{
	if (productModel)
		*productModel = '\0';
	if (productId)
		*productId = '\0';
	if (hardwareModel)
		*hardwareModel = '\0';
	if (firmwareVersion)
		*firmwareVersion = '\0';
	if (protocolVersion)
		*protocolVersion = 0;

	byte msg[] = { 0x53, 0x59, 0x02, 0xff, 0x00, 0x01, 0x0F, 0xff, 0x54, 0x43 };

	// send several commands
	// product model = 0xA1
	// product id = 0xA2
	// hardware model = 0xA3
	// firmware version = 0xA4
	// protocol version = 0xA5
	for (uint cmd = 0xA1; cmd <= 0xA5; ++cmd) {
		char* dest = NULL;
		switch (cmd) {
		case 0xA1:
			dest = productModel;
			break;
		case 0xA2:
			dest = productId;
			break;
		case 0xA3:
			dest = hardwareModel;
			break;
		case 0xA4:
			dest = firmwareVersion;
			break;
		case 0xA5:			// protocol version
			if (protocolVersion == NULL)
				break;		// not needed, all done
		}
		// skip the data we don't need
		if (dest == NULL && cmd != 0xA5)
			continue;
		msg[3] = (byte)cmd;
		msg[7] = checksumMsg(msg, sizeof(msg));
		if (!sendMessageAndGetResponse(msg, sizeof(msg)))
			return false;

		if (rxMsg[3] != cmd) {
			MR24HPC1_LOGERROR("wrong response");
			return false;
		}
		// last one is a binary value
		if (cmd == 0xA5) {
			*protocolVersion = rxMsg[6];
			break;			// all done
		}
		// character values, max 15 characters
		if (rxDataLen > 15) {
			MR24HPC1_LOGERROR("product info truncated");
			rxDataLen = 15;
		}
		strncpy(dest, (char*)(rxMsg + 6), rxDataLen);
		dest[rxDataLen] = '\0';
	}
	return true;
}

// Reset the module and wait until it's ready to continue
// uses getInitStatus() for polling, it normally takes just under 2 seconds
// I'm not sure what it does, it's not described in the User Manual
bool MR24HPC1mmWave::resetModule()
{
	static const byte msg[] = { 0x53, 0x59, 0x01, 0x02, 0x00, 0x01, 0x0F, 0xBF, 0x54, 0x43 };
	if (!sendMessageAndGetResponse(msg, sizeof(msg)))
		return false;
	delay(1000);
	// wait for init complete, with 5 retries = 5 second timeout
	for (uint retries = 0; retries < 5; ++retries) {
		uint initComplete;
		if (getInitStatus(&initComplete) && initComplete > 0)
			return true;
	}
	MR24HPC1_LOGERROR("resetModule timeout");
	return false;
}

// Is initialization complete? 
// See also 'Initialization Complete' report message in processReportMessage().
// initComplete : 0 = busy; 1 = complete
bool MR24HPC1mmWave::getInitStatus(uint* initComplete)
{
	static const byte msg[] = { 0x53, 0x59, 0x05, 0x81, 0x00, 0x01, 0x0F, 0x42, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), initComplete);
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// The following methods are only needed for one-time configuration

#ifdef MR24HPC1_ENABLE_CONFIG

// Select a scene/scanario preset
// this selects the maximum detection distance for motion detection
bool MR24HPC1mmWave::setScene(SCENE scene)
{
	byte msg[] = { 0x53, 0x59, 0x05, 0x07, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), scene);
}
bool MR24HPC1mmWave::getScene(SCENE* scene)
{
	static const byte msg[] = { 0x53, 0x59, 0x05, 0x87, 0x00, 0x01, 0x0F, 0x48, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), (uint*)scene);
}

/*
Sets the maximum detection distance for a human body in the static
(stationary) state
0 = <not set>
1 = 2.5m
2 = 3m
3 = 4m
see convertDistanceToCm()
*/
bool MR24HPC1mmWave::setSensitivity(uint sensitivity)
{
	MR24HPC1_ASSERT(sensitivity > 0 && sensitivity < 4);
	byte msg[] = { 0x53, 0x59, 0x05, 0x08, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), sensitivity);
}
bool MR24HPC1mmWave::getSensitivity(uint* sensitivity)
{
	static const byte msg[] = { 0x53, 0x59, 0x05, 0x88, 0x00, 0x01, 0x0F, 0x49, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), sensitivity);
}

/*
The time interval from "someone present" to "no one present"
Unoccupied Delay :
0 = none
1 = 10s
2 = 30s
3 = 1min
4 = 2min
5 = 5min
6 = 10min
7 = 30min
8 = 60min
see convertUnoccupiedDelayToSeconds()
*/
bool MR24HPC1mmWave::setUnoccupiedDelay(uint unoccupiedDelay)
{
	MR24HPC1_ASSERT(unoccupiedDelay > 0 && unoccupiedDelay < 9);
	byte msg[] = { 0x53, 0x59, 0x80, 0x0A, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), unoccupiedDelay);
}
bool MR24HPC1mmWave::getUnoccupiedDelay(uint* unoccupiedDelay)
{
	static const byte msg[] = { 0x53, 0x59, 0x80, 0x8A, 0x00, 0x01, 0x0F, 0xC6, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), unoccupiedDelay);
}


////////////////////////////////////////////////////////
// Standard Function Mode, measured states and values

/*
get occupied state for moving or non-moving (static) people
presence :
0x00 = unoccupied, 0x01 = occupied
OR
0 = presence, 1 = absence
*/
bool MR24HPC1mmWave::getPresence(uint* presence)
{
	static const byte msg[] = { 0x53, 0x59, 0x80, 0x81, 0x00, 0x01, 0x0F, 0xBD, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), presence);
}

/*
motion :
0x00 = none
0x01 = motionless / static / stationary
0x02 = active / moving
*/
bool MR24HPC1mmWave::getMotion(uint* motion)
{
	static const byte msg[] = { 0x53, 0x59, 0x80, 0x82, 0x00, 0x01, 0x0F, 0xBE, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), motion);
}

/*
motionAmplitude :
0..100
0 = nobody
1 = present but static
2..100 = the greater the amplitude/distance of the motion, the larger the body movement parameter
*/
bool MR24HPC1mmWave::getMotionAmplitude(uint* motionAmplitude)
{
	static const byte msg[] = { 0x53, 0x59, 0x80, 0x83, 0x00, 0x01, 0x0F, 0xBF, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), motionAmplitude);
}

/*
motionDirection :
0x00 = unknown
0x01 = moving closer / approaching
0x02 = moving away / receding
*/
bool MR24HPC1mmWave::getMotionDirection(uint* motionDirection)
{
	static const byte msg[] = { 0x53, 0x59, 0x80, 0x8B, 0x00, 0x01, 0x0F, 0xC7, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), motionDirection);
}


/////////////////////////////////////////////////////////////
// Underlying Open Function Mode [Advanced Mode]

// 0 = disable open function mode; 1 = enable open function mode (Advanced Mode)
bool MR24HPC1mmWave::setOpenFunctionMode(bool openFunctionMode)
{
	byte msg[] = { 0x53, 0x59, 0x08, 0x00, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), openFunctionMode);
}
bool MR24HPC1mmWave::getOpenFunctionMode(bool* openFunctionMode)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x80, 0x00, 0x01, 0x0F, 0x44, 0x54, 0x43 };
	uint b;
	bool ok = read1Byte(msg, sizeof(msg), &b);
	*openFunctionMode = b & 1;
	return ok;
}

// Get measured 'existence energy' or 'static noise of environment'
// 0..250
bool MR24HPC1mmWave::getStaticEnergy(uint* staticEnergy)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x81, 0x00, 0x01, 0x0F, 0x45, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), staticEnergy);
}

// Get measured 'motion energy' or 'motion noise of environment'
// 0..250
bool MR24HPC1mmWave::getMotionEnergy(uint* motionEnergy)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x82, 0x00, 0x01, 0x0F, 0x46, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), motionEnergy);
}

/* Get measured 'static distance' or 'detection range of presence'
staticDistance :
0x00 = no one
0x01 = 0.5m
0x02 = 1m
0x03 = 1.5m
0x04 = 2.0m
0x05 = 2.5m
0x06 = 3m
see convertDistanceToCm()
*/
bool MR24HPC1mmWave::getStaticDistance(uint* staticDistance)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x83, 0x00, 0x01, 0x0F, 0x47, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), staticDistance);
}

/* Get measured 'motion distance' or 'detection range of moving object'
0x00: no motion
0x01: 0.5m
0x02: 1m
0x03: 1.5m
0x04: 2.0m
0x05: 2.5m
0x06: 3m
0x07: 3.5m
0x08: 4m
see convertDistanceToCm()
*/
bool MR24HPC1mmWave::getMotionDistance(uint* motionDistance)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x84, 0x00, 0x01, 0x0F, 0x48, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), motionDistance);
}

/* Get measured 'motion speed' or 'detection speed of moving object'
motionSpeed :
0.5m/s per step
0x01..0x09 = approaching (+ve)
0x0A       = none
0x0B..0x14 = moving away (-ve)
see convertSpeedToCmPerSecond()
*/
bool MR24HPC1mmWave::getMotionSpeed(uint* motionSpeed)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x85, 0x00, 0x01, 0x0F, 0x49, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), motionSpeed);
}

/*TODO ???
08 86	get position event	0=none 1=forward 2=backward  ??? 
        1=object disordered motion or absence 2=object approach 3=object recede
08 87	get motion amplitude quantified data	0..100
*/


////////////////////////////////////////////////////////////////////////////////////////
// Custom Mode parameters

// Select 'custom mode'
// Up to four custom mode configurations can be saved
// customMode : 1..4 = select custom mode configuration 1..4
bool MR24HPC1mmWave::setCustomMode(uint customMode)
{
	MR24HPC1_ASSERT(customMode > 0 && customMode <= 4);
	byte msg[] = { 0x53, 0x59, 0x05, 0x09, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), customMode);
}
bool MR24HPC1mmWave::getCustomMode(uint* customMode)
{
	static const byte msg[] = { 0x53, 0x59, 0x05, 0x89, 0x00, 0x01, 0x0F, 0x4A, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), customMode);
}

// Saves changes to the current Custom Mode configuration
// contrary to the documentation, the current custom mode remains selected
bool MR24HPC1mmWave::saveCustomMode()
{
	static const byte msg[] = { 0x53, 0x59, 0x05, 0x0A, 0x00, 0x01, 0x0F, 0xCB, 0x54, 0x43 };
	return sendMessageAndGetResponse(msg, sizeof(msg));
}

/* Set Static Energy Threshold
'Set threshold of presence perception' or 'existence judgement threshold'.
Sets the static energy level to distinguish human presence from background
noise.

Tiny movements of the human body are used to detect non-moving (static)
people. There may be objects in the vicinity that make similar micro-
movements, such as fans or air conditioning units. This setting is used
to filter these out.

It should be selected to be just above the 'static background noise' when
the room is empty. Use the 'Static Energy' value to set the threshold,
see getStaticEnergy().

threshold : 0..250, default = 33
*/
bool MR24HPC1mmWave::setStaticEnergyThreshold(uint threshold)
{
	MR24HPC1_ASSERT(threshold <= 250);
	byte msg[] = { 0x53, 0x59, 0x08, 0x08, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), threshold);
}
bool MR24HPC1mmWave::getStaticEnergyThreshold(uint* threshold)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x88, 0x00, 0x01, 0x0F, 0x4C, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), threshold);
}

/* Set Static Boundary
The 'existence perception boundary' or 'detection range of presence'.

Sets the maximum detection range for static (slightly moving) objects to
reduce false alarms and minimize interference from outside the detection
range, through partitions or windows or from non-human sources.

0x00 = <not set>
0x01 = 0.5m
0x02 = 1m
0x03 = 1.5m
0x04 = 2.0m
0x05 = 2.5m
0x06 = 3m
0x07 = 3.5m
0x08 = 4m
0x09 = 4.5m
0x0A = 5m
see convertBoundaryToCm();
*/
bool MR24HPC1mmWave::setStaticBoundary(uint boundary)
{
	MR24HPC1_ASSERT(boundary <= 10);
	byte msg[] = { 0x53, 0x59, 0x08, 0x0A, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), boundary);
}
bool MR24HPC1mmWave::getStaticBoundary(uint* boundary)
{
	static const byte msg[] = {0x53, 0x59, 0x08, 0x8A, 0x00, 0x01, 0x0F, 0x4E, 0x54, 0x43};
	return read1Byte(msg, sizeof(msg), boundary);
}

/* Set Motion Energy Threshold
'Motion trigger threshold setting'
Sets the motion energy threshold level for motion detection. The motion
energy must be above this level to detect motion. Use this to adjust
motion sensitivity to small or large body movements, or to reduce false
alarms from outside the area.
This can be set by observing the motion energy for relevant situations,
see getMotionEnergy().

For example:
0..5	empty space
7..9	detects small body movements
15..20	detects small movements at a distance
60..80	detects only large movements at close range

The default is 4, but 10..14 may be optimal.

0..250, default = 4
*/
bool MR24HPC1mmWave::setMotionEnergyThreshold(uint threshold)
{
	MR24HPC1_ASSERT(threshold <= 250);
	byte msg[] = { 0x53, 0x59, 0x08, 0x09, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), threshold);
}
bool MR24HPC1mmWave::getMotionEnergyThreshold(uint* threshold)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x89, 0x00, 0x01, 0x0F, 0x4D, 0x54, 0x43 };
	return read1Byte(msg, sizeof(msg), threshold);
}

/* Set Motion Boundary
'Motion trigger boundary setting'
Sets the maximum distance at which motion will be detected. This limits
the detection range, or reduces interference from sources like continuously
rotating fans.

0x00 = <not set>
0x01 = 0.5m
0x02 = 1m
0x03 = 1.5m
0x04 = 2.0m
0x05 = 2.5m
0x06 = 3m
0x07 = 3.5m
0x08 = 4m
0x09 = 4.5m
0x0A = 5m
see convertRangeToCm();
*/
bool MR24HPC1mmWave::setMotionBoundary(uint boundary)
{
	MR24HPC1_ASSERT(boundary > 0 && boundary <= 10);
	byte msg[] = { 0x53, 0x59, 0x08, 0x0B, 0x00, 0x01, 0xff, 0xff, 0x54, 0x43 };
	return write1Byte(msg, sizeof(msg), boundary);
}
bool MR24HPC1mmWave::getMotionBoundary(uint* boundary)
{
	static const byte msg[] = {0x53, 0x59, 0x08, 0x8B, 0x00, 0x01, 0x0F, 0x4F, 0x54, 0x43};
	return read1Byte(msg, sizeof(msg), boundary);
}

/* Set Motion Duration
The 'motion trigger time' or 'motion valid time'.

This is the time period used to determine whether the state should be
changed from unoccupied to occupied. The motion energy threshold and
motion range conditions must remain active for this duration for the
state to be changed to occupied.

milliseconds = 1..1000ms, default 3000ms
*/
bool MR24HPC1mmWave::setMotionDuration(ulong milliseconds)
{
	byte msg[] = { 0x53, 0x59, 0x08, 0x0C, 0x00, 0x04, 0xff, 0xff, 0xff, 0xff, 0xff, 0x54, 0x43 };
	return write4Bytes(msg, sizeof(msg), milliseconds);
}
bool MR24HPC1mmWave::getMotionDuration(ulong* milliseconds)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x8C, 0x00, 0x01, 0x0F, 0x50, 0x54, 0x43 };
	return read4Bytes(msg, sizeof(msg), milliseconds);
}

/* Set Motion To Static Delay
'motion to still time setting' or 'set stationary valid time'

Sets time period used to determine whether the state should be changed from
moving to static (active to stationary).

1..60s (1000..60000ms) default 30000ms
*/
bool MR24HPC1mmWave::setMotionToStaticDelay(ulong milliseconds)
{
	byte msg[] = { 0x53, 0x59, 0x08, 0x0D, 0x00, 0x04, 0xff, 0xff, 0xff, 0xff, 0xff, 0x54, 0x43 };
	return write4Bytes(msg, sizeof(msg), milliseconds);
}
bool MR24HPC1mmWave::getMotionToStaticDelay(ulong* milliseconds)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x8D, 0x00, 0x01, 0x0F, 0x51, 0x54, 0x43 };
	return read4Bytes(msg, sizeof(msg), milliseconds);
}

/* Set Custom Unoccupied Delay
'Time for entering no person state setting' or 'set absence valid time'.

The occupied state is retained for this minimum period. This is useful
if the 'occupied' output is used to turn on lighting etc, so lights remain
on for a minimum duration.

'Custom' is use to differentiate it from the setUnoccupiedDelay() of the
standard mode - these are different settings.

0..3600s (0..3600000ms)
*/
bool MR24HPC1mmWave::setCustomUnoccupiedDelay(ulong milliseconds)
{
	byte msg[] = { 0x53, 0x59, 0x08, 0x0E, 0x00, 0x04, 0xff, 0xff, 0xff, 0xff, 0xff, 0x54, 0x43 };
	return write4Bytes(msg, sizeof(msg), milliseconds);
}
bool MR24HPC1mmWave::getCustomUnoccupiedDelay(ulong* milliseconds)
{
	static const byte msg[] = { 0x53, 0x59, 0x08, 0x8E, 0x00, 0x01, 0x0F, 0x52, 0x54, 0x43 };
	return read4Bytes(msg, sizeof(msg), milliseconds);
}

#endif	// #ifdef MR24HPC1_ENABLE_CONFIG
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// Parse a polled report message and update the associated public values, 
// see "Data from latest Report messages"
bool MR24HPC1mmWave::processReportMessage(bool* changed)
{
	*changed = false;
	
	uint controlWord = rxMsg[2];
	uint commandWord = rxMsg[3];

	// Timestamp of the last report message received
	lastReportTime = rxTimestamp;

	// Heartbeat packet, sent once a minute
	// this indicates that the module is still alive
	// (this module does not register human heartbeats!)
	if (controlWord == 0x01 && commandWord == 0x01) {
		MR24HPC1_LOG("<heartbeat>");
		return true;
	}

	// Initialization complete, sent after a resetModule()
	// this is normally awaited in resetModule(), so we do nothing
	if (controlWord == 0x05 && rxDataLen == 1) {
		MR24HPC1_LOG("<init complete>");
		return true;
	}

	// Standard Mode report
	if (controlWord == 0x80) {
		if (rxDataLen == 1) {
			MR24HPC1_LOG("<0x80 standard mode report>");

			uint data = rxMsg[6];
			switch (commandWord) {
			case 0x01:		// presence, reported on state change
				*changed = presence != data;
				presence = data;
				break;
			case 0x02:		// motion, reported on state change
				*changed = motion != data;
				motion = data;
				break;
			//TODO also called 'body movement parameter' 0..100, 0=nobody, 1=present but still, 2..100=activity 
			// 0 = unoccupied; 1 = static person; 2..100 motion amplitude
			case 0x03:		// motion amplitude, reported once a second
				//TODO set presence and motion from motionAmplitude?
				*changed = motionAmplitude != data;
				motionAmplitude = data;
				break;
			case 0x0B:		// motion direction, reported on state change
				*changed = motionDirection != data;
				motionDirection = data;
				break;
			default:
				MR24HPC1_LOGERROR("unknown 0x80 report data");
				return false;
			}
			return true;
		}
	}
	if (controlWord == 0x08) {
		if (rxDataLen == 1) {
			MR24HPC1_LOG("<0x08 1-byte report>");
			// https://files.seeedstudio.com/wiki/mmWave-radar/24GHz_mmWave_Sensor-Human_Static_Presence_Module_Lite_User_Manual.pdf#page=10
			uint data = rxMsg[6];
			switch (commandWord) {

			// motion direction, 0 = none; 1 = forward; 2 = back
			case 6:
				*changed = motionDirection != data;
				motionDirection = data;
				break;
			// motion amplitude
			case 7:
				*changed = motionAmplitude != data;
				motionAmplitude = data;
				// 0 = unoccupied/unknown; 1 = occupied
				presence = motionAmplitude == 0 ? 0u : 1u;
				// 0 = unoccupied/unknown; 1 = static person; 2 = moving person
				motion = motionAmplitude > 1 ? 2 : motionAmplitude;
				break;
			default:
				MR24HPC1_LOGERROR("unknown 0x08 report data");
				return false;
			}
			return true;
		}
		
		// Underlying open Function mode 
		// sent twice a second
		//TODO descriptions are different
		// https://files.seeedstudio.com/wiki/mmWave-radar/MR24HPC1_User_Manual-V1.5.pdf#page=17
		// https://files.seeedstudio.com/wiki/mmWave-radar/24GHz_mmWave_Sensor-Human_Static_Presence_Module_Lite_User_Manual.pdf#page=10
		/*
		byte 1 (0-250): statics noise of environment
		byte 2 (0x01-0x06): detection range of presence
		byte 3 (0-250): motion noise of environment
		byte 4 (0x01-0x08): detection range of object moving
		byte 5 (0x01-0x14): detection speed of object moving
		*/
		if (rxDataLen == 5 && commandWord == 0x01) {
			MR24HPC1_LOG("<0x08 advanced mode report>");

			uint data = rxMsg[6];
			if (staticEnergy != data)
				*changed = true;
			staticEnergy = data;

			data = rxMsg[7];
			if (staticDistance != data)
				*changed = true;
			staticDistance = data;

			data = rxMsg[8];
			if (motionEnergy != data)
				*changed = true;
			motionEnergy = data;

			data = rxMsg[9];
			if (motionDistance != data)
				*changed = true;
			motionDistance = data;

			data = rxMsg[10];
			if (motionSpeed != data)
				*changed = true;
			motionSpeed = data;

			return true;
		}
	}
	MR24HPC1_LOGERROR("unknown report messsage");
	return false;
}


// Sends a message and uses the state machine to receive the response
// the state machine skips any report messages that may have been sent
// The response is returned in 'rxMsg' and the response data length in 
// 'rxDataLen'
bool MR24HPC1mmWave::sendMessageAndGetResponse(const byte* msg, uint msglen)
{
	MR24HPC1_LOGMSG(true, msg, msglen);

	// clear receive buffer
	while (stream->available())
		stream->read();

	// send message
	if (stream->write(msg, msglen) != msglen) {
		MR24HPC1_LOGERROR("write failed");
		return false;
	}

	// get response
	ulong tstart = millis();
	while (!receiveNextMessage(msg)) {
		if ((millis() - tstart) > 1000) {
			MR24HPC1_LOGERROR("response timeout");
			return false;
		}
	}
	return true;
}

/*
Receive next message into rxMsg[]
Use this for both report messages and command responses.
cmd = Command message if expecting a command response,
	or null for a report message poll
	The control and command bytes are used check to ensure this
	is the response to the command that was sent.
For command responses, the timeout must be implemented by the caller.
On return:
	rxMsgLen  = full message length
	rxDataLen = number of data bytes
Returns true if a message has been received, false if not.
It may need to be called more than once to receive a full message.
*/
bool MR24HPC1mmWave::receiveNextMessage(const byte* cmd)
{
	while (stream->available()) {
		byte b = stream->read();

		// buffer all characters of message
		if (rxState != 0) {
			if (rxIndex == sizeof(rxMsg)) {
				MR24HPC1_LOGERROR("rxMsg overflow");
				rxState = 0;
				return false;
			}
			rxMsg[rxIndex++] = b;
		}

		switch (rxState) {
		case 0:
			// start frame 1st byte
			if (b == 0x53) {
				rxTimestamp = millis();
				rxIndex = 0;
				rxIndex = 0;
				rxMsgLen = 0;
				rxDataLen = 0;
				rxLen = 0;
				rxGotMsg = false;

				rxMsg[rxIndex++] = b;
				rxState = 1;
			}
			// character not part of message, ignore it
			//else
			//	;
			break;

		case 1:
			// start frame 2nd byte
			rxState = b == 0x59 ? 2 : 0;
			break;

		case 2:
			// control byte | command byte | length word
			if (cmd) {
				// if receiving a command response, 
				// validate the control and command bytes
				// restart receive if they don't match
				if (rxIndex == 3) {
					// control byte
					if (b != cmd[2])
						rxState = 0;
					break;
				}
				if (rxIndex == 4) {
					// command byte
					if (b != cmd[3])
						rxState = 0;
					break;
				}
			}
			if (rxIndex == 6) {
				// 2nd byte of length word
				rxLen = (rxMsg[4] << 8) + rxMsg[5];
				if (rxLen == 0 || rxLen > 100) {
					MR24HPC1_LOGERROR("invalid rxLen");
					rxState = 0;
					break;
				}
				rxDataLen = rxLen;
				++rxLen;		// +1 byte for the checksum
				rxState = 3;
			}
			break;

		case 3:
			// receiving data, 'len' bytes
			// note that the data may contain end-of-frame bytes
			// that's why you need a state machine
			if (--rxLen == 0)
				rxState = 4;
			break;

		case 4:
			// end frame 1st byte
			rxState = b == 0x54 ? 5 : 0;
			break;

		case 5:
			// end frame 2nd byte
			rxGotMsg = b == 0x43;	// message received
			rxState = 0;
			break;

		default:
			MR24HPC1_LOGERROR("rx bug");
			rxState = 0;
			return false;
		}

		// full message received, verify the checksum
		if (rxGotMsg) {
			rxMsgLen = rxIndex;

			MR24HPC1_LOGMSG(false, rxMsg, rxMsgLen);

			uint ixcsum = rxIndex - 3;
			uint csum = 0;
			for (int i = 0; i < ixcsum; ++i)
				csum += rxMsg[i];
			if ((byte)csum != rxMsg[ixcsum]) {
				MR24HPC1_LOGERROR("bad csum");
				return false;
			}
			return true;
		}
	}

	// full message not yet received
	return false;
}

// Returns the checksum of a message
// the LS byte of the sum of all bytes before the checksum byte
byte MR24HPC1mmWave::checksumMsg(const byte* msg, uint msglen)
{
	uint cs = 0;
	for (int i = 0; i < msglen - 3; ++i)
		cs += msg[i];
	return (byte)cs;
}

// Send a message, get the response and extract a single 1-byte value
bool MR24HPC1mmWave::read1Byte(const byte* msg, uint msglen, uint* value)
{
	MR24HPC1_ASSERT(msglen == 10);
	if (!sendMessageAndGetResponse(msg, msglen))
		return false;
	if (rxDataLen != 1) {
		MR24HPC1_LOGERROR("response not 1 byte");
		return false;
	}
	*value = rxMsg[6];
	return true;
}


#ifdef MR24HPC1_ENABLE_CONFIG

// Put a single 1, 2, or 4-byte value into a message, update the checksum, 
// send the message and get the response
bool MR24HPC1mmWave::write1Byte(byte* msg, uint msglen, uint value)
{
	MR24HPC1_ASSERT(msglen == 10);
	msg[6] = (byte)value;
	msg[7] = checksumMsg(msg, msglen);
	return sendMessageAndGetResponse(msg, msglen);
}
bool MR24HPC1mmWave::write2Bytes(byte* msg, uint msglen, uint value)
{
	MR24HPC1_ASSERT(msglen == 11);

	// MS byte first, LS byte last
	// little-endian, or is it big-startian?
	msg[7] = (byte)value;
	value >>= 8;
	msg[6] = (byte)value;
	msg[8] = checksumMsg(msg, msglen);
	return sendMessageAndGetResponse(msg, msglen);
}
bool MR24HPC1mmWave::write4Bytes(byte* msg, uint msglen, ulong value)
{
	MR24HPC1_ASSERT(msglen == 13);

	// MS byte first, LS byte last
	// little-endian, or is it big-startian?
	msg[9] = (byte)value;
	value >>= 8;
	msg[8] = (byte)value;
	value >>= 8;
	msg[7] = (byte)value;
	value >>= 8;
	msg[6] = (byte)value;
	msg[10] = checksumMsg(msg, msglen);
	return sendMessageAndGetResponse(msg, msglen);
}

// Send a message, get the response and extract a single 2 or 4-byte value
bool MR24HPC1mmWave::read2Bytes(const byte* msg, uint msglen, uint* value)
{
	if (!sendMessageAndGetResponse(msg, msglen))
		return false;
	if (rxDataLen != 2) {
		MR24HPC1_LOGERROR("response not 2 bytes");
		return false;
	}
	// MS byte first
	*value = (uint)(rxMsg[6] << 8) + rxMsg[7];
	return true;
}
bool MR24HPC1mmWave::read4Bytes(const byte* msg, uint msglen, ulong* value)
{
	if (!sendMessageAndGetResponse(msg, msglen))
		return false;
	if (rxDataLen != 4) {
		MR24HPC1_LOGERROR("response not 4 bytes");
		return false;
	}
	// MS byte first
	*value = (rxMsg[6] << 24) + (rxMsg[7] << 16) + (rxMsg[8] << 8) + rxMsg[9];
	return true;
}

// Conversion methods

uint MR24HPC1mmWave::convertUnoccupiedDelayToSeconds(uint unoccupiedDelay)
{
	static const uint secs[9] =	{ 0, 10, 30, 60, 120, 300, 600, 1800, 3600 };
	if (unoccupiedDelay > 8) {
		// if invalid, return the largest uint
		// this works for 16 and 32 bit machines, -1 = 0xFFFF or 0xFFFFFFFF
		return -1;
	}
	return secs[unoccupiedDelay];
}

// 1 = 2.5m, 2 = 3m, 3 = 4m
uint MR24HPC1mmWave::convertSensitivityToCm(uint sensitivity)
{
	static const uint cm[4] = { 0, 250, 300, 400 };
	if (sensitivity > 3)
		return -1;
	return cm[sensitivity];
}

/*
0  = No one moving
1  = 0.5m (50cm)
...
10 = 5m   (500cm);
*/
inline uint MR24HPC1mmWave::convertDistanceToCm(uint distance)
{
	return distance * 50;
}

/* +500 .. -500 cm/s (+-5m per second, +-18km/hr)
motion : 0.5m/s per step
0x01..0x09 = approaching (+ve)
0x0A       = none
0x0B..0x14 = moving away (-ve)
//TODO 0 should be 500cm/s, but it starts at 0 instead of 0x0A
*/
int MR24HPC1mmWave::convertMotionToCmPerSecond(uint motion)
{
	if (motion > 0x14) {
		// if invalid, return the largest positive integer
		// this works for 16 and 32 bit machines, 0x7FFF or 0x7FFFFFFF
		return ((uint)-1) >> 1;
	}
	if (motion == 0x0A)
		return 0;
	if (motion < 0x0A)
		return (int)((0x0A - motion) * 50);
	return -((int)(motion - 0x0A) * 50);
}


// This method is only available in DEBUG mode
#ifdef DEBUG

// Print the entire configuration
// parameters for Standard Mode, Open Function Mode and Custom Mode are printed
void MR24HPC1mmWave::printParameters()
{
	MR24HPC1_SERIAL.println();
	MR24HPC1_SERIAL.println("MR24HPC1 PARAMETERS");

	bool ofm;
	if (!getOpenFunctionMode(&ofm)) {
		MR24HPC1_SERIAL.println("getOpenFunctionMode() failed");
		MR24HPC1_SERIAL.flush();
		return;
	}

	uint failed = 0;
	char txt[100];

	char productModel[16], productId[16], hardwareModel[16], firmwareVersion[16];
	uint protocolVersion;
	if (getProductInformation(productModel, productId, hardwareModel, firmwareVersion,
		&protocolVersion)) {
		sprintf(txt, "ProductModel = %s", productModel);
		MR24HPC1_SERIAL.println(txt);
		sprintf(txt, "ProductId = %s", productId);
		MR24HPC1_SERIAL.println(txt);
		sprintf(txt, "HardwareModel = %s", hardwareModel);
		MR24HPC1_SERIAL.println(txt);
		sprintf(txt, "FirmwareVersion = %s", firmwareVersion);
		MR24HPC1_SERIAL.println(txt);
		sprintf(txt, "ProtocolVersion = %u", protocolVersion);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getProductInformation failed");
		++failed;
	}

	SCENE scene;
	if (getScene(&scene)) {
		char* s;
		switch (scene) {
		case 0:
			s = "None";
			break;
		case SCENE_LIVING_ROOM:
			s = "Living Room, range 4..4.5m";
			break;
		case SCENE_BEDROOM:
			s = "Bedroom, range 3.5..4m";
			break;
		case SCENE_BATHROOM:
			s = "Bathroom, range 2.5..3m";
			break;
		case SCENE_AREA:
			s = "Area, range 3..3.5m";
			break;
		default:
			s = "<unknown>";
			break;
		}
		sprintf(txt, "Scene = %u : %s", (uint)scene, s);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getScene failed");
		++failed;
	}

	uint sensitivity;
	if (getSensitivity(&sensitivity)) {
		if (sensitivity == 0)
			strcpy(txt, "Sensitivity = 0 : <not set>");
		else {
			sprintf(txt, "Sensitivity = %u : %ucm",
				sensitivity,
				convertSensitivityToCm(sensitivity));
		}
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getSensitivity failed");
		++failed;
	}

	uint unoccupiedDelay;
	if (getUnoccupiedDelay(&unoccupiedDelay)) {
		uint secs = convertUnoccupiedDelayToSeconds(unoccupiedDelay);
		sprintf(txt, "UnoccupiedDelay = %u : %u seconds",
			unoccupiedDelay, secs);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getUnoccupiedDelay failed");
		++failed;
	}

	bool openFunctionMode;
	if (getOpenFunctionMode(&openFunctionMode)) {
		// show original mode!
		openFunctionMode = ofm;

		sprintf(txt, "OpenFunctionMode = %u : %s", 
			openFunctionMode, 
			openFunctionMode ? "On" : "Off");
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getOpenFunctionMode failed");
		++failed;
	}

	uint customMode;
	if (getCustomMode(&customMode)) {
		if (customMode == 0)
			MR24HPC1_SERIAL.println("CustomMode = 0 : <not set>");
		else {
			sprintf(txt, "CustomMode = %u", customMode);
			MR24HPC1_SERIAL.println(txt);
		}
	}
	else {
		MR24HPC1_SERIAL.println("getCustomMode failed");
		++failed;
	}

	uint set;
	if (getStaticEnergyThreshold(&set)) {
		sprintf(txt, "StaticEnergyThreshold = %u", set);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getStaticEnergyThreshold failed");
		++failed;
	}

	uint sb;
	if (getStaticBoundary(&sb)) {
		sprintf(txt, "StaticBoundary = %u : %ucm", sb, convertDistanceToCm(sb));
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getStaticBoundary failed");
		++failed;
	}

	uint met;
	if (getMotionEnergyThreshold(&met)) {
		sprintf(txt, "MotionEnergyThreshold = %u", met);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getMotionEnergy failed");
		++failed;
	}

	uint mb;
	if (getMotionBoundary(&mb)) {
		sprintf(txt, "MotionBoundary = %u : %ucm", mb, convertDistanceToCm(mb));
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getMotionBoundary failed");
		++failed;
	}
	
	ulong md;
	if (getMotionDuration(&md)) {
		sprintf(txt, "MotionDuration = %lums", md);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getMotionDuration failed");
		++failed;
	}
	
	ulong msd;
	if (getMotionToStaticDelay(&msd)) {
		sprintf(txt, "MotionToStaticDelay = %lums", msd);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getMotionToStaticDelay failed");
		++failed;
	}

	ulong cud;
	if (getCustomUnoccupiedDelay(&cud)) {
		sprintf(txt, "CustomUnoccupiedDelay = %lums", cud);
		MR24HPC1_SERIAL.println(txt);
	}
	else {
		MR24HPC1_SERIAL.println("getCustomUnoccupiedDelay failed");
		++failed;
	}

	// restore open function mode
	if (ofm != 0)
		setOpenFunctionMode(ofm);

	if (failed) {
		sprintf(txt, "Error: %u messages failed", failed);
		MR24HPC1_SERIAL.println(txt);
	}
	MR24HPC1_SERIAL.println();
	MR24HPC1_SERIAL.flush();
}

#endif		// #ifdef DEBUG

#endif		// #ifdef MR24HPC1_ENABLE_CONFIG
