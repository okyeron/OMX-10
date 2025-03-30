#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include "bsp/board.h"
#include "tusb.h"

static const int I2C_BAUDRATE = 100000; // 100 kHz

#define ROWS 5
#define COLS 2

// Button Pins
#define BUTTON_PIN1 27
#define BUTTON_PIN2 28

const int LED_COUNT = 10;
const int LED_PIN = 24;
const int LED_BRIGHTNESS = 90;

const uint8_t I2C_SDA = 2;
const uint8_t I2C_SCL = 3;
const uint8_t TXLED = 0;
const uint8_t RXLED = 1;
const int REDLED = 16;		// RED LED
const int BLUELED = 18;		// BLUE LED
const int NEOPIXPIN = 24;
const int POWERENABLE = 21;
const int NEOPWR = 17;

int Rows[ROWS] = {12, 11, 10, 9, 8};
int Cols[COLS]= {7, 6};

char keys[COLS][ROWS] = {
	{0, 1, 2, 3, 4},
	{5, 6, 7, 8, 9}
	};

#define DEBOUNCE 3

enum{
    BUTTON_PRESSED,
    BUTTON_RELEASED,
    BUTTON_DOWN,
    BUTTON_UP,
    __BUTTON_STATES
};

class ButtonState

{
    public:

    ButtonState()
    {

    }
    int ID;
    int state = BUTTON_UP;
    bool newstate = false;
    int statecount = 0;
    uint8_t msg[3];
    void SetState(int S)
    {
        if (state!= S)
        {
            state = S;
            if (state == BUTTON_DOWN)
            {
                // do something on button down
                printf("pressed button %d!\n", ID);
                // Send Note On for current position at full velocity (127) on channel 1.
                msg[0] = 0x90;                    // Note On - Channel 1
                msg[1] = 60+ID; // Note Number
                msg[2] = 127;                     // Velocity
                tud_midi_n_stream_write(0, 0, msg, 3);
            }
            else
            {
                // do something on button up
                printf("released button %d!\n", ID);

            }
        }
    }
    void Update(bool D)
    {
        if (D!=newstate)
        {
            newstate = D;
            statecount  = 0;
        }
        else{
            statecount ++;
            if (statecount == DEBOUNCE)
            {
                if (newstate)
                {
                    SetState(BUTTON_DOWN);
                }
                else
                {
                    SetState(BUTTON_UP);
                }
                statecount = DEBOUNCE+1;
            }
        }
    }
};

ButtonState AllButtons[ROWS][COLS];

void midi_task(void);

void SetOut(int p)
{
    gpio_init(p);
    gpio_set_dir(p, GPIO_OUT);
}

void SetIn(int p, bool pullup)
{
    gpio_init(p);
    gpio_set_dir(p, GPIO_IN);
    if (pullup)
        gpio_pull_up(p);
}

PIO LED_pio = pio0;
uint LED_sm = 0;
#define IS_RGBW false

void LED_init()
{
    int ledoff = pio_add_program(LED_pio, &ws2812_program);
    ws2812_program_init(LED_pio, LED_sm, ledoff, NEOPIXPIN, 800000, false);

}

void SetupBoard(){

    SetOut(REDLED);                 // RED LED
	gpio_put(REDLED, 1);            // indicate board is on

	SetOut(POWERENABLE);		        // 5v enable Pin
	gpio_put(POWERENABLE, 1);	        // Turn 5v enable ON

	SetOut(NEOPWR); 		        // Neopixel power enable Pin
	gpio_put(NEOPWR, 1);	        // Neopixel power ON
	
    SetOut(NEOPWR);                 // UART TX
    gpio_put(NEOPWR, 1);

    SetOut(NEOPIXPIN);

      // BUTTONS
    SetIn(BUTTON_PIN1, true);
    SetIn(BUTTON_PIN2, true);

    // i2c
    gpio_init(I2C_SDA);
    gpio_init(I2C_SCL);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
	gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    i2c_init(i2c1, I2C_BAUDRATE);

    
    uart_init(uart0, 31250); // HARDWARE MIDI  UART
    // Set the TX and RX for HARDWARE MIDI
    gpio_set_function(TXLED, GPIO_FUNC_UART);
    gpio_set_function(RXLED, GPIO_FUNC_UART);
  
    for(int r =0;r<ROWS;r++)
    {
        gpio_init(Rows[r]);
        gpio_set_dir(Rows[r], false);
        gpio_pull_down(Rows[r]);
    }
    for(int c =0;c<COLS;c++)
    {
        gpio_init(Cols[c]);
        gpio_set_dir(Cols[c], true);
    }

    for(int r =0;r<ROWS;r++)
    {
        for(int c =0;c<COLS;c++)
        {
            AllButtons[r][c].ID = keys[c][r];//r * ROWS + c;
        }
    }
    LED_init();
}

int main()
{
    stdio_init_all();
    printf("Hello, Button Matrix!\n");
    
    SetupBoard();
    tusb_init();


    int CurrentCol = 0;
    while (true) {
        tud_task(); // tinyusb device task
        // led_blinking_task();
        midi_task();
    
        
        for(int c =0;c<COLS;c++)
        {
            if (c== CurrentCol)
            {
                gpio_put(Cols[c], true);
                //gpio_set_dir(Cols[c], true);
            }
            else{
                gpio_put(Cols[c], false);
               // gpio_set_dir(Cols[c], false);
                //gpio_pull_down(Cols[c]);
            }
        }

        sleep_us(100);
    
        for(int r =0;r<ROWS;r++)
        {
            AllButtons[r][CurrentCol].Update(gpio_get(Rows[r]));
        }

        CurrentCol = (CurrentCol  + 1)%COLS;
    }
}

void midi_task(void)
{
  static uint32_t start_ms = 0;
  uint8_t msg[3];

}