#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/adc.h"  
#include <math.h>



#define SPI_PORT spi0
#define SPI_RX 16
#define PIN_CS 17
#define SPI_SCK 18
#define SPI_TX 19
#define pi 3.14159265


void writeDAC(int channel, float v);
static inline void cs_select(uint cs_pin);
static inline void cs_deselect(uint cs_pin);

int main()
{
    stdio_init_all();
    spi_init(SPI_PORT, 1000 * 1000); // the baud, or bits per second
    //gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

   // adc_init();
   // adc_gpio_init(26);
   // adc_gpio_init(27);

    //cs_select(PIN_CS);
   // spi_write_blocking(SPI_PORT, data, len); // where data is a uint8_t array with length len
    //cs_deselect(PIN_CS);

    //Sine Steps
    float t = 0;
    float dt = 0.005; //5 ms

    //Triangle Steps
    float tri = 0.0;
    float step = 0.033;
    int direction = 1;

    while (true) {
        //call writeDAC
        //Sine (2Hz)
       float voltage = (sin(2*pi*2*t)+1)*(3.3/2.0);
       writeDAC(0, voltage);
      // adc_select_input(0);
      // uint16_t rawA = adc_read();


        //Triangle 
        tri += direction * step;

        if (tri >= 3.3) {
            tri = 3.3;
            direction = -1;
        }
        if (tri <= 0.0){
            tri = 0.0;
            direction = 1;
        }
        writeDAC(1, tri);
       // adc_select_input(1);
       // uint16_t rawB = adc_read();

       //float voltA = rawA*3.3/4095.0;
      // float voltB = rawB*3.3/4095.0;

       // printf("A = %.2f V, B = %.2f V\n", voltA, voltB);
        sleep_ms(5);
        t += dt;
    }
}


void writeDAC(int channel, float v){

    uint8_t data[2];
    if (v < 0){
        v=0;
    }
    if ( v > 3.3) {
        v = 3.3;
    }
    uint16_t myV = v/3.3*1023; //0b11111111

    data[0] = 0b01110000;

    data[0] = data[0] | ((channel&0b1)<<7); //Put channel in bit 
    data [0] = data[0] | (myV>>6)&0b00001111;

    data[1] = (myV<<2) & 0xFF; //0b11111100
  
    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, 2); // where data is a uint8_t array with length len
    cs_deselect(PIN_CS);
}
static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}