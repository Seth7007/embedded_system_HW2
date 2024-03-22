# embedded_system HW2

This is embedded_system HW2

A program runs on STM32 will read 3D accelerator and 3D gyro sensor value 
and another program will run a socket server on host to receive the sensor data and visualize it.

Programs is included in the repository:
  main.cpp, socket_server.py
  
main.cpp:
The program is modified by a mbed-os socket example.
Thus, to make the program work correctly the example(URL:https://github.com/ARMmbed/mbed-os-example-sockets) should be imported into mbed studio first.
Next, replace the main.cpp with the one in this repository.
Last but not least, BSP library(URL:http://developer.mbed.org/teams/ST/code/BSP_B-L475E-IOT01/) should be imported to read sensor value
After doing that, this part of program is ready to run.

socket_server.py:
The program runs a socket server on the host. To make the server work properly, make sure to change the IP address to the current one.
```
  HOST = '140.112.248.225' -> should be change to current IP.
```

Finally, one should modify `mbed_app.json` to make things go smoothly
To connect to internet, wifi SSID and password should be provided.
To connect to socket, value under hostname under config should be change to the IP address used in socket_server.py
```
"config": {
        "hostname": {
            "help": "The demo will try to connect to this web address on port 80 (or port 443 when using tls).",
            "value": "\"140.112.248.225\"" //this should be changed to the IP address used in socket_server.py
        },
        "use-tls-socket": {
            "value": false
        }
    },
```
With all setting properly, running the program will make STM32 scan the neighboring wifi AP and try to connect to the one specified in `mbed_app.json`.
Once the connection is set up successfully, STM32 will try to connect to the socket server runs on host.
If STM32 connects to that server, it start to send sensor data continually to the server and the server program will visiualize the data it receive.
To stop sending data, just press the user button on STM32 and it will stop sending data and end the program.
However, this might crash the server program, so if one is desired to run the program again, the socket_server.py should be re-ran again. 
