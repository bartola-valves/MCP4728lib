# Design configuration and specifications of the MCP4728

1. MCP4728 powered with +5V
2. SCL and SDA using I2C port
3. LDAC managed by GPIO port
4. Output from RDY/BSY is connected via GPIO port
5. VOUTA to VOUTD are connected to non-inverting amplifiers to produce 0 to 5V output signals
6. A 74CB3T3245DW is used to level shift the SCL, SDA, LDAC and RDY signals between 3V3 and 5V

