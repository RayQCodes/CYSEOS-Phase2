#include "io/io.h"
#include "idt/idt.h"
#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "string/string.h"
#include "fs/pparser.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "memory/memory.h"
#include "gdt/gdt.h"
#include "config.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"
#include "isr80h/isr80h.h"
#include "keyboard/keyboard.h"

uint16_t* video_mem = 0;

uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

void terminal_writechar(char c, char colour)
{
    if (c == '\b')
    {
        if (terminal_col > 0)
        {
            terminal_col--;
            terminal_putchar(terminal_col, terminal_row, ' ', colour);
        }
        return;
    }

    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
    }
    else
    {
        terminal_putchar(terminal_col, terminal_row, c, colour);
        terminal_col += 1;
        if (terminal_col >= VGA_WIDTH)
        {
            terminal_col = 0;
            terminal_row += 1;
        }
    }

    if (terminal_row >= VGA_HEIGHT)
    {
        terminal_row = VGA_HEIGHT - 1;
        for (int i = 0; i < terminal_row * VGA_WIDTH; i++)
        {
            video_mem[i] = video_mem[i + VGA_WIDTH];
        }
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, terminal_row, ' ', colour);
        }
    }
}

void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);

    terminal_row = 0;
    terminal_col = 0;

    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0);
        }
    }
}



void print(const char* str)
{
    size_t len = strlen(str);

    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0;

struct tss tss;

struct gdt gdt_real[PEACHOS_TOTAL_GDT_SEGMENTS];

struct gdt_structured gdt_structured[
    PEACHOS_TOTAL_GDT_SEGMENTS
] =
{
    // NULL segment
    {
        .base = 0x00,
        .limit = 0x00,
        .type = 0x00
    },

    // Kernel code segment
    {
        .base = 0x00,
        .limit = 0xffffffff,
        .type = 0x9a
    },

    // Kernel data segment
    {
        .base = 0x00,
        .limit = 0xffffffff,
        .type = 0x92
    },

    // User code segment
    {
        .base = 0x00,
        .limit = 0xffffffff,
        .type = 0xf8
    },

    // User data segment
    {
        .base = 0x00,
        .limit = 0xffffffff,
        .type = 0xf2
    },

    // TSS segment
    {
        .base = (uint32_t)&tss,
        .limit = sizeof(tss),
        .type = 0xE9
    }
};


void panic(const char * message)
{
	print(message);
	while (1)
	{
	}
}

void kernel_main()
{
    terminal_initialize();

    // Initialize and load the GDT
    memset(gdt_real, 0x00, sizeof(gdt_real));

    gdt_structured_to_gdt(
        gdt_real,
        gdt_structured,
        PEACHOS_TOTAL_GDT_SEGMENTS
    );

    gdt_load(
        gdt_real,
        sizeof(gdt_real)
    );

    // Initialize the heap
    kheap_init();

    // Initialize filesystem layer
    fs_init();

    // Initialize interrupt descriptor table
    idt_init();

    // Setup the TSS
    memset(&tss, 0x00, sizeof(tss));

    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SELECTOR;

    tss_load(0x28);

    // Create kernel paging structure
    kernel_chunk = paging_new_4gb(
        PAGING_IS_WRITEABLE |
        PAGING_IS_PRESENT |
        PAGING_ACCESS_FROM_ALL
    );

    // Switch to kernel paging
    // Keep this version for now.
    // We will change it during the paging-refactor milestone.
    paging_switch(kernel_chunk);

    // Enable paging
    enable_paging();
    
    isr80h_register_commands();
    keyboard_init();
    // Initialize the disk and detect FAT16 filesystem
    disk_search_and_init();

    struct process* process = 0;
    if (process_load("0:/blank.bin", &process) != PEACHOS_ALL_OK)
    {
        panic("Failed to load blank.bin\n");
    }

    process->syscall_permissions =
        PROCESS_PERMISSION_READ | PROCESS_PERMISSION_WRITE | PROCESS_PERMISSION_DELETE;
    process->delete_record_id = 20;
    task_run_first_ever_task();

    // We should never reach this point
    while (1)
    {
    }
}
void kernel_page()
{
kernel_registers();
paging_switch(kernel_chunk);
}
