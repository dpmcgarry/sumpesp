# SumpESP

A simple IoT project for the ESP32 microcontroller written in C using the ESP-IDF, FreeRTOS, and AWS IoT.

## Background 

For some reason I have gone through multiple sump pumps in my house which are costly to replace, not a fun job to do, and has the potential for a large "blast radius" if undetected. I have observed two failure modes for the pumps in the past:

1. Pump runs continuously (due to stuck float switch or some other reason) causing the pump to eventually overheat and burn itself out / lose production
1. Pump gets clogged / overwhelmed from water volume and water level in the sump becomes unmanageable for the pump to recover

This project is intended to be a basic IoT monitor for a sump pump using an ESP32 microcontroller. The plan is to combine two sensors:

* SCT-013 Non-invasive AC Current Sensor / Split Core Transformer
* Ultrasonic Distance Sensor

The thought is that the AC current sensor can be an early 'canary' for the health of the pump.  Hopefully I can use this metric to quickly detect when the pump is running for longer periods and/or it stuck on and take action (either manually or through smarthome integration).  The ultrasonic distance sensor will measure the level of water in the sump. I hope to use this data for some type of longer term analytic to determine a baseline sump level. Once I get that baseline I'm thinking I can correlate the slope of the sump water level to rain data to get a better metric of pump health over time.

## Why This Approach?

Here are some basic requirements I came up with:

1. I wanted something simple that wouldn't add to the number of devices on my network I needed to keep a patch regimen on (which ruled out yet another pi). 
1. I  wanted this to be simple, reliable, and resilient to power loss (not needing a UPS to prevent data corruption). 
1. I wanted something that could have backup power from a LiPo battery with a built in charger
1. I wanted something 'frugal'
1. When I started looking at some of the packaged FreeRTOS examples for AWS IoT and they seemed to be geared towards demonstrating all the functionality of FreeRTOS but that backed you into older versions of the ESP-IDP so I decided to build something using the native IDF from the ground up to hopefully keep closer track to the mainline of ExpressIF's implementation.
1. I could have used one of the Pi's I have doing other tasks as an IoT gateway using GreenGrass, but I am already using this for a couple of other projects and wanted this one to go VFR direct to the cloud versus through a proxy since it seemed to be more reliable.

## The Plan

1. Get the basics working (Network Connectivity, Logging, Sensors, AWS IoT Core)
1. Deploy one of the ESP's to start reporting data to AWS and work on the backend alarming, visualization, and analytics
1. Get alarming working for lack of data (e.g. let me know if the thing stops working)
1. Move away from relying on hardcoded values for SSID, Password, AWS Endpoint, and Certs at compile time.  My current 'thought' it to make a companion app to provision devices with.  We shall see...

## Tooling
* Developing on Ubuntu 22 LTS
* ESP-IDF 4.4.1
* VSCode w/ ESP-IDF extension

## Getting Started
* Create your own aws_clientcredential_keys.h in esp/include
* Create your own KConfig.projbuild in esp/main
* Setup the IDF version stated above
* run idf.py build in the esp folder
* Flash your ESP32

## ToDo:
* Go back and add sr04 support back in
* 