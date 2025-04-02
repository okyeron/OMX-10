#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

#define RGB(r, g, b) ((((int)(r)) << 8) + (((int)(g)) << 16) + (((int)(b))))
#define RGB32(R, G, B) RGB(R, G, B)

const int LED_COUNT = 10;
const int LED_BRIGHTNESS = 90;
const int LED_PIN = 24;
#define NEOPIXPIN 24
int ledcolors[10]={0,0,0,0,0,0,0,0,0,0};
int numPixels = 10;

PIO LED_pio = pio0;
uint LED_sm = 0;
uint LED_offset=0;
#define IS_RGBW false

void LED_init()
{
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &LED_pio, &LED_sm, &LED_offset, NEOPIXPIN, 1, true);
    hard_assert(success);

    //ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    ws2812_program_init(LED_pio, LED_sm, LED_offset, NEOPIXPIN, 800000, false);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

uint32_t colorHSV(uint16_t hue, uint8_t sat, uint8_t val)
{

    uint8_t r, g, b;
    //   hue = (hue * 1530 + 32768) / 65536;
    hue = (hue * 1530L + 32768) / 65536;  
    if (hue < 510)
    { // Red to Green-1
        b = 0;
        if (hue < 255)
        { //   Red to Yellow-1
            r = 255;
            g = hue; //     g = 0 to 254
        }
        else
        {                  //   Yellow to Green-1
            r = 510 - hue; //     r = 255 to 1
            g = 255;
        }
    }

    else if (hue < 1020)
    { // Green to Blue-1
        r = 0;
        if (hue < 765)
        { //   Green to Cyan-1
            g = 255;
            b = hue - 510; //     b = 0 to 254
        }
        else
        {                   //   Cyan to Blue-1
            g = 1020 - hue; //     g = 255 to 1
            b = 255;
        }
    }
    else if (hue < 1530)
    { // Blue to Red-1
        g = 0;
        if (hue < 1275)
        {                   //   Blue to Magenta-1
            r = hue - 1020; //     r = 0 to 254
            b = 255;
        }
        else
        { //   Magenta to Red-1
            r = 255;
            b = 1530 - hue; //     b = 255 to 1
        }
    }
    else
    { // Last 0.5 Red (quicker than % operator)
        r = 255;
        g = 0;
        b = 0;
    }
    // Apply saturation and value to R,G,B, pack into 32-bit result:
    uint32_t v1 = 1 + val;  // 1 to 256; allows >>8 instead of /255
    uint16_t s1 = 1 + sat;  // 1 to 256; same reason
    uint8_t s2 = 255 - sat; // 255 to 0
                            //   return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
                            //   		(((((g * s1) >> 8) + s2) * v1) & 0xff00) |
                            // 		(((((b * s1) >> 8) + s2) * v1) >> 8);
    return (RGB32((((((r * s1) >> 8) + s2) * v1) >> 8),
                  (((((g * s1) >> 8) + s2) * v1) >> 8),
                  (((((b * s1) >> 8) + s2) * v1) >> 8)));
}


void SendLeds()
{
    for(int i=0;i<numPixels;i++)
    {   
        put_pixel(LED_pio, LED_sm, ledcolors[i]);
    }
    // sleep_ms(1);
};
void ClearLeds(){

    for (int i = 0; i < numPixels; i++){
        ledcolors[i] = urgb_u32(0,0,0);    
    }
    SendLeds();
}

void rainbow(int wait)
{
    for (long firstPixelHue = 0; firstPixelHue < 1 * 65536; firstPixelHue += 256)
	{
		for (int i = 0; i < numPixels; i++)
		{
    	    int pixelHue = firstPixelHue + (i * 65536 / numPixels);
        	ledcolors[i] = colorHSV(pixelHue, 255, 64); 
		}
		SendLeds(); // Update leds
		sleep_ms(wait);  // Pause for a miliseconds
	}
}

