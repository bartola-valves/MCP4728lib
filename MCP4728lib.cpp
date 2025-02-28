// MCP4728lib.cpp

/*
 * Design specification included in the DESIGN.md file

 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h> // Added for round() function

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4   // GPIO4 (pin 6)
#define I2C_SCL_PIN 5   // GPIO5 (pin 7)
#define I2C_FREQ 100000 // 100 kHz

// additional connections of the MCP4728
// VDD to 5V
// GND to GND
// SDA to GPIO4
// SCL to GPIO5
// LDAC to GPIO6
// RDY to GPIO7

#define LDAC_PIN 6 // GPIO6 (pin 9)
#define RDY_PIN 7  // GPIO7 (pin 10)

// MCP4728 I2C address (default is 0x60)
#define MCP4728_ADDR 0x60

// MCP4728 commands
#define MCP4728_CMD_WRITE_DAC 0x40
#define MCP4728_CMD_WRITE_DAC_EEPROM 0x58
#define MCP4728_CMD_MULTI_WRITE 0x50

// DAC channels
typedef enum
{
    CHANNEL_A = 0,
    CHANNEL_B = 1,
    CHANNEL_C = 2,
    CHANNEL_D = 3
} mcp4728_channel_t;

// Reference voltage options
typedef enum
{
    VREF_VDD = 0, // Use VDD as reference. My VDD is 5V for this DAC.
    VREF_INT = 1  // Use internal 2.048V reference
} mcp4728_vref_t;

// Gain options
typedef enum
{
    GAIN_1X = 0, // 1x gain
    GAIN_2X = 1  // 2x gain
} mcp4728_gain_t;

// Function to initialize I2C for the MCP4728
void mcp4728_init()
{
    // Initialize I2C
    i2c_init(I2C_PORT, I2C_FREQ);

    // Set up I2C pins
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    // Enable pull-ups on I2C pins (recommended for I2C)
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Initialize LDAC pin as output and set it high (inactive)
    gpio_init(LDAC_PIN);
    gpio_set_dir(LDAC_PIN, GPIO_OUT);
    gpio_put(LDAC_PIN, 1);

    // Initialize RDY pin as input
    gpio_init(RDY_PIN);
    gpio_set_dir(RDY_PIN, GPIO_IN);
    gpio_pull_up(RDY_PIN);

    printf("I2C and control pins initialized for MCP4728\n");
}

// Function to set a single channel
bool mcp4728_set_channel(mcp4728_channel_t channel, uint16_t value,
                         mcp4728_vref_t vref, mcp4728_gain_t gain)
{
    // Limit value to 12 bits (0-4095)
    value &= 0x0FFF;

    uint8_t buffer[3];

    // First byte: Command + Channel + VREF + Power-Down bits
    buffer[0] = MCP4728_CMD_WRITE_DAC | (channel << 1);

    // Second byte: Upper 8 bits of the 12-bit value
    // Upper data bits + VREF + Gain + Power-down mode
    buffer[1] = ((vref & 0x01) << 7) | ((gain & 0x01) << 4) | (value >> 8);

    // Third byte: Lower 8 bits of the 12-bit value
    buffer[2] = value & 0xFF;

    // Send the data to the DAC
    int result = i2c_write_blocking(I2C_PORT, MCP4728_ADDR, buffer, 3, false);

    return (result == 3);
}

// Function to set all four channels at once (faster than setting individually)
bool mcp4728_set_all_channels(uint16_t values[4], mcp4728_vref_t vref, mcp4728_gain_t gain)
{
    uint8_t buffer[9];

    // Sequential write command byte
    buffer[0] = MCP4728_CMD_MULTI_WRITE;

    for (int i = 0; i < 4; i++)
    {
        // Limit value to 12 bits
        values[i] &= 0x0FFF;

        // Upper data bits + VREF + Gain + Power-down mode
        buffer[i * 2 + 1] = ((vref & 0x01) << 7) | ((gain & 0x01) << 4) | (values[i] >> 8);

        // Lower 8 bits of the 12-bit value
        buffer[i * 2 + 2] = values[i] & 0xFF;
    }

    // Debug: Print buffer contents
    printf("I2C buffer: ");
    for (int i = 0; i < 9; i++)
    {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");

    // Send the data to the DAC
    int result = i2c_write_blocking(I2C_PORT, MCP4728_ADDR, buffer, 9, false);
    printf("I2C write result: %d (expected 9)\n", result);

    return (result == 9);
}

// Convert a voltage to DAC value based on reference voltage
uint16_t mcp4728_voltage_to_value(float voltage, float vref, mcp4728_gain_t gain)
{
    float max_voltage = vref;
    if (gain == GAIN_2X)
        max_voltage *= 2.0;

    // Calculate the DAC value (0-4095) from the voltage
    float normalized_value = voltage / max_voltage;

    // Use round() instead of truncating for better precision
    uint16_t dac_value = (uint16_t)round(normalized_value * 4095.0);

    // Clamp value to valid range
    if (dac_value > 4095)
        dac_value = 4095;

    return dac_value;
}

// Function to trigger LDAC (updates all DAC outputs simultaneously)
void mcp4728_trigger_ldac()
{
    gpio_put(LDAC_PIN, 0); // Pull LDAC low
    busy_wait_us(1);       // Wait for at least 100ns
    gpio_put(LDAC_PIN, 1); // Return LDAC high
}

// Add this function to your code. TEsting the I2C bus to find the MCP4728
void i2c_scan()
{
    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr_prefix = 0; addr_prefix < 8; addr_prefix++)
    {
        printf("%d0:", addr_prefix);
        for (int addr_suffix = 0; addr_suffix < 16; addr_suffix++)
        {
            int addr = (addr_prefix << 4) | addr_suffix;

            // Skip reserved addresses
            if ((addr >= 0x00 && addr <= 0x07) || addr >= 0x78)
            {
                printf("   ");
                continue;
            }

            // Try to detect device
            uint8_t rxdata;
            int ret = i2c_read_blocking(I2C_PORT, addr, &rxdata, 1, false);

            printf(ret >= 0 ? " %02X" : " --", addr);
        }
        printf("\n");
    }
}

int main()
{
    stdio_init_all();
    busy_wait_ms(2000);

    printf("MCP4728 DAC Test - Calibration Mode with Op-Amp Compensation\n");

    // Initialize I2C
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Initialize LDAC pin
    gpio_init(LDAC_PIN);
    gpio_set_dir(LDAC_PIN, GPIO_OUT);
    gpio_put(LDAC_PIN, 1);

    // Initialize RDY pin
    gpio_init(RDY_PIN);
    gpio_set_dir(RDY_PIN, GPIO_IN);
    gpio_pull_up(RDY_PIN);

    // First run a scan to verify the device is present
    i2c_scan();

    printf("\nStarting calibration pattern using internal reference with 1x gain\n");
    printf("Max DAC voltage: 2.048V (being scaled by op-amp gain of 2.4414 to 5V output)\n");

    // Op-amp gain factor
    const float OP_AMP_GAIN = 2.4414f; // 5V / 2.048V

    // We'll use these arrays to store our alternating values
    uint16_t values_a[2], values_b[2], values_c[2];
    uint16_t value_d;

    // Calculate DAC values, compensating for op-amp gain
    // For each target voltage, calculate DAC voltage = target voltage / op-amp gain

    // OUTA: 0V and 5V after op-amp
    values_a[0] = mcp4728_voltage_to_value(0.0 / OP_AMP_GAIN, 2.048, GAIN_1X); // 0V
    values_a[1] = mcp4728_voltage_to_value(5.0 / OP_AMP_GAIN, 2.048, GAIN_1X); // 2.048V

    // OUTB: 1V and 3V after op-amp
    values_b[0] = mcp4728_voltage_to_value(1.0 / OP_AMP_GAIN, 2.048, GAIN_1X); // ~0.41V
    values_b[1] = mcp4728_voltage_to_value(3.0 / OP_AMP_GAIN, 2.048, GAIN_1X); // ~1.23V

    // OUTC: 2V and 4V after op-amp
    values_c[0] = mcp4728_voltage_to_value(2.0 / OP_AMP_GAIN, 2.048, GAIN_1X); // ~0.82V
    values_c[1] = mcp4728_voltage_to_value(4.0 / OP_AMP_GAIN, 2.048, GAIN_1X); // ~1.64V

    // OUTD: fixed at 5V after op-amp
    value_d = mcp4728_voltage_to_value(5.0 / OP_AMP_GAIN, 2.048, GAIN_1X); // 2.048V

    // Print the calculated values and corresponding DAC voltages
    printf("OUTA values: %u (0V -> 0V), %u (2.048V -> 5V)\n", values_a[0], values_a[1]);
    printf("OUTB values: %u (0.41V -> 1V), %u (1.23V -> 3V)\n", values_b[0], values_b[1]);
    printf("OUTC values: %u (0.82V -> 2V), %u (1.64V -> 4V)\n", values_c[0], values_c[1]);
    printf("OUTD value: %u (2.048V -> 5V)\n", value_d);

    printf("\nStarting alternating pattern (3 sec interval)\n");

    // Set OUTD to fixed 5V
    mcp4728_set_channel(CHANNEL_D, value_d, VREF_INT, GAIN_1X);

    // Calibration loop
    bool toggle = false;
    while (true)
    {
        // Update values based on toggle state
        uint16_t all_values[4];
        all_values[0] = values_a[toggle ? 1 : 0]; // OUTA: 0V or 5V
        all_values[1] = values_b[toggle ? 1 : 0]; // OUTB: 1V or 3V
        all_values[2] = values_c[toggle ? 1 : 0]; // OUTC: 2V or 4V
        all_values[3] = value_d;                  // OUTD: fixed 5V

        // Set all channels at once
        if (mcp4728_set_all_channels(all_values, VREF_INT, GAIN_1X))
        {
            printf("Outputs set to: %s\n",
                   toggle ? "OUTA=5V, OUTB=3V, OUTC=4V, OUTD=5V" : "OUTA=0V, OUTB=1V, OUTC=2V, OUTD=5V");
        }
        else
        {
            printf("Failed to set outputs\n");
        }

        // Wait 3 seconds before toggling
        busy_wait_ms(3000);

        // Toggle for next iteration
        toggle = !toggle;
    }
}