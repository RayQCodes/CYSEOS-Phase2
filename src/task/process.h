#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "task.h"
#include "config.h"
#include "kernel.h"

enum process_syscall_permissions
{
    PROCESS_PERMISSION_READ = 1 << 0,
    PROCESS_PERMISSION_DELETE = 1 << 1,
    PROCESS_PERMISSION_WRITE = 1 << 2
};

struct process
{
    // Process ID
    uint16_t id;

    // Executable filename
    char filename[PEACHOS_MAX_PATH];

    // Main task owned by this process
    struct task* task;

    // Memory allocations owned by the process
    void* allocations[PEACHOS_MAX_PROGRAM_ALLOCATIONS];

    // Physical pointer to loaded program data
    void* ptr;

    // Physical pointer to user stack memory
    void* stack;

    // Size of loaded program data
    uint32_t size;


   struct keyboard_buffer
{
    char buffer[PEACHOS_KEYBOARD_BUFFER_SIZE];
    int tail;
    int head;
} keyboard;

    uint32_t syscall_permissions;
    uint32_t delete_record_id; // Record covered by this process's DELETE permission.
};

struct process* process_current();
struct process* process_get(int process_id);

int process_load_for_slot(
    const char* filename,
    struct process** process,
    int process_slot
);


int process_load(
    const char* filename,
    struct process** process
);

int process_load_for_slot(
    const char* filename,
    struct process** process,
    int process_slot
);




#endif
