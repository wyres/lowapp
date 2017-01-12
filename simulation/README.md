# LoWAPP Simulation

This folder contains a simulation of the LoWAPP protocol using Linux processes and threads communicating through files.

## Structure of the project

```
.
|-- Log
|-- Nodes
|-- Radio
`-- src
    |-- boards
    |-- lowapp
    |   |-- lowapp_core
    |   |-- lowapp_shared_res
    |   |-- lowapp_sys
    |   `-- lowapp_utils
    |-- mac
    |-- radio
    `-- system

```

You might have to create the Log/ and Radio/ folders by hand before starting the simulation.

### Source folder

The `src/` folder contains the source code of the project. It contains the LoWAPP core, as well as the simulation implementation of the system level functions required for the core to run.

#### LoWAPP Core

The core of the protocol is stored next to this simulation, in the root directory of the repository (`lowapp/`). It is present in the project as linked resource in `src/lowapp/`.

#### LoWAPP Simulation System Implementation

The other subfolders of `src/` contain the implementation of the system level functions required by the core. The functions directly called by the core are stored in `src/lowapp/lowapp_sys/` while underlying functionality are stored in the other folders.

Functionalities present in those system level implementation:
  * Managing the threads
  * Communication through the `Radio/` files (simulating the LoRa radio)
  * Sending and receiving data with the stdin/stdout streams (simulating UART for AT commands)
  * Reading and writing configuration files (simulating the EEPROM persistant memory)

### Nodes

The `Nodes/` folder contains a series of node configuration files whose name are valid UUIDs (like `3f26c561-1c24-49f6-b9db-6414fd245a8a`).

### Radio

The `Radio/` folder is designed to store the text files used for simulating radio communication. When a node starts transmission, it creates a file called `channel-X` with X the frequency used for transmission. An empty file represents a preamble and a non-empty file represents an ongoing transmission. End of transmission is simulated as the file being deleted.

Channel activity detection (CAD) and reception is managed through the use a Linux program called `inotify` (see [Manual page of inotify](http://man7.org/linux/man-pages/man7/inotify.7.html)). This allows a node's process to poll for any change to the `Radio/` folder so that we do not have to waste time looping on a check for the existance of files.

## Installation

### Dependencies

The program is linked to several libraries, most of them are included in most Linux distributions :

* `librt` for using Linux timers
* `libmath` for the math library
* `libpthread` for multi-threading
* `libuuid` for managing UUIDs. You night have to install the `uuid-dev` package on Ubuntu using `sudo apt-get install uuid-dev`

### Toolchain

This project was developped using Eclipse Neon 4.6.0 CDT (C/C++ Development Tooling) on a Linux Operating System (Ubuntu 16.04 LTS). It was installed using the `eclipse-inst-linux64` script from the Eclipse website and selecting "Eclipse IDE for C/C++ Developper".

GCC 5.4.0 was used for compilation. This was the version included in the standard Ubuntu repository at the time.

The python scripts used for launching nodes and parsing results are all written for Python 3 (tested on Python 3.5.2)

### Build

For building the binary, you should run Eclipse, import the project from the Git repository and then build it using the `Debug` build configuration.

## Usage

You can run the binary generated in the `Debug/` folder directly with some parameters.

```
$ Debug/lowapp-simu --help
Usage: lowapp-simu [OPTION...]
lowapp_simu -- Simulation running LoRa-based LoWAPP protocol as Linux
processes

  -c, --config=CONFIG_FILE   Relative path to the node configuration file from
                             DIRECTORY or working directory
  -d, --directory=DIRECTORY  Root directory of the simulation (with Radio/ and
                             Nodes/ subdirectories)
  -u, --uuid=UUID            UUID of the node file, stored in DIRECTORY/Nodes/
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

By using a single `-u/-uuid` parameter, the node would be using the current folder as root directory for the simulation. This means that the main `Nodes/`, `Radio/`, and `Stats/` folder are used for reading and writing data :
```
$ Debug/lowapp-simu -u 3f26c561-1c24-49f6-b9db-6414fd245a8a
```

It is also possible to specify another root directory with the `-d/-directory` option. This is especially usefull for running several groups of nodes with no interactions between the groups (specific statistics, nodes, logs and radio files).

Once the program is running, you can type AT commands directly in the console and press Enter to send it to the device.

## Doc

Doxygen compliant comments are included throughout the code so that a documentation can be generated automatically.

To generate the documentation, run the following doxygen command :

```
$ doxygen doxygen.doxyfile
```

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
  