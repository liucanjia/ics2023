#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

extern void do_syscall(Context *c);

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 0xb: 
        if (c->GPR1 == -1) {  // event_yield
          ev.event = EVENT_YIELD; 
          c = user_handler(ev, c);
        } else if (c->GPR1 >= 0) {  // event_syscall
          ev.event = EVENT_SYSCALL;
          do_syscall(c);
        }
        c->mepc += 4;
        break;
      default: ev.event = EVENT_ERROR; break;
    }
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
