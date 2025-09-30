# Expansion Board Hardware Layout

## GPIO Map
| Module | Signal | Pico 2 W Pin | Notes |
|--------|--------|---------------|-------|
| RGB LED | R | GP0 | Active-high, 330 Ω recommended |
|         | G | GP1 | Active-high |
|         | B | GP2 | Active-high |
| Buttons | BTN1 | GP4 | Pull-down with 10 kΩ resistor |
|         | BTN2 | GP5 | Pull-down with 10 kΩ resistor |
| SD Card (SPI1) | CS | GP9 | Chip select |
|                | SCK | GP10 | 10 MHz max |
|                | MOSI | GP11 | Data to SD |
|                | MISO | GP12 | Data from SD |
| MPU6050 (I2C1) | SDA | GP14 | Requires 4.7 kΩ pull-up |
|                | SCL | GP15 | Requires 4.7 kΩ pull-up |

## Assembly Tips
- Route the MPU6050 module away from Wi-Fi antenna to reduce RF noise.
- Tie sensor grounds together near the Pico to minimise reference drift in acceleration readings.
- Keep ribbon cables under 20 cm to maintain SPI signal integrity for SD logging.
