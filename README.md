# Arduino Library for Seeed Studio MR24HPC1 25GHz mmWave Sensor

The "MR24HPC1 24GHz mmWave Sensor Human Static Presence Module Lite" from Seeed Studio has a detection range of up to 5m. 

It runs on 5V, 3.3V cannot be used. Communications uses a proprietary binary serial protocol at a fixed rate of 115200 baud. 

<img height="300" alt="image" src="images/mr24hpc1-boards.jpg" />

The board seems to have two mmWave sensor chips. The chip connected to the gold pads is an ICLEGEND S3KM111L mmWave radar chip. Below that, there's a larger MicRadar T15BT chip, which is also an mmWave radar sensor module...

"_The MicRadar T15BT is an integrated 24GHz mmWave radar sensor and Bluetooth/Wi-Fi combo module that uses the ICLEGEND Micro S3KM111L radar System-on-Chip (SoC)._"

I guess the T15BT (BT=Bluetooth) is just being used to manage the serial protocol. It's not wired for Bluetooth, there's no antenna. (Seeed Studio does have a "Sensor Kit" that uses Bluetooth and WiFi, but this has a XIAO microcontroller to manage the Bluetooth and WiFi communications.)

There is a very similar mmWave board from MicRadar, the R24DVD1, but this has a different communications protocol.


<!-- ========================================================================================== -->

## Contents

- [Summary](#summary)
- [Detecting the Occupied/Unoccupied State](#occupied-state)
- [Connecting the Module](#connecting-the-module)
- [Windows Application](#windows-application)
- [Class Reference](#class-reference)
- [Example Sketch](#example-sketch)
- [Updating the Firmware](#firmware-update)
- [Why Use This Library?](#advantages)
- [References](#references)
- [Revision History](#revision-history)


<!-- ========================================================================================== -->

<a name="summary"></a>
## Summary from the User Manual

The Seeed Studio documentation is not easy to understand, so here is a "brief" summary...

The sensor transmits a complex 24GHz band millimetre-wave radar signal and receives the reflections. A powerful on-board microcontroller processes the reflected signal and determines human presence, distance and motion according to certain preset or configured parameters. It detects large movements (motion) and also the very small movements (static) made by a person standing, sitting or lying down. It detects the micro-motion of breathing and other tiny movements. More expensive modules can even detect beating hearts.

Because it's radar, it can "see through" some objects, clothes, thin walls and windows. For this reason it can be configured to reject signals below a certain intensity (energy level) or a certain distance (boundary). The module can be mounted inside a thin plastic enclosure, such as a surveillance camera enclosure, which is transparent to the radar signals.

Metal objects are not transparent to the radar signals and are highly reflective. Vibrating metal objects such as fans and air conditioning units can cause the reflected signal to be misinterpreted. There are several configuration parameters which can be used to reduce the effects of this noise. People wearing clothes are less reflective than solid static objects, which is why motion is used to detect human presence. (Presumably you should use the "Bedroom" and "Bathroom" presets if the targets are not wearing clothes :-)

The radar signal is a cone, which means that it behaves best when mounted on the ceiling, with sensitivity varying from the edge to the centre of the cone. This reflected signal is a "radius" rather than a straight-line distance, and I suspect all the distances are actually radiuses too (or 'radii'). It can be mounted under a chair or a bed because the radar signal passes through the cushions or mattress. Mounting on a wall is possible too, where it will be able to detect humans moving about on the ceiling, but if you don't need that feature then half of the radar's field-of-vision will not be usable.

A good quality 5V power supply is needed, with very low ripple (< 100mV p-p) and a filter for high frequency noise. The manual suggests circuits for the power supply. For best accuracy, maybe a 5V power supply separate from the USB and microcontroller power would be a good idea. A good ground circuit is very important.

The module has two digital (5V/0V on/off) outputs. Output S1 is high if "Occupied" when a person or persons are detected. Output S2 is high when motion is detected. There are also 4 general-purpose outputs GP1..GP4, but these are undocumented - it would have been nice if these were configurable.

The board has two configuration modes. _Standard Function Mode_ uses a _Scene Preset_ to select ranges and energy levels for four standard environments (rooms), along with a _Sensitivity_ distance and an _Unoccupied Delay_ (10s..60mins). _Advanced Mode_, confusingly called _Underlying Open Function Mode_ in the manual, allows a much more detailed configuration, with programmable energy thresholds, distance ranges and timing. The configuration parameters are described below.

Once configured, the module can be used stand-alone if the S1 (Occupied/Unoccupied) and S2 (Motion/No Motion) outputs are sufficient. Alternatively, the serial connection or USB can be used to supply real-time data, or it can be connected to a microcontroller to provide Bluetooth or WiFi communications with a server.

During normal operation, the sensor sends unsolicited _Report_ messages on its TX line. In Standard Mode these are sent only when a state changes. In Advanced Mode, an additional status message is sent twice-a-second, with extra data for logging or graphical traces.


<!-- ========================================================================================== -->

<a name="occupied-state"></a>
## Detecting the Occupied/Unoccupied State

The microcontroller does many mysterious things, which boils down to the simple 'Occupied' and 'Unoccupied' states. This is how the documentation describes it.

The state is 'Occupied' when _motion_ (a moving person) or _static presence_ (a stationary person) is detected. Could 'person' be an animal or any moving object? This is not clear.

1) To detect motion, the following conditions must be satisfied:

- The 'Motion Energy' reading must be greater than the 'Motion Energy Threshold' setting.
- The 'Motion Distance' reading must be within the 'Motion Boundary' setting.
- If these two conditions are not met, the 'moving' state will lapse after the 'Motion To Static Delay' period and it will switch to 'static presence' detection.

2) To detect static presence, these conditions must be satisfied:

- The 'Static Energy' reading must be greater than the 'Static Energy Threshold' value.
- The 'Static Distance' must be within the 'Static Boundary' setting.
- If these conditions are not met, the 'Occupied' state will revert to 'Unoccupied' after the 'Unoccupied Delay' period.

When 'Unoccupied', if motion or static presence is again detected, it goes to step 1 or 2.

_But I must say that it does not behave in exactly this way when I observe the states in the MR24HPC1.exe App's graphical view..._


<!-- ========================================================================================== -->

<a name="connecting-the-module"></a>
## Connecting the Module

> [!WARNING]
> This module runs at 5V! You must NOT connect it directly to 3.3V a microcontroller unless it has 5V-tolerant GPIOs! Also note that NOT ALL GPIOs are 5V-tolerant, so check the data sheet very carefully. \
> On a 3.3V MCU, you could put a 10K ohm current-limiting resistor between the sensor's TX output and the MCU's RX input, and also for the S1 and S2 signals if you use them. (See Disclaimer.)

The library needs microcontroller serial port which can run at 115200 baud. The example sketch uses `Serial1` for the sensor, with `Serial` being used for the debug output to the Serial Monitor.

Because the library uses a `Stream` pointer, it can use either `HardwareSerial` or `SoftwareSerial`. 

A `HardwareSerial` port has fixed pins, which will probably be different for each microcontroller board. The standard pins are RX=Pin 0 and TX=Pin 1. Unfortunately this is usually the default `Serial` port which is used for the Serial Monitor, so these pins cannot be used for the mmWave sensor unless you use a different port for the Serial Monitor. The example sketch shows how to select the sensor's serial port for different MCUs.

With `SoftwareSerial`, you can use almost any two GPIO pins for RX/TX. But SoftwareSerial _will not run_ at the required 115200 baud on many controllers. To use SoftwareSerial reliably at 115200 baud, and have time to do other things, you will need a _very fast_ 32-bit controller clocked at over 120MHz. On an STM32F103RB (72MHz) and it did not work, the max speed was 86400. On an STM32F407ZGT6 (168MHz), SoftwareSerial ran perfectly at 115200 baud.

TODO test SoftwareSerial at 120MHz

### 2mm Pin Spacing :-(

The board's male 2 x 8 pin header for the UART connection has 2mm spacing. This is VERY annoying because the standard spacing is 2.54mm. Not many people have 2mm female connectors, so you'll probably have to buy some. It's not a good idea to force 2.54mm Dupont jumpers onto these smaller pins because they will be bent, and the connections are not reliable due to the smaller pin width (as I found out).


TODO diagram and photo




<!-- ========================================================================================== -->

<a name="windows-application"></a>
## Windows Application, MR24HPC1.exe

A Windows application called `MR24HPC1.exe` has been developed to help with configuration, testing and visualisation. This application will help you understand the configuration parameters and sensor readings.

<img height="300" alt="image" src="images/mr24hpc1-app.jpg" />


The full User Manual for the `MR24HPC1.exe` application is here, \
https://muman.ch/muman/index.htm?muman-mr24hpc1-mmwave-sensor.htm

The source for this application (Visual Studio 2026, C#) will soon be on github, but until then you can download the executable from here, \
https://muman.ch/muman/MR24HPC1.exe \
(Windows will have a 'Windows Protected Your PC' fit if you try to download and run it. It's very safe though, it does not use any internet connections or even write to the hard drive. So just press 'Run Anyway'.)

**TODO check this**  Once configured by the application, you do not need to configure with code in `setup()`.

An Arduino or other controller is not needed to use the Windows application. The PC connects directly to the mmWave sensor using a _USB-to-5V-Serial Convertor_.

???
To use the _mumanchu_ MR24HPC1.exe or Seeed Studio configurators, you will need to convert the PC's USB communications to 5V TTL Serial.

The best converter to use is an FTDI TTL-232R-5V, but there are many suitable converters available. Make sure it's the 5V version, because a 3.3V version will not work and may be damaged.

**Using an Arduino Uno as a USB-to-serial converter**

The FTDI converter is quite expensive. But and old Arduino Uno can be used instead. These contain the same FTDI chip, the FT232, so they do the same thing. To disable the Arduino microcontroller, connect the RESET pin to GND. This prevents the microcontroller from starting up, allowing the FT232 chip to be used for USB-to-TTL RX/TX communications.

Connect the Arduino's 5V and GND to power the sensor, the RX (pin 0) to the module's TX, and TX (pin 1) to the module's RX.

Now run the _mumanchu_ MR24HPC1.exe or Seeed Studio configurator, select the COM port and press the "Connect" button.



TODO



<!-- ========================================================================================== -->

<a name="class-reference"></a>
## Class Reference

Descriptions of each method can be read from the associated comments in the source code file [MR24HPC1mmWave.h](src/MR24HPC1mmWave.h). The descriptions are not duplicated here.

If you need to access an instance of this class from more than one .cpp file, split `MR24HPC1mmWave.h` into two files, one with the class definition (.h) and one with the code (.cpp).


```cpp
class MR24HPC1mmWave
{
public:
    // Set in begin(), normally 1
    uint protocolVersion;   // can be used if there are differences in the protocol

    // Data from Standard Mode reports
    uint presence;          // 0 = unoccupied/unknown; 1 = occupied
    uint motion;            // 0 = unoccupied/unknown; 1 = static person; 2 = moving person
    uint motionAmplitude;   // 0 = unoccupied; 1 = static person; 2..100 motion amplitude
    uint motionDirection;   // 0 = static; 1 = moving closer; 2 = moving away

    // Data from Underlying Open Function Mode mode reports
    uint staticEnergy;      // 0..250
    uint staticDistance;    // 0..8 in 0.5m steps (0..4m)
    uint motionEnergy;      // 0..250
    uint motionDistance;    // 0..6 in 0.5m steps (0..3m)
    uint motionSpeed;       // 0x00 = none, 0x01..0x09 = approaching, 0x0A = static, 
                            // 0x0B..0x14 = moving away, 0.5m/s steps
    uint lastReportTime;    // time of last report message, including 'heartbeat'

    bool begin(Stream* stream);

    bool getProductInformation(char* productModel, char* productId, 
        char* hardwareModel, char* firmwareVersion);
    bool resetModule();
    bool getInitStatus(uint* initComplete);

    // Standard Mode, also known as Scene Mode

	// Presets
    // note: new document has AREA and BEDROOM swapped AREA = 1, BEDROOM = 4
    // which version is correct?
    // sorted by 4/3.5/3/2.5? swap BATHROON & AREA, not BEDROOM & AREA
    // sets the 'detection radius'
    enum SCENE : uint
    {
        SCENE_NONE = 0,         // scene not set
        SCENE_LIVING_ROOM = 1,  // 4 .. 4.5m
        SCENE_BEDROOM = 2,      // 3.5 .. 4mm
        SCENE_BATHROOM = 3,     // 2.5 .. 3m
        SCENE_AREA = 4          // 3 .. 3.5m
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
    bool setOpenFunctionMode(uint openFunctionMode);
    bool getOpenFunctionMode(uint* openFunctionMode);
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

    // Messages
    bool processReportMessage(bool* changed);
    bool receiveNextMessage(const byte* cmd = NULL);

    #ifdef DEBUG
    void printParameters();
    #endif
};
```


<!-- ========================================================================================== -->

<a name="example-sketch"></a>
## Example Sketch

[MR24HPC1mmWave-Example.ino](examples/MR24HPC1mmWave-Example/MR24HPC1mmWave-Example.ino)



<!-- ========================================================================================== -->

<a name="firmware-update"></a>
## Updating the Firmware

TODO can a cheap ST-LINK device be used?



The Seeed Studio Wiki contains a detailed section on updating the firmware via the SWD/J-TAG pins, but this requires the very expensive "J-Link Programmer" to do the job, which can only be afforded by professionals. However, I managed to upgrade the firmware using the far cheaper "J-Link EDU Mini" version with the free "SEGGER J-Flash Lite" application, see the link in the References section below. At CHF60.-, the J-Link EDU Mini version is about one-tenth the cost of the professional version.

First I had to download the "Pack_Segger_AT32F4xx_v1.3.3.zip" file and run "Segger_AT32F4xx_AddOn.exe" to add the AT32F4xx devices to the J-Link database for J-Flash to use. Device 'AT32F403ARGT7' must be selected as the target.

It's also possible to update the firmware via the serial port using their own "PackageMake" application (download "PackageMake-v1.1.1.zip"), but that only works for later versions of the firmware. I have not tried this.

I must say that I'm always very dubious about running unsigned Chinese applications on my computer. Why don't they 'Sign' their software? Are they unable to get a Software Signing Certificate?

The firmware version I am using is now G24VD1SYV001006, from file "Jlink_MR24HPC1-20230302.bin" which was downloaded from the Seeed Studio wiki page (link in References section). They have not released a new version since March 2023, so they have probably stopped further development of this product.


<img height="300" alt="image" src="images/j-flash-lite.png" />


<!-- ========================================================================================== -->

<a name="advantages"></a>
## Why Use This Library?

There are only a couple of other minimalist MR24HPC1 libraries out there, but this library has several advantages...

- It contains methods to configure _all_ the sensor's parameters. (But if you don't use them, the compiler will optimize them out.)

- Optionally displays all messages sent and received, in hex. This is very useful for testing and understanding the binary protocol.

- Has debug code to dispaly the entire current configuration, see `printParameters()`.

- Because the library uses a `Stream` pointer, it can use either `HardwareSerial` (`Uart`) or `SoftwareSerial`. With SoftwareSerial you can use almost any two GPIO pins to connect to the sensor, but you need a very fast microcontroller (>120MHz).

- There is a [Windows Application](#windows-application) called MR24HPC1.exe to help with the configuration, testing and visualisation.

- The library's source code contains useful comments describing what each method does, and why.

- It's been tested on multiple microcontrollers, STM32, SAMD, AVR, ESP32 and ESP8266.

- The library recommended by Seeed Studio contains `Serial.print()` statements everywhere, which cannot be disabled. This makes the library unusable in a release product.

- This library uses integers, not floats, so the code is smaller and faster.


<!-- ========================================================================================== -->

<a name="references"></a>
## References

**Seeed Studio MR24HPC1 Wiki** \
https://wiki.seeedstudio.com/Radar_MR24HPC1/

**MR24HPC1 User Manual V1.5 (no date)** \
https://files.seeedstudio.com/wiki/mmWave-radar/MR24HPC1_User_Manual-V1.5.pdf

**Slightly Different User Manual (2022)** \
https://files.seeedstudio.com/wiki/mmWave-radar/24GHz_mmWave_Sensor-Human_Static_Presence_Module_Lite_User_Manual.pdf

**MR24HPC1 Data Sheet (2023)** \
https://files.seeedstudio.com/wiki/mmWave-radar/24GHz_mmWave_Sensor-Human_Static_Presence_Module_Lite_Datasheet.pdf

**IcLegend S3KM111L Brochure** \
https://edworks.co.kr/wp-content/uploads/2023/10/S3KM111L_Brochure.pdf

**Seeed Studio Recommended Library** \
Linked from their wiki page (but not written by Seeed Studio?) \
https://github.com/limengdu/Seeed_24GHz_Human_Static_Presence_Module_Lite

**Visual Micro ** \
For developing Arduino apps using Microsoft Visual Studio 2026 (free Community Edition) \
https://www.visualmicro.com/


<!-- ========================================================================================== -->

<a name="revision-history"></a>
## Revision History

| Date       | Version  | Notes        |
|:---------- |:---------| :----------- |
| 2026.11.11 | 0.0.0    | Preliminary  |


<br/>

## Joke of the Week 

Did you sleep well? \
_I don't know, I was asleep._

© matt 2026
