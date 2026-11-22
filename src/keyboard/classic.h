#ifndef CLASSIC_KEYBOARD_H
#define CLASSIC_KEYBOARD_H

#define PS2_PORT 0x64
#define PS2_DATA_PORT 0x60
#define PS2_COMMAND_ENABLE_FIRST_PORT 0xAE

struct keyboard* classic_init();
void classic_keyboard_handle_interrupt();

#endif
