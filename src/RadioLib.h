#if !defined(_RADIOLIB_H)
#define _RADIOLIB_H

/*!
  \mainpage RadioLib Documentation

  Universal wireless communication library for embedded devices.

  \par Currently Supported Wireless Modules and Protocols
  - CC1101 FSK module
  - LLCC68 LoRa/FSK module
  - LR11x0 LoRa/FSK/LR-FHSS module
  - nRF24 FSK module
  - RF69 FSK module
  - RFM2x FSK module
  - Si443x FSK module
  - SX126x LoRa/FSK module
  - SX127x LoRa/FSK module
  - SX128x LoRa/GFSK/BLE/FLRC module
  - SX1231 FSK module
  - PhysicalLayer protocols
    - RTTY (RTTYClient)
    - Morse Code (MorseClient)
    - AX.25 (AX25Client)
    - SSTV (SSTVClient)
    - Hellschreiber (HellClient)
    - 4-FSK (FSK4Client)
    - APRS (APRSClient)
    - POCSAG (PagerClient)
    - LoRaWAN (LoRaWANNode)

  \par Quick Links
  Documentation for most common methods can be found in its reference page (see the list above).\n
  Some methods (mainly configuration) are also overridden in derived classes, such as SX1272, SX1278, RFM96 etc. for SX127x.\n
  \ref status_codes have their own page.\n
  Some modules implement methods of one or more compatibility layers, loosely based on the ISO/OSI model:
  - PhysicalLayer - FSK and LoRa radio modules

  \see https://github.com/jgromes/RadioLib
  \see https://jgromes.github.io/RadioLib/coverage/src/index.html

  \copyright  Copyright (c) 2019 Jan Gromes
*/

// Minimal RadioLib umbrella header trimmed for Raspberry example
#include "TypeDef.h"
#include "Module.h"
#include "Hal.h"

// Only keep Raspberry Pi HAL
#include "hal/RPi/PiHal.h"

// Keep only SX126x module (SX1262) used by the example
#include "modules/SX126x/SX1262.h"

// Protocols required by the example
#include "protocols/PhysicalLayer/PhysicalLayer.h"
#include "protocols/LoRaWAN/LoRaWAN.h"

// Utilities required by LoRaWAN/SX126x
#include "utils/CRC.h"
#include "utils/Cryptography.h"

#endif
