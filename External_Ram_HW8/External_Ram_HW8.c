#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/adc.h"  
#include <math.h>



#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS_DAC 17
#define PIN_CS_RAM 15
#define SPI_SCK 18
#define SPI_TX 19
#define pi 3.14159265

//DAC set-up
void writeDAC(int channel, float v);
static inline void cs_select(uint cs_pin);
static inline void cs_deselect(uint cs_pin);
//RAM set-up
void spi_ram_init();
void ram_write_sine();
void spi_ram_write(uint16_t addr, uint8_t * data, int len);
void spi_ram_read(uint16_t addr, uint8_t *data, int len);
void update_dac_from_ram(int i);


int main()
{
    stdio_init_all();
    spi_init(SPI_PORT, 1000 * 1000 * 20); // the baud, or bits per second
    //gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_CS_DAC);
    gpio_set_dir(PIN_CS_DAC, GPIO_OUT);
    gpio_put(PIN_CS_DAC, 1);

 
    gpio_init(PIN_CS_RAM);
    gpio_set_dir(PIN_CS_RAM, GPIO_OUT);
    gpio_put(PIN_CS_RAM, 1);

    spi_ram_init();
    ram_write_sine();

    //1000 values for sine 
    while(true){
        for (int i = 0; i<1000; i++){
            update_dac_from_ram(i*2);
            sleep_ms(1);

        }
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
    cs_select(PIN_CS_DAC);
    spi_write_blocking(SPI_PORT, data, 2); // where data is a uint8_t array with length len
    cs_deselect(PIN_CS_DAC);
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

void spi_ram_init(){
    uint8_t data[2];
    data[0] = 0b00000001;
    data[1] = 0b01000000; //bits 7 and 6 are 01 for sequential mode 
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, data, 2); // where data is a uint8_t array with length len
    cs_deselect(PIN_CS_RAM);
}
void update_dac_from_ram(int i){
    uint8_t data[2];
    spi_ram_read(i, data, 2);

    cs_select(PIN_CS_DAC);
    spi_write_blocking(SPI_PORT,data,2);
    cs_deselect(PIN_CS_DAC);
}
void ram_write_sine(){
    for (int i=0; i<1000; i++){

        float voltage = (sin(2*pi*i/1000.0)+1) * (3.3/2.0);

        uint16_t v = voltage/3.3*1023;

        uint8_t data[2];
        data[0] = 0b01110000;
        data[0] |= (0<<7);
        data[0] |= (v>>6)&0x0F;

        data[1] = (v<<2)&0xFF;
        spi_ram_write(i*2, data,2);
    }
}
void spi_ram_write(uint16_t addr, uint8_t * data, int len){
    uint8_t packet[5]; //addr = 0b111111111111111 (Send first 8 bits)
    packet[0] = 0b00000010; //isntruction, write
    packet[1] = addr>>8; //addr
    packet[2] = addr&0xFF; //addr
    packet[3] = data[0]; //Values you store
    packet[4] = data[1];  //Values you store
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, packet, 5);
    cs_deselect(PIN_CS_RAM);
}
void spi_ram_read(uint16_t addr, uint8_t * data, int len){
    uint8_t packet[5];
    packet[0] = 0b00000011;
    packet[1] = addr >> 8;
    packet[2] = addr&0xFF;
    packet[3] = 0; //Bytes trying to read
    packet[4] = 0; //Bytes trying to read
    uint8_t dst[5];
    cs_select(PIN_CS_RAM);
    spi_write_read_blocking(SPI_PORT, packet, dst, 5);
    cs_deselect(PIN_CS_RAM);
    data[0] = dst[3];
    data[1] = dst[4];
}