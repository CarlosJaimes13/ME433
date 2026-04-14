#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "hardware/adc.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

#define LED_PIN PICO_DEFAULT_LED_PIN


void draw_letter(unsigned char x, unsigned char y, char character);
void draw_message(unsigned char x, unsigned char y, char *arr);

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    //LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    ssd1306_setup();
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    //initialize adc
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    ssd1306_clear();
    uint64_t start = to_us_since_boot(get_absolute_time());

    //Read ADC
    uint16_t adc_value = adc_read();
    float volts = adc_value * 3.3 /4095.0;
    char message[50];
    sprintf(message, "ADC0 = %.2f V", volts);
    draw_message(0,0, message);
    ssd1306_update();

    uint64_t end = to_us_since_boot(get_absolute_time());
    float fps = 1000000.0/ (end-start);

    char fps_message[50];
    sprintf(fps_message, "FPS = %.1f", fps);
    draw_message(0,24,fps_message);
    ssd1306_update();
    sleep_ms(20000);

    ssd1306_clear();
    ssd1306_update();
    /* 
    int i = 15;
    sprintf(message, "Hello Lizzy McSizzy");
    draw_message(10,10,message);
    */

    //draw_letter(20,10, 'B');
    /*
    while (true) {
       /* gpio_put(LED_PIN, 0);
        ssd1306_drawPixel(64,16,0);
        ssd1306_update();

        sleep_ms(500);

        gpio_put(LED_PIN, 0);
        ssd1306_drawPixel(64,16,0);
        ssd1306_update();
        sleep_ms(500);
    }
    */
}
void draw_letter(unsigned char x, unsigned char y, char character){
    //need loops for columns and rows
    int col = 0;
    int row = 0;
    //Start index at 0 since the ascii table starts at 0x20
    int index = character - 0x20;

    for(col = 0; col <5; col++){
        //Make each column for each character
         unsigned char column =ASCII[index][col];
        for (row = 0; row <8; row ++){
            if((column>>row) & 0x01){
                ssd1306_drawPixel(x + col,y +row,1);
            }
            else {
                ssd1306_drawPixel(x + col,y +row,0);
            }
        }

    }
}
void draw_message(unsigned char x, unsigned char y, char *arr){
    int i = 0;
    while (arr[i] != '\0'){
        draw_letter(x+(6*i), y, arr[i]); //You want there to be a space for each letter
        i ++;
    }
}




