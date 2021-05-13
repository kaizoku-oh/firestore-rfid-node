# firestore-rfid-node
IoT node that sends detect RFID tags serial-number and send it to Cloud Firestore

<!-- ![Firestore logo](https://github.com/kaizoku-oh/firestore-rfid-node/blob/main/docs/image/logo.png) -->
<!-- ![](https://github.com/<OWNER>/<REPOSITORY>/workflows/<WORKFLOW_NAME>/badge.svg) -->
![GitHub Build workflow status](https://github.com/kaizoku-oh/firestore-rfid-node/workflows/Build/badge.svg)
![GitHub Release workflow status](https://github.com/kaizoku-oh/firestore-rfid-node/workflows/Release/badge.svg)
[![GitHub release](https://img.shields.io/github/v/release/kaizoku-oh/firestore-rfid-node)](https://github.com/kaizoku-oh/firestore-rfid-node/releases)
[![GitHub issues](https://img.shields.io/github/issues/kaizoku-oh/firestore-rfid-node)](https://github.com/kaizoku-oh/firestore-rfid-node/issues)
![GitHub top language](https://img.shields.io/github/languages/top/kaizoku-oh/firestore-rfid-node)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/kaizoku-oh/firestore-rfid-node/blob/main/LICENSE)
[![Twitter follow](https://img.shields.io/twitter/follow/kaizoku_ouh?style=social)](https://twitter.com/kaizoku_ouh)

## Getting started
This project is created [PlatformIO](https://platformio.org/) IDE using the [esp-idf framework](https://docs.platformio.org/en/latest/frameworks/espidf.html), the project also uses 2 external esp-idf components which are found as submodules under the [components](https://github.com/kaizoku-oh/firestore-rfid-node/tree/main/components) directory.

To get started:
1. Install [PlatformIO Extension for vscode](https://platformio.org/install/ide?install=vscode)
2. Create a new project using ESP-IDF framework
3. Under the root of the project create a new directory called [components](https://docs.platformio.org/en/latest/frameworks/espidf.html#esp-idf-components)
4. Clone this repo
``` bash
$ git clone https://github.com/kaizoku-oh/firestore-rfid-node.git --recursive
```
5. To avoid exposing sensitive data in the code we'll store them in environment variables that'll be when building the project.
#### Window (Powershell)
``` powershell
# Set local environment variables
> $env:WIFI_SSID = '"TYPE_YOUR_WIFI_SSID_HERE"'
> $env:WIFI_PASS = '"TYPE_YOUR_WIFI_PASSWORD_HERE"'
> $env:FIRESTORE_FIREBASE_PROJECT_ID = '"TYPE_YOUR_FIREBASE_PROJECT_ID_HERE"'
> $env:FIRESTORE_FIREBASE_API_KEY = '"TYPE_YOUR_FIREBASE_API_KEY_HERE"'

# OPTIONAL: To read and verify the values of the variables that you just set:
> Get-ChildItem Env:WIFI_SSID
> Get-ChildItem Env:WIFI_PASS
> Get-ChildItem Env:FIRESTORE_FIREBASE_PROJECT_ID
> Get-ChildItem Env:FIRESTORE_FIREBASE_API_KEY
```

#### Linux (Bash)
``` bash
# Set local environment variables
$ export WIFI_SSID='"TYPE_YOUR_WIFI_SSID_HERE"'
$ export WIFI_PASS='"TYPE_YOUR_WIFI_PASS_HERE"'
$ export FIRESTORE_FIREBASE_PROJECT_ID='"TYPE_YOUR_FIREBASE_PROJECT_ID_HERE"'
$ export FIRESTORE_FIREBASE_API_KEY='"TYPE_YOUR_FIREBASE_PROJECT_ID_HERE"'

# OPTIONAL: To read and verify the values of the variables that you just set:
$ echo $WIFI_SSID
$ echo $WIFI_PASS
$ echo $FIRESTORE_FIREBASE_PROJECT_ID
$ echo $FIRESTORE_FIREBASE_API_KEY
```
