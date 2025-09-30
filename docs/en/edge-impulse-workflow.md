# Edge Impulse Workflow

## Project Setup
1. Create a new Edge Impulse project and enable the Data Acquisition tab.
2. Define labels `case0`–`case3` to mirror on-device capture modes.
3. Upload at least 20 windows per class captured from the SD card exporter.

## Model Training
- Start with the default `Spectral Analysis` + `NN Classifier` impulse.
- Use a 1,000 Hz sampling rate and a 1,000 sample window (1 s) to match firmware buffers.
- Target <25 kB RAM usage so the compiled library fits alongside Wi-Fi and MQTT stacks.

## Deployment
- Export as "Arduino Library" after each training batch.
- Replace `lib/EdgeAI_inferencing` in this repo and rebuild the data logging and MQTT sketches.
- Validate on-device with `edge-impulse-run-impulse --raw` to compare serial logits against Studio results.
