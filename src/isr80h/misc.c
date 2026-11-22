#include "misc.h"
#include "idt/idt.h"
#include "status.h"
#include "task/process.h"
#include "task/task.h"

struct demo_record
{
    uint32_t id;
    uint8_t value;
    bool present;
};

static struct demo_record demo_records[] = {
    {.id = 10, .present = true},
    {.id = 20, .present = true}
};

void* isr80h_command0_sum(struct interrupt_frame* frame)
{
    struct task* task = task_current();
    uint32_t v1;
    uint32_t v2;
    if (task_get_stack_item(task, 0, &v1) < 0 ||
        task_get_stack_item(task, 1, &v2) < 0)
    {
        return ERROR(-EINVARG);
    }

    return (void*)(v1 + v2);
}

static void* demo_record_request(struct interrupt_frame* frame, uint32_t permission)
{
    struct demo_record* record = 0;
    for (size_t i = 0; i < sizeof(demo_records) / sizeof(demo_records[0]); i++)
    {
        if (demo_records[i].id == frame->ebx)
        {
            record = &demo_records[i];
            break;
        }
    }
    if (!record || !record->present)
    {
        return ERROR(-EINVARG);
    }

    struct process* process = task_current()->process;
    if (!(process->syscall_permissions & permission))
    {
        return ERROR(-EACCES);
    }

    if (permission == PROCESS_PERMISSION_DELETE)
    {
        if (record->id != process->delete_record_id)
        {
            return ERROR(-EACCES);
        }
        record->present = false;
    }

    if (permission == PROCESS_PERMISSION_WRITE)
    {
        record->value++;
    }

    if (permission == PROCESS_PERMISSION_READ)
    {
        // READ returns the record's byte as a nonnegative result.
        return (void*)(uintptr_t)record->value;
    }

    return 0;
}

void* isr80h_command3_read(struct interrupt_frame* frame)
{
    return demo_record_request(frame, PROCESS_PERMISSION_READ);
}

void* isr80h_command4_delete(struct interrupt_frame* frame)
{
    return demo_record_request(frame, PROCESS_PERMISSION_DELETE);
}

void* isr80h_command5_write(struct interrupt_frame* frame)
{
    return demo_record_request(frame, PROCESS_PERMISSION_WRITE);
}
