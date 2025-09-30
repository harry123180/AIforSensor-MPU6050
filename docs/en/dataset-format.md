# IMU Dataset Format

## File Naming
```
case<id>.sample<index>.csv
```
- `id` denotes the motion class captured on the device (0–3 by default).
- `index` increments per recording session; reset the board to clear the counter.

## CSV Layout
```
time_ms,acc_x_g,acc_y_g,acc_z_g
0,0.01,-0.03,1.02
1,0.02,-0.04,1.01
...
```
- Units are milliseconds and G-forces derived from the ±2 g full-scale range.
- Each file stores 1,000 samples (1 second at 1 kHz); pad shorter captures with trailing rows marked `NaN`.
- Preserve the comma delimiter so Edge Impulse's uploader accepts the batch import.

## Quality Checklist
- Cross-check timestamps for monotonic increments; resend data if jitter exceeds 3 ms.
- Capture at least three sessions per class before training to avoid imbalance.
- Archive raw SD card dumps under `data/raw` with the capture date in ISO format (e.g., `2025-09-30`).
