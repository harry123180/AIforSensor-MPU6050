# Repository Guidelines

## Project Structure & Module Organization
- Target board: **Raspberry Pi Pico 2 W** (FQBN `rp2040:rp2040:pico_w`). Arduino IDE requires each folder name to match its single `.ino` entry point.
- **Primary sketches** (core teaching flow): `CollectData/`, `CollectDataV2/`, `Inferencing/`, `MQTTwithAI/`. These drive the end-to-end path from data capture → model training → on-device inference → MQTT publishing.
- **Sensor & peripheral demos**: `BTNCode/`, `RGBLEDCode/`, `SDCardWrite/`, `MPU6050Code/`, `example/`. Use them to verify wiring and to crib working snippets — do not duplicate this logic into primary sketches.
- **Reusable templates**: `WifiConnector/` (standalone Wi-Fi AP provisioning skeleton), `DHT11MQTT/` (parallel non-IMU MQTT example). Refactor toward these when extracting shared Wi-Fi/MQTT code.
- `Data/` stores collected CSV samples; `PCB/` stores board Gerber + schematic; `docs/` holds zh-TW/en guides, slide decks, and the `EdgeImpulseCourse/` archive. Update hardware docs whenever pinouts or peripherals change.
- Every directory ships its own `README.md`. When adding a new folder, add a `README.md` alongside the new `.ino`.

## Build, Test, and Development Commands
- Open sketches in Arduino IDE 2.x with board `Raspberry Pi Pico W`, or run `arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectData/CollectData.ino` to build headlessly.
- Upload with `arduino-cli upload --fqbn rp2040:rp2040:pico_w --port <COMx> Inferencing/Inferencing.ino` after selecting the correct serial port.
- Synchronize libraries via `arduino-cli lib install Button2 PubSubClient ArduinoJson`; refresh the exported Edge Impulse library (`EdgeAI_inferencing.h`) whenever the model is retrained.

## Coding Style & Naming Conventions
- Use two-space indents, braces on the same line, camelCase for functions, snake_case for longer-lived buffers, and uppercase for macros or constant pin assignments.
- Keep configuration blocks (Wi-Fi, MQTT, sensor addresses) near the top of each sketch and note measurement units in comments.
- Prefix Serial diagnostics with short tags such as `[INFO]` or `[ERR]` to keep logs concise and easy to filter.

## Testing Guidelines
- There are no automated tests; validate on hardware by checking SD card writes, LED states, serial output at 115200 baud, and MQTT publishes at the expected 1 kHz sampling cadence.
- Capture a 10-second sample with `edge-impulse-data-forwarder` when verifying new models and confirm classifier labels against bench motions.
- Document any deviations from the default I2C address (0x68) or wiring in the relevant sketch comments and companion guides.

## Commit & Pull Request Guidelines
- Existing history uses compact summaries like `upload`; continue with a single imperative line (for example, `Add MQTT reconnect guard`) that describes the change set.
- Reference related hardware tickets or Edge Impulse experiments in the commit body, and include validation evidence in PRs (serial logs, broker screenshots, or power measurements).
- PR descriptions should explain what changed, how it was tested, and any configuration steps teammates must repeat before flashing.
