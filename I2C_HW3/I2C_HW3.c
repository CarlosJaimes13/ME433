#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

#define LED_PIN 7
#define Button_pin 0 

#define MCP_ADDR 0x20
#define IODIR 0x00

void init_mcp();

#define OLAT 0x0A
#define GPIO 0x09
void setPin(unsigned char address, unsigned char reg, unsigned char value);
unsigned char readPin(unsigned char address, unsigned char reg);

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c
    sleep_ms(100);
    init_mcp();

 // Test Function for GP7 Blinking
    while (true) {
       unsigned char gpio_val = readPin(MCP_ADDR, GPIO);

       if ((gpio_val & 0x01) ==0) {
            setPin(MCP_ADDR, OLAT, 0x80);
       } else{
            setPin(MCP_ADDR, OLAT, 0x00);
       }

       printf("GPIO = 0x%02X\n", gpio_val);
       sleep_ms(100);
    }
} 


void init_mcp(){
    setPin(MCP_ADDR, IODIR, 0x7F);
}

void setPin(unsigned char address, unsigned char reg, unsigned char value) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    i2c_write_blocking(I2C_PORT, address, buf, 2, false);
}

unsigned char readPin(unsigned char address, unsigned char reg) {
    uint8_t value; 
    i2c_write_blocking(I2C_PORT, address, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, address, &value, 1, false);
    return value;
}
/*
void write_led(uint8_t value){
    uint8_t buf[2];
    buf[0] = OLAT;
    buf[1] = value;
    i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 2, false);
*/

