#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
// sensor data registers:
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75

#define CHIP_ADDRESS 0x68
#define GPIO 0x09

unsigned char readPin(unsigned char address, unsigned char reg);
void setPin(unsigned char address, unsigned char reg, unsigned char value);
void mpu6050_init();
void burst_read(uint8_t *data);
int16_t combine_bytes(uint8_t first_byte, uint8_t second_byte);

//Drawing 
void draw_line(int x0, int y0, int x1, int y1);

int main()
{
    stdio_init_all();
    sleep_ms(10000);
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    //ssd1306 setup
    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();
    

    //MPU initialize
    mpu6050_init();
    while(true){
    //READ ALL DATA 
    uint8_t data[14];
    burst_read(data);
    //Combine Bytes
    //convert to units 
    int16_t ax_raw = combine_bytes(data[0], data[1]);
    int16_t ay_raw = combine_bytes(data[2], data[3]);
    int16_t az_raw = combine_bytes(data[4], data[5]);
    float ax_g = ax_raw * 0.000061;
    float ay_g = ay_raw * 0.000061;

    //SCALING 
    int cx = 64;
    int cy = 16;
    float scale = 15;
    int x_end = cx - (int)(ax_g * scale);
    int y_end = cy + (int)(ay_g * scale);

    if (x_end < 0) x_end = 0;
    if (x_end > 127) x_end = 127;
    if (y_end < 0) y_end = 0;
    if (y_end > 31) y_end = 31;

    //DRAW GRAPH
    ssd1306_clear();
    ssd1306_drawPixel;(cx,cy,1);
    draw_line(cx, cy, x_end, y_end);
    ssd1306_update();

    float az_g = az_raw * 0.000061;

    int16_t temp_raw = combine_bytes(data[6], data[7]);
    float temp_c = (temp_raw / 340.0) + 36.53;

    int16_t gx_raw = combine_bytes(data[8], data[9]);
    int16_t gy_raw = combine_bytes(data[10], data[11]);
    int16_t gz_raw = combine_bytes(data[12], data[13]);
    float gx_dps = gx_raw * 0.007630;
    float gy_dps = gy_raw * 0.007630;
    float gz_dps = gz_raw * 0.007630;

    printf("AX: %f g, AY: %f g, AZ: %f g\n", ax_g, ay_g, az_g);
    printf("GX: %f dps, GY: %f dps, GZ: %f dps\n", gx_dps, gy_dps, gz_dps);
    printf("TEMP: %f C\n\n", temp_c);

    //100 Hz
    sleep_ms(10);
    /* CHECK CHIP CODE
    unsigned char who_am_i = readPin(CHIP_ADDRESS, WHO_AM_I);
    printf("WHO_AM_I = 0x%02X\n", who_am_i);
        */
 
    }
}

//FUNCTIONS FROM PREVIOUS HW
unsigned char readPin(unsigned char address, unsigned char reg) {
    uint8_t value; 
    i2c_write_blocking(I2C_PORT, address, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, address, &value, 1, false);
    return value;
}

void setPin(unsigned char address, unsigned char reg, unsigned char value) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    i2c_write_blocking(I2C_PORT, address, buf, 2, false);
}

void mpu6050_init(){
    setPin(CHIP_ADDRESS, PWR_MGMT_1, 0x00);
    sleep_ms(10);
    setPin(CHIP_ADDRESS,ACCEL_CONFIG, 0x00);
    //FS_SEL = 3 (11 in binary -> (11000) -> HEX = 0x18
    setPin(CHIP_ADDRESS, GYRO_CONFIG, 0x18);
}

void burst_read(uint8_t *data){
    uint8_t reg = ACCEL_XOUT_H;
    i2c_write_blocking(I2C_PORT, CHIP_ADDRESS, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, CHIP_ADDRESS, data, 14, false);
}

int16_t combine_bytes(uint8_t first_byte, uint8_t second_byte){
    int16_t value;
    value = (first_byte <<8)|second_byte;
    return value;
}

void draw_line(int x0, int y0, int x1, int y1){
    int steps = abs(x1-x0);
    if (abs(y1-y0) > steps) steps = abs(y1-y0);
    for (int i = 0; i  <= steps; i++){
        int x = x0 + (x1-x0) * i / steps;
        int y =y0 + (y1-y0) * i / steps;       
        ssd1306_drawPixel(x,y,1);
    }

}
