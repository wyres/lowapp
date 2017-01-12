# LoWAPP

## What is LoWAPP ?

LoWAPP is a LoWPAN type protocol, similar to Zigbee or Thread, but focused on providing the peer-peer interactions rather than generic mesh routing functionality up to some central server. Unlike these protocols, it does not require any ‘special’ function nodes to communicate between peers, nor does it require any nodes that are permanently awake and therefore powered. 

The protocol is a micro-sampling type protocol based on a LoRa radio hardware layer. Devices wake up for a small period of time at regular intervals to listen for incomming messages. Outgoing messages are preceeded by long preamble.

This repository contains the core of the protocol, a simulation implementation and a hardware implementation of the protocol.

## The core of the protocol

The core of the protocol is located in the `lowapp/` folder. The core is designed to be portable and can be run on any type of hardware.

It requires a set of system functions in order to work. These functions are sent to the initialisation functions and are the bridge between the protocol and the hardware running below it.

`lowapp_utils`contains a series of utility functions used by the core. It includes definitions of circular buffer, hex/dec/ascii conversion functions and CRC computation.

The gateway functionality described in the specification is currently **not implemented** in the LoWAPP core, therefore devices can only communicate with each other within the same LoWAPP group and cannot send messages to other networks.

### Format of the messages

Two main format of messages are available in the core. They are selected through a preprocessor constant:

  * `LOWAPP_MSG_FORMAT_CLASSIC` : In this mode, messages are simple ASCII text based messages. Received packets are returned to the application as a JSON object. This is the main mode that should be used in the general case.
  * `LOWAPP_MSG_FORMAT_GPSAPP` : This mode was developped to communicate both text message and GPS data to an Android application (not published). The format of the send requests and received responses are described in the implementation document from the `Doc/`folder.

## Simulation

A simulation has been developed using Linux processes and threads, communicating through text files instead of the radio.

The `simulation` folder contains the simulation, including the Eclipse project files. More informations can be found in the simulation specific [README.md file](simulation/README.md).

## Hardware implementation

A hardware implementation aimed at running on a custom board with an STM32L151CC microcontroller and a Semtech SX1272 LoRa radio chip.

The `hardware`folder contains this implementation, including the Coocox CoIDE project files. More informations can be found in the hardware specific [README.md file](hardware/README.md).

## Doc

Doxygen compliant comments are included throughout the code so that a documentation can be generated automatically.

To generate the documentation, run the following doxygen command :

```
$ doxygen doxygen.doxyfile
```

Both the simulation and the hardware implementation projects have their own doxygen configuration. Please run the command above in each directories to generate both documentations.

Also included are :
* The functional specification of the LoWAPP protocol
* An implementation document describing the inner working of the protocol and the simulation
* A validation report made from testing the simulation and the hardware implementation

## License

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