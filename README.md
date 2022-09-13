DAS.UHR Firmware
================

This repository contains the source code of the firmware for the DAS.UHR word clock.

The Espressif IoT Development Framework (ESP-IDF) is needed for compiling and flashing.

When using Visual Studio Code, run "EDF-ISP: Add vscode configuation folder" and adjust the settings in `.vscode/settings.json`.

Example:

    {
        ...
        "C_Cpp.intelliSenseEngine": "Tag Parser",
        "idf.flashType": "UART",
        "idf.portWin": "COM4",
    }


License
-------

Copyright 2012-2022 Daniel A. Spilker

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
