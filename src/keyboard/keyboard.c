#include "keyboard.h"
#include "status.h"
#include "kernel.h"
#include "task/process.h"
#include "task/task.h"
#include "classic.h"
static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_last = 0;

void keyboard_init()
{
 keyboard_insert(classic_init());
}
int keyboard_insert(struct keyboard* keyboard)
{
    int res = 0;

    if (keyboard->init == 0)
    {
        res = -EINVARG;
        goto out;
    }

    if (keyboard_list_last)
    {
        keyboard_list_last->next = keyboard;
        keyboard_list_last = keyboard;
    }
    else
    {
        keyboard_list_head = keyboard;
        keyboard_list_last = keyboard;
    }

    res = keyboard->init();

out:
    return res;
}
void keyboard_backspace(struct process* process)
{
    struct keyboard_buffer* queue = &process->keyboard;
    if (queue->head == queue->tail)
    {
        return;
    }
    queue->tail = (queue->tail + sizeof(queue->buffer) - 1) % sizeof(queue->buffer);
    queue->buffer[queue->tail] = 0;
}

void keyboard_push(char c)
{
    struct task* task = task_current();
    if (!task)
    {
        return;
    }

    struct keyboard_buffer* queue = &task->process->keyboard;
    int next = (queue->tail + 1) % sizeof(queue->buffer);
    if (next == queue->head)
    {
        return;
    }
    queue->buffer[queue->tail] = c;
    queue->tail = next;
}

char keyboard_pop()
{
    struct task* task = task_current();
    if (!task)
    {
        return 0;
    }

    struct keyboard_buffer* queue = &task->process->keyboard;
    if (queue->head == queue->tail)
    {
        return 0;
    }

    char c = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % sizeof(queue->buffer);
    return c;
}






