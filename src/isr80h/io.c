#include "io.h"
#include "task/task.h"
#include "kernel.h"
#include "keyboard/keyboard.h"

void* isr80h_command1_print(
    struct interrupt_frame* frame
)
{
    struct task* task = task_current();
    uint32_t user_buffer;
    int res = task_get_stack_item(task, 0, &user_buffer);
    if (res < 0)
    {
        return ERROR(res);
    }

    char buf[1024];
    res = copy_string_from_task(task, (void*)user_buffer, buf, sizeof(buf));
    if (res < 0)
    {
        return ERROR(res);
    }

    print(buf);

    return 0;
}

void* isr80h_command2_getkey(struct interrupt_frame* frame)
{
    return (void*)(uintptr_t)(unsigned char)keyboard_pop();
}
