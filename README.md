# Affordable Sensor-Enabled Medical Training Manikin

An affordable sensor-enabled medical training manikin designed to provide objective feedback during CPR and basic emergency-response training.

The prototype combines force, displacement, pressure, and pulse-simulation sensing with an ESP32-based embedded system. A Bluetooth connection is used to transmit training data for monitoring and analysis.

## Repository Contents

| File | Description |
|---|---|
| [`main.ino`](./main.ino) | ESP32 firmware for sensor acquisition, processing, training modes, and Bluetooth communication |
| [`Ecology Report Team 4.pdf`](./Ecology%20Report%20Team%204.pdf) | Project report containing the system design, hardware, software, testing, cost analysis, limitations, and future scope |

## Key Features

- CPR compression force measurement using load cells and HX711
- Chest compression depth measurement using a Hall Effect sensor and magnet
- Rescue-breathing pressure measurement
- Pulse simulation using a vibration motor
- ESP32-based sensor acquisition and control
- Bluetooth communication for transmitting training data
- Moving-average filtering for sensor readings
- Calibration support for sensor measurements
- Separate training and test operating modes
- JSON-based data packets for structured communication

## Embedded Techniques

### Event-Driven Bluetooth Communication

Bluetooth commands are handled through a callback instead of continuously polling for incoming commands. This allows the firmware to react when a command is received.

### State-Based Operation

The firmware uses operating states to control different stages of the training process. This keeps the behavior separated into clearly defined modes.

### Non-Blocking Timing

The firmware uses Arduino timing functions such as `millis()` for timed operations without continuously blocking the main program.

### Sensor Filtering

A moving-average approach is used to reduce short-term fluctuations in sensor readings before they are used by the application.

### Structured Data Communication

Sensor and training information is packaged as JSON before being transmitted over Bluetooth.

## Technologies and Libraries

- **ESP32 Arduino Core** for microcontroller development
- **BluetoothSerial** for Bluetooth Classic communication
- **ArduinoJson** for JSON serialization and parsing
- **HX711** for interfacing with load-cell ADC modules
- **Arduino GPIO and timing APIs** for digital I/O and timing

## System Overview

The embedded system follows this general data flow:

```text
Sensors
   |
   v
ESP32
   |
   +--> Sensor Reading
   |
   +--> Filtering and Processing
   |
   +--> Training Logic
   |
   v
JSON Data
   |
   v
Bluetooth
```

## Sensors

### Compression Force

Load cells measure the force applied during chest compressions. An HX711 module provides the interface between the load cells and the ESP32.

### Compression Depth

A Hall Effect sensor and magnet are used to detect chest displacement during compression.

### Rescue Breathing

An air pressure sensor is used to detect pressure associated with rescue breathing.

### Pulse Simulation

A vibration motor is used to provide a physical pulse simulation during the training process.

## Communication Protocol

The firmware uses Bluetooth Classic and exchanges structured JSON messages.

Example command:

```json
{
  "command": "start"
}
```

Example sensor data structure:

```json
{
  "force": 45.2,
  "depth": 5.4,
  "pressure": 550
}
```

The exact fields and commands supported by the current firmware are defined in [`main.ino`](./main.ino).

## Project Documentation

The complete design and implementation details are available in [`Ecology Report Team 4.pdf`](./Ecology%20Report%20Team%204.pdf).

The report covers:

- Problem motivation
- System architecture
- Hardware design
- Sensor selection
- Embedded firmware
- Power system
- Manikin construction
- Mobile application concept
- Testing and validation
- Cost analysis
- Limitations
- Future scope

## Cost

The reported prototype cost is approximately ₹3,670.

## Future Scope

The project report identifies several possible improvements:

- More realistic chest compression behavior
- Improved pulse monitoring
- AED training functionality
- Additional first-aid training modules
- Hand-placement detection
- Improved mechanical durability
- AI-assisted training guidance

## Project Status

This repository contains the current embedded firmware and the project documentation for the prototype.

## Disclaimer

This is an academic engineering prototype intended for training and demonstration purposes. It is not a certified medical device.
