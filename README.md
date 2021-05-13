# pio-rfid
RFID example using ESP32 and PlatformIO esp-idf framework

## How to use?
### Window (Powershell)
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

### Linux (Bash)
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
