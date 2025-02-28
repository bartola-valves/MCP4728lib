/**
 * @file basic_usage.cpp
 * @brief Basic example of using the MCP4728 DAC library
 */

#include "MCP4728.h"
#include <stdio.h>

int main()
{
    stdio_init_all();
    busy_wait_ms(2000); // Wait for serial connection

    printf("MCP4728 Basic Usage Example\n");

    // Create DAC instance with default settings
    MCP4728 dac;

    // Initialize the DAC
    if (!dac.begin())
    {
        printf("Failed to initialize DAC\n");
        return -1;
    }

    // Example 1: Setting individual channels
    printf("\nExample 1: Individual channel control\n");
    dac.setVoltage(MCP4728::CHANNEL_A, 1.0, MCP4728::VREF_VDD, MCP4728::GAIN_1X);
    printf("Channel A set to 1.0V\n");

    // Example 2: Setting all channels at once
    printf("\nExample 2: All channels simultaneous update\n");
    uint16_t values[4] = {
        dac.voltageToValue(0.5, 5.0, MCP4728::GAIN_1X),
        dac.voltageToValue(1.0, 5.0, MCP4728::GAIN_1X),
        dac.voltageToValue(1.5, 5.0, MCP4728::GAIN_1X),
        dac.voltageToValue(2.0, 5.0, MCP4728::GAIN_1X)};

    if (dac.setAllChannels(values, MCP4728::VREF_VDD, MCP4728::GAIN_1X))
    {
        printf("All channels updated\n");
    }

    // Example 3: Using LDAC for synchronized output
    printf("\nExample 3: Synchronized update using LDAC\n");
    // Setup new values but don't update outputs yet
    dac.setChannel(MCP4728::CHANNEL_A, 2048, MCP4728::VREF_VDD, MCP4728::GAIN_1X);
    dac.setChannel(MCP4728::CHANNEL_B, 3072, MCP4728::VREF_VDD, MCP4728::GAIN_1X);
    dac.setChannel(MCP4728::CHANNEL_C, 1024, MCP4728::VREF_VDD, MCP4728::GAIN_1X);
    dac.setChannel(MCP4728::CHANNEL_D, 4095, MCP4728::VREF_VDD, MCP4728::GAIN_1X);

    printf("Values loaded, triggering LDAC...\n");
    dac.triggerLDAC(); // Update all outputs simultaneously

    while (1)
    {
        tight_loop_contents();
    }
}
