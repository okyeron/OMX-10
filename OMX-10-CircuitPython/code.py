import board
import keypad
import neopixel
import usb_hid
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keycode import Keycode


# neopixel colors
RED = (255, 0, 0)
ORANGE = (255, 127, 0)
YELLOW = (255, 255, 0)
GREEN = (0, 255, 0)
AQUA = (0, 255, 255)
BLUE = (0, 0, 255)
PURPLE = (127, 0, 255)
PINK = (255, 0, 255)
OFF = (50, 50, 50)

# axis states selected with keys 9-11
axis_states = [0, "x", "y", "z"]
state = axis_states[0]
# keymap for key matrix
keymap = {
    (0): ("0", BLUE), 
    (1): ("5", ORANGE), 
    (2): ("1", RED), 
    (3): ("6", GREEN), 
    (4): ("2", AQUA), 
    (5): ("7", BLUE), 
    (6): ("3", AQUA), 
    (7): ("8", PURPLE), 
    (8): ("4", PINK), 
    (9): ("9", RED), 

}

# make a keyboard
kbd = Keyboard(usb_hid.devices)
# key matrix
COLUMNS = 5
ROWS = 2
keys = keypad.KeyMatrix(
    row_pins=(board.D12, board.D11, board.D10, board.D9, board.D8),
    column_pins=(board.D7, board.D6),
    columns_to_anodes=True,
)
# neopixels and key num to pixel function
pixels = neopixel.NeoPixel(board.NEOPIXEL, 12, brightness=0.3)
def key_to_pixel_map(key_number):
    row = key_number // COLUMNS
    column = key_number % COLUMNS
    if row % 2 == 1:
        column = COLUMNS - column - 1
    return row * COLUMNS + column
pixels.fill(OFF)  # Begin with pixels off.


while True:
    # poll for key event
    key_event = keys.events.get()

    if key_event:
        print(keymap[key_event.key_number][0])

        
#     # if a key event..
#     if key_event:
#         # print(key_event)
#         # ..and it's pressed..
#         if key_event.pressed:
#             # ..and it's keys 0-8, send key presses from keymap:
#             if keymap[key_event.key_number][0] is axis_states[0]:
#                 state = axis_states[0]
#                 kbd.press(*keymap[key_event.key_number][1])
#             # ..and it's key 9, set state to x
#             if keymap[key_event.key_number][0] is axis_states[1]:
#                 state = axis_states[1]
#                 pixels[key_to_pixel_map(10)] = OFF
#                 pixels[key_to_pixel_map(11)] = OFF
#             # ..and it's key 10, set state to y
#             if keymap[key_event.key_number][0] is axis_states[2]:
#                 state = axis_states[2]
#                 pixels[key_to_pixel_map(9)] = OFF
#                 pixels[key_to_pixel_map(11)] = OFF
#             # ..and it's key 11, set state to z
#             if keymap[key_event.key_number][0] is axis_states[3]:
#                 state = axis_states[3]
#                 pixels[key_to_pixel_map(9)] = OFF
#                 pixels[key_to_pixel_map(10)] = OFF
#             # turn on neopixel for key with color from keymap
#             pixels[key_to_pixel_map(key_event.key_number)] = keymap[key_event.key_number][2]
#         # ..and it's released..
#         if key_event.released:
#             # if it's key 0-8, release the key press and turn off neopixel
#             if keymap[key_event.key_number][0] is axis_states[0]:
#                 kbd.release(*keymap[key_event.key_number][1])
#                 pixels.fill(OFF)