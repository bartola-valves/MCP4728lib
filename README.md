# MCP4728 DAC Library for Raspberry Pi Pico

A C++ library for controlling the MCP4728 12-bit quad-channel DAC using the Raspberry Pi Pico.

## Features

- Control up to 4 DAC channels independently
- Support for both VDD and internal voltage reference
- Configurable gain settings (1x or 2x)
- Power-down mode control
- EEPROM storage support
- LDAC pin support for synchronized updates
- Ready/Busy pin monitoring
- Voltage-to-value and value-to-voltage conversion utilities

## Hardware Connections

Default pin connections:
- SDA: GPIO4 (Pin 6)
- SCL: GPIO5 (Pin 7)
- LDAC: GPIO6 (Pin 9, optional)
- RDY: GPIO7 (Pin 10, optional)
- VDD: 5V
- GND: Ground

## Quick Start

```cpp
#include "MCP4728.h"

// Create MCP4728 instance with default settings
MCP4728 dac;

void setup() {
    // Initialize the DAC
    if (!dac.begin()) {
        printf("DAC initialization failed!\n");
        return;
    }

    // Set channel A to 2.5V using VDD reference
    dac.setVoltage(MCP4728::CHANNEL_A, 2.5, MCP4728::VREF_VDD, MCP4728::GAIN_1X);
}
```

## Documentation

See [DESIGN.md](DESIGN.md) for detailed technical information and implementation details.

## License

This library is released under the MIT License.
