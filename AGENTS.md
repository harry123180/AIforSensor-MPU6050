# Repository Guidelines

## Project Structure & Module Organization
- `CollectData`, `CollectDataV2`, `MPU6050Code`, `Inferencing`, and `MQTTwithAI` contain the primary Raspberry Pi Pico W sketches; each folder holds a single `.ino` entry point for one workflow (data capture, training, inference, or MQTT streaming).
- `BTNCode`, `RGBLEDCode`, `WifiConnector`, `DHT11MQTT`, and the archived PM2.5 examples provide focused sensor or peripheral demos; reuse helpers from these sketches instead of duplicating logic.
- `PCB/` stores board layout images, and the localized guides in `docs/` describe hardware assembly and data formats—update them whenever pinouts or peripherals change.

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
