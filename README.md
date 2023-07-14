# Simple-IOT-System using ESP32
Simple example of an IOT system that receive environment data (temperature, humid,...) and send to a website (local). Here I create 3 arrays for the Temperature, Humidity and Rain. If you have sensors, you can use real life data.
# Configuration
In this project, I use MQTT service of [Flespi](https://flespi.io). If you don't have an account, feel free to go and register one. After that, create a MQTT board with username is the token you received after creating account, and password is optional.\
![image](https://github.com/Taiga-chann/Simple-IOT-System/assets/90364299/93d18291-9f5b-48d5-9158-3a0e5ff2427f)

In app_main.c, you need to change configuration parameter.
Find ![image](https://github.com/Taiga-chann/Simple-IOT-System/assets/90364299/e22ecf2b-3cf0-446b-8907-3b783564c8cf)\
and modify the .username and .password field to user name and password in previous step.
And that it's, we done here.Now we move to [my local website]() used for data visualization to modify a little bit.
