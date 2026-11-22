#include "task.h"
#include "kernel.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "process.h"
#include "idt/idt.h"
#include "string/string.h"
#include "memory/paging/paging.h"
// The current task that is running
struct task* current_task = 0;

// Task linked list
struct task* task_tail = 0;
struct task* task_head = 0;

// Internal function prototype
int task_init(struct task* task, struct process* process);


// Returns the task currently considered active.
struct task* task_current()
{
    return current_task;
}


// Creates a new task and adds it to the task linked list.
struct task* task_new(struct process* process)
{
    int res = 0;

    struct task* task =
        kzalloc(sizeof(struct task));

    if (!task)
    {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);

    if (res != PEACHOS_ALL_OK)
    {
        goto out;
    }

    // If this is the first task,
    // it becomes both the head and tail.
    if (task_head == 0)
    {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }

    // Otherwise append it to the end
    // of the doubly linked task list.
    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;

out:
    if (ISERR(res))
    {
        if (task)
        {
            task_free(task);
        }

        return ERROR(res);
    }

    return task;
}


// Returns the next task for round-robin scheduling.
struct task* task_get_next()
{
    if (!current_task)
    {
        return task_head;
    }

    if (!current_task->next)
    {
        return task_head;
    }

    return current_task->next;
}


// Removes a task from the linked list.
static void task_list_remove(struct task* task)
{
    if (!task)
    {
        return;
    }

    if (task->prev)
    {
        task->prev->next = task->next;
    }

    if (task->next)
    {
        task->next->prev = task->prev;
    }

    if (task == task_head)
    {
        task_head = task->next;
    }

    if (task == task_tail)
    {
        task_tail = task->prev;
    }

    if (task == current_task)
    {
        current_task = task_get_next();
    }
}


// Frees a task and its paging resources.
int task_free(struct task* task)
{
    if (!task)
    {
        return -EINVARG;
    }

    if (task->page_directory)
    {
        paging_free_4gb(task->page_directory);
    }

    task_list_remove(task);

    kfree(task);

    return PEACHOS_ALL_OK;
}


// Initializes a task's paging and initial CPU state.
int task_init(struct task* task, struct process* process)
{
    if (!task)
    {
        return -EINVARG;
    }

    memset(task, 0, sizeof(struct task));
    task->process = process;
    // Create a 4GB page directory for this task.
    task->page_directory =
        paging_new_4gb(
            PAGING_IS_PRESENT |
            PAGING_ACCESS_FROM_ALL
        );

    if (!task->page_directory)
    {
        return -EIO;
    }

    // The user program will eventually begin
    // execution at 0x400000.
    task->registers.ip =
        PEACHOS_PROGRAM_VIRTUAL_ADDRESS;

    // User-mode stack segment.
    task->registers.ss =
        USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    // Initial user-mode stack pointer.
    task->registers.esp =
        PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    return PEACHOS_ALL_OK;
}


int task_switch(struct task* task)
{
    current_task = task;

    paging_switch(
        task->page_directory
    );

    return 0;
}


void task_run_first_ever_task()
{
    if (!current_task)
    {
        panic(
            "task_run_first_ever_task(): "
            "No current task exists!\n"
        );
    }

    task_switch(task_head);

    task_return(
        &task_head->registers
    );
}

int task_page()
{
    user_registers();

    task_switch(current_task);

    return 0;
}

void task_save_state(
    struct task* task,
    struct interrupt_frame* frame
)
{
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;

    task->registers.eax = frame->eax;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.ecx = frame->ecx;
    task->registers.edi = frame->edi;
    task->registers.edx = frame->edx;
    task->registers.esi = frame->esi;
}

void task_current_save_state(
    struct interrupt_frame* frame
)
{
    if (!task_current())
    {
        panic("No current task to save\n");
    }

    struct task* task = task_current();
    task_save_state(task, frame);
}


int copy_string_from_task(
    struct task* task,
    void* virtual,
    void* phys,
    int max
)
{
    if (max <= 0 || max >= PAGING_PAGE_SIZE)
    {
        return -EINVARG;
    }

    struct process* process = task->process;
    uintptr_t address = (uintptr_t)virtual;
    uintptr_t program_start = PEACHOS_PROGRAM_VIRTUAL_ADDRESS;
    uintptr_t stack_start = PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END;
    uintptr_t stack_end = PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    const char* source;
    size_t available;

    // The binary loader maps these two process-owned buffers into the task.
    if (address >= program_start && address - program_start < process->size)
    {
        size_t offset = address - program_start;
        source = (const char*)process->ptr + offset;
        available = process->size - offset;
    }
    else if (address >= stack_start && address < stack_end)
    {
        source = (const char*)process->stack + (address - stack_start);
        available = stack_end - address;
    }
    else
    {
        return -EINVARG;
    }

    if (available > (size_t)max)
    {
        available = max;
    }
    int length = strnlen(source, available);
    if (length == (int)available)
    {
        return -EINVARG;
    }
    strncpy(phys, source, length + 1);
    return 0;
}

int task_page_task(struct task* task)
{
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

int task_get_stack_item(struct task* task, unsigned int index, uint32_t* value)
{
    uintptr_t stack_pointer = task->registers.esp;
    uintptr_t stack_start = PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END;
    uintptr_t stack_end = PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    if (stack_pointer < stack_start || stack_pointer >= stack_end ||
        index >= (stack_end - stack_pointer) / sizeof(*value))
    {
        return -EINVARG;
    }

    char* stack = task->process->stack;
    size_t offset = stack_pointer - stack_start + index * sizeof(*value);
    memcpy(value, stack + offset, sizeof(*value));
    return 0;
}
