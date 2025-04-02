#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "ws2812.pio.h"

#include "bsp/board.h"
#include "tusb.h"
#include "include/leds.h"
#include "include/PicoDebounceButton/PicoDebounceButton.hpp"

static const int I2C_BAUDRATE = 100000; // 100 kHz

#define ROWS 5
#define COLS 2

// Button Pins
#define BUTTON_PIN1 27
#define BUTTON_PIN2 28

#define UART_ID uart0
#define UART_BAUD 31250

#define I2C_SDA 2
#define I2C_SCL 3
#define TXLED 0
#define RXLED 1
#define REDLED 16		// RED LED
#define POWERENABLE 21
#define NEOPWR 17
#define NEOPIXELPIN NEOPIXPIN

// Button Pins and Layout
int Rows[ROWS] = {12, 11, 10, 9, 8};
int Cols[COLS]= {7, 6};
char keys[COLS][ROWS] = {
	{0, 1, 2, 3, 4},
	{5, 6, 7, 8, 9}
};

// GPIO Buttons
auto debounceButtonOne =
    picodebounce::PicoDebounceButton(BUTTON_PIN1, 10, picodebounce::PicoDebounceButton::PRESSED, false);
auto debounceButtonTwo =
    picodebounce::PicoDebounceButton(BUTTON_PIN2, 10, picodebounce::PicoDebounceButton::PRESSED, false);

    void buttonReadTask() {
  // Read the button states
  auto ButtonOneReadState = debounceButtonOne.update();
  auto ButtonTwoReadState = debounceButtonTwo.update();
  // If the button state has changed
  if (ButtonOneReadState) {
    // If the button is pressed
    if (debounceButtonOne.getPressed()) {
        gpio_put(REDLED, true);
        ClearLeds();
    } else {
        gpio_put(REDLED, false);
    }
  }
  if (ButtonTwoReadState) {
    // If the button is pressed
    if (debounceButtonTwo.getPressed()) {
        gpio_put(REDLED, true);
        ClearLeds();
    } else {
        gpio_put(REDLED, false);
    }
  }
}

// Matrix Button/Switches
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
                ledcolors[ID]=urgb_u32(20,20,20);

                // USBMIDI
                tud_midi_n_stream_write(0, 0, msg, 3);
                // HWMIDI
                // uart_putc_raw(UART_ID, msg);// HOW TO FORMAT THE BYTES?
                uart_write_blocking(UART_ID, msg, 3);
            }
            else if (state == BUTTON_UP)
            {
                // do something on button up
                printf("released button %d!\n", ID);
                msg[0] = 0x90;                    // Note On - Channel 1
                msg[1] = 60+ID; // Note Number
                msg[2] = 0;                     // Velocity
                ledcolors[ID]=urgb_u32(20,0,0);
                // USBMIDI
                tud_midi_n_stream_write(0, 0, msg, 3);
                // HWMIDI
                uart_write_blocking(UART_ID, msg, 3);
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

void SetupBoard(){

    SetOut(REDLED);                 // RED LED
	gpio_put(REDLED, 1);            // indicate board is on

	SetOut(POWERENABLE);		        // 5v enable Pin (for MIDI)
	gpio_put(POWERENABLE, 1);	        // Turn 5v enable ON

	SetOut(NEOPWR); 		        // Neopixel power enable Pin
	gpio_put(NEOPWR, 1);	        // Neopixel power ON
	
    SetOut(TXLED);                 // UART TX
    gpio_put(TXLED, 1);

    SetOut(NEOPIXELPIN);

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

    uart_init(UART_ID, UART_BAUD); // HARDWARE MIDI UART
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
    
    rainbow(5);
    ClearLeds();

    uint32_t now=0 ;
    uint32_t lastLedUpdate=0;
    uint32_t ledUpdateInterval=10000;
    int CurrentCol = 0;
    while (true) {
       
        tud_task(); // tinyusb device task
        // led_blinking_task();
        midi_task();
    
        //GPIO buttons
        buttonReadTask();
        
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

        now = time_us_32();  
        if((now-lastLedUpdate)>ledUpdateInterval)
        {
            lastLedUpdate=now;
            SendLeds();
        }
       
        CurrentCol = (CurrentCol  + 1)%COLS;
    }
}

void midi_task(void)
{
  static uint32_t start_ms = 0;
  uint8_t msg[3];

}