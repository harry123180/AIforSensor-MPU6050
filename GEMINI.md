# Project Overview

This repository contains an AIoT project for education, utilizing a Raspberry Pi Pico 2 W and an MPU6050 IMU sensor. The system is designed to collect motion data at 1 kHz, store it on an SD card, and use this data to train a TinyML model with Edge Impulse. The trained model can then perform real-time inference on the device, with results being published over Wi-Fi via the MQTT protocol.

The project is documented in both English and Traditional Chinese, covering hardware setup, data collection, model training, and deployment.

> **Scope of this file**: `GEMINI.md` is a compact, machine-friendly overview for LLM-based assistants. Human readers should start from [`README.md`](README.md); contributor conventions live in [`AGENTS.md`](AGENTS.md).

## Building and Running

The firmware is developed for the Arduino platform. You can use either the Arduino IDE or the `arduino-cli` command-line tool.

**Board Setup:**
- **Board:** Raspberry Pi Pico 2 W (`rp2040:rp2040:pico_w`)
- **Arduino IDE Board Manager URL:** `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

**Required Libraries:**
```bash
arduino-cli lib install Button2 PubSubClient ArduinoJson
```
The Edge Impulse model is bundled as an Arduino library under `Inferencing/` and `MQTTwithAI/`. After exporting a new model from Edge Impulse Studio, replace the existing library files and update the `#include` header name.

**Key Firmware Sketches:**
- `CollectData/CollectData.ino` — collects training data from the MPU6050 and writes CSV to SD card.
- `Inferencing/Inferencing.ino` — runs the deployed Edge Impulse model and prints classification results to USB serial.
- `MQTTwithAI/MQTTwithAI.ino` — integrates inference with Wi-Fi provisioning and MQTT publishing.

For a complete module index (including demo sketches, datasets, and PCB assets) see the "Repository Layout" section of `README.md`. Per-folder `README.md` files describe pins, libraries, and commands.

**Example Build & Upload Commands:**
```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectData/CollectData.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <YOUR_COM_PORT> Inferencing/Inferencing.ino
```

## Development Conventions

- Firmware is written in C/C++ for the Arduino framework.
- Each workflow lives in its own folder with a single `.ino` entry point (Arduino IDE requirement: folder name must match the sketch filename).
- Two-space indent; `camelCase` for functions; `snake_case` for long-lived buffers; `UPPER_CASE` for macros and pin constants (see `AGENTS.md` for the full style guide).
- Detailed documentation is kept under `docs/` (zh-TW and en), plus one `README.md` per sketch folder.
- `README.md` is the primary entry point for humans; this file is the compact overview for AI assistants.
