# LoWAPP Hardware Implementation

This folder contains an implementation of the LoWAPP protocol aimed at running on a STM32L151CC MCU from STMicroelectronics, connected to a Semtech SX1272 LoRa radio chip via SPI.

## Structure of the project

This project is based on the LoRaWan stack implementation of Semtech, available at https://github.com/Lora-net/LoRaMac-node. Semtech added an abstraction layer between its application and the STM32 HAL (Hardware Abstraction Layer) from STMicroelectronics. We are using the same abstraction layer for this project, as well as the same code structure. Communication with the SX1272 chip is done using the sx1272 source files also from Semtech.
```
.
`-- src
    |-- boards
    |-- lowapp
    |   |-- lowapp_core
    |   |-- lowapp_shared_res
    |   |-- lowapp_sys
    |   `-- lowapp_utils
    |-- mac
    |-- radio
    |-- sensors_supply
    `-- system
```

### Source folder

The `src/` folder contains the source code of the project. It contains the LoWAPP core, as well as the implementation of the system level functions required for the core to run.

#### LoWAPP Core

The core of the protocol is stored next to this simulation, in the root directory of the repository (`lowapp/`). It is present in the project as linked resource in `src/lowapp/`.

#### LoWAPP Hardware System Implementation

The `src/lowapp/lowapp_sys/` folder contains a middle layer used to interface the LoWAPP core with the Semtech's abstraction layer:
* EEPROM: Read and write configuration values to persistant memory in EEPROM
* Radio: Layer between the sx1272 files from Semtech and the core
* Timer: Gives the LoWAPP core the ability to use 3 timers using Semtech code
* UART: Process incoming and outgoing communication through UART to and from the LoWAPP core.

The other subfolders of `src/` contain code directly from the Semtech repository, with a few changes done to some files:
* Changes in the UART files to enable sending and receiving string with end of line characters
* Unused code commented out to enable build

#### Example application

An example application where a GPS module is connected to the MCU through I2C is available in `hardware/example-application`. It uses a Xadow GPS v2 from Seeed Studio (Seeed Studio wiki page : [http://wiki.seeedstudio.com/wiki/Xadow_GPS_v2](http://wiki.seeedstudio.com/wiki/Xadow_GPS_v2)).

This example application extracts the GPS location from the module every 30 seconds and send it as broadcast packets. This example application uses the `LOWAPP_FORMAT_GPSAPP` message format for use with a special (not published) Android application. However, this can be easily modified as it only affects the way the `send_gps_coords_from_module` function builds the AT command send request.

# Installation

## Dependencies

The project is based on Semtech's LoRaMac-node project (https://github.com/Lora-net/LoRaMac-node). A few changes were made to the original project. Those changes are available as a patch file to be applied to the 4.3.0 version of the LoRaMac repository.

## Toolchain

The project available in this repository was created using Coocox coIDE v1.7.8, a free IDE for embedded software development. It can be directly imported into a coIDE workspace. The compiler used to build the project was GNU Tools ARM Embedded version 5.4 2016q2.

### Build

Here is a list of symbols that are compiler defined in order to build the project :
* STM32L151CC
* STM32L1XX_MDP
* STM32L151_C
* USE_BAND_868
* USE_HAL_DRIVER
* USE_DEBUGGER
* HARD_FAULT_HANDLER_ENABLED

Compilation was done with the following option:
* -std=c11
* -fdata-sections
* -mcpu=cortex-m3
* -mthumb
* -Wall
* -ffunction-sections
* -g
* -O0

The program is linked with the nano libc library using the `--specs=nano.specs` option to reduce code size.

**Warning : Currently the project needs to be compiled without compiler optimisation. There is a unknown issue causing random communication failures with the SX1272 when optimisation is enabled**

# Communication with the device

AT commands are sent to the device via UART (baud at 19200 bps). 

This can be done either using a standard USB to UART adapter from a computer or from any other microcontroller containing a UART peripheral.

# Doc

Doxygen compliant comments are included throughout the code so that a documentation can be generated automatically.

To generate the documentation, run the following doxygen command :

```
$ doxygen doxygen.doxyfile
```

# License

Copyright 2016 Wyres

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
