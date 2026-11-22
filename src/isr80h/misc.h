#ifndef ISR80H_MISC_H
#define ISR80H_MISC_H

struct interrupt_frame;

void* isr80h_command0_sum(
    struct interrupt_frame* frame
);
void* isr80h_command3_read(struct interrupt_frame* frame);
void* isr80h_command4_delete(struct interrupt_frame* frame);
void* isr80h_command5_write(struct interrupt_frame* frame);

#endif
