mqtt-examples
==
An example project using MQTT to query and set pins on the CC3200 
Launchapd

MQTT Message Format
-------------
 Pxx:X
  - xx is a GPIO Pin number
  - X is a command
    - H : High
    - L : Low
    - ? : Read Back Input Pin
    
Supported Pins
 - GPIO22 - SW2 - Input
 - GPIO13 - SW3 - Input
 - GPIO03 - AP Jumper - Input
 - GPIO11 - Green LED - Output
 - GPIO10 - Orange LED - Output
 - GPIO09 - Red LED - Output

Examples
```
C:\Program Files (x86)\mosquitto>mosquitto_pub.exe -h test.mosquitto.org 
-p 1883 -t ticc3200 -m P13:?
C:\Program Files (x86)\mosquitto>mosquitto_pub.exe -h test.mosquitto.org 
-p 1883 -t ticc3200 -m P09:H
```
To see status of input pin queries:
```
C:\Program Files (x86)\mosquitto>mosquitto_sub.exe -h test.mosquitto.org 
-p 1883 -t ticc3200
```

Notes:
I2C disabled, if you want to control Orange and/or Green LEDs, remove 
Jumpers J2 & J3 in Top Left

