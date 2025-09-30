# Raspberry Pi Pico 2 W Wi-Fi Guide

## Overview
- CYW43439 module offers 2.4 GHz IEEE 802.11b/g/n connectivity for the Pico 2 W.
- Supports station and soft-AP modes for provisioning the MQTT + Edge Impulse workflow.
- WPA2/WPA3 security is recommended; keep credentials below 31 characters to fit EEPROM buffers.

## Practical Notes
- Typical throughput: ~9 Mbps near the access point; plan for sub-6 Mbps indoors.
- Idle current stays below 50 mA; full radio bursts reach 80 mA at 5.15 V—budget power traces accordingly.
- When acting as an AP during configuration, set the channel manually to avoid 2.4 GHz interference with the IMU cable harness.
