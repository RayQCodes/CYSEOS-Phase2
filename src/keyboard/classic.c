#include "classic.h"
#include "keyboard.h"
#include "io/io.h"
#include "string/string.h"
#include <stdbool.h>
#include <stdint.h>

int classic_keyboard_init();

static uint8_t shift_keys;
static bool extended_scancode;

static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0',
    '-',
    '=',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    0x0d, 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'',
    '`',
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0',
    '.'
};

struct keyboard classic_keyboard = {
    .name = {"Classic"},
    .init = classic_keyboard_init
};

int classic_keyboard_init()
{
    outb(
        PS2_PORT,
        PS2_COMMAND_ENABLE_FIRST_PORT
    );

    return 0;
}

uint8_t classic_keyboard_scancode_to_char(
    uint8_t scancode
)
{
    if (scancode >= sizeof(keyboard_scan_set_one))
    {
        return 0;
    }

    return keyboard_scan_set_one[scancode];
}

void classic_keyboard_handle_interrupt()
{
    uint8_t scancode = insb(PS2_DATA_PORT);
    if (scancode == 0xE0)
    {
        extended_scancode = true;
        return;
    }
    if (extended_scancode)
    {
        extended_scancode = false;
        return;
    }

    uint8_t key = scancode & 0x7F;
    bool released = scancode & 0x80;
    if (key == 0x2A || key == 0x36)
    {
        uint8_t shift = 1;
        if (key == 0x36)
        {
            shift = 2;
        }
        if (released)
        {
            shift_keys &= ~shift;
        }
        else
        {
            shift_keys |= shift;
        }
        return;
    }
    if (released)
    {
        return;
    }

    char c = classic_keyboard_scancode_to_char(key);
    if (!shift_keys)
    {
        c = tolower(c);
    }
    else if (c == '9')
    {
        c = '(';
    }
    else if (c == '0')
    {
        c = ')';
    }

    if (c)
    {
        keyboard_push(c);
    }
}

struct keyboard* classic_init()
{
    return &classic_keyboard;
}






