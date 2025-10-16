# Trap, IRQ, Signal, and Scheduler Flow Map

This note maps the x86 interrupt/exception stubs to the higher-level signal
and scheduling code paths so we can design Arm64 exception vectors that
preserve every assumption.

## 1. Stub Epilogue Responsibilities

All exception and IRQ stubs built in `kernel/core386.S` save registers, call a
C handler, then execute the same epilogue before `iret`:

1. `SAVE_ALL` pushes general-purpose registers and segment selectors so the C
   side can treat the stack as a `struct sigcontext`.【F:kernel/core386.S†L36-L104】
2. After the handler returns, `BOTTOM_HALVES` enables interrupts and runs
   `do_bh()` to service deferred work items registered in the global bottom
   half list.【F:kernel/core386.S†L53-L99】【F:kernel/irq.c†L123-L138】
3. `CHECK_IF_NESTED_INTERRUPT` skips signal/scheduler checks when the saved CS
   equals `KERNEL_CS`, ensuring kernel-mode reentry paths avoid reentrancy into
   user coordination logic.【F:kernel/core386.S†L57-L100】
4. For user-mode returns, `CHECK_IF_SIGNALS` calls `issig()` and, if a signal is
   pending, invokes `psig()` with the saved `sigcontext` so signal delivery is
   processed before scheduling.【F:kernel/core386.S†L61-L100】【F:kernel/signal.c†L122-L243】
5. `CHECK_IF_NEED_SCHEDULE` consults the global `need_resched` flag and calls
   `do_sched()` when rescheduling is required prior to restoring registers and
   executing `iret`.【F:kernel/core386.S†L71-L112】【F:kernel/sched.c†L20-L82】

## 2. Exception Path Call Graph

```
exceptN stub → trap_handler(trap,#sc)
             → traps_table[trap].handler()
                 ↳ do_* functions issue send_sig()/panic
return to stub  → bottom halves → signal check → scheduler → iret
```

- Each stub calls `trap_handler()`, which dispatches through `traps_table[]` to
  the specific `do_*` routine for the trap number.【F:kernel/core386.S†L89-L146】【F:kernel/traps.c†L244-L250】
- The `do_*` routines primarily diagnose the fault and translate it into the
  POSIX signal that should be delivered to the current task via
  `send_sig()`.【F:kernel/traps.c†L34-L242】
- After the C handler returns, the common epilogue drives bottom halves, signal
  delivery, and scheduling as described above.

## 3. Hardware IRQ Path Call Graph

```
irqN stub → irq_handler(num,#sc)
          → disable_irq/ack_pic_irq
          → walk struct interrupt list, calling handler(num,&sc)
return to stub → bottom halves → signal check → scheduler → iret
```

- IRQ stubs invoke `irq_handler()`, which disables the source, acknowledges the
  PIC, and walks the linked list of registered `struct interrupt` handlers for
  that vector, incrementing per-interrupt statistics as it goes.【F:kernel/core386.S†L161-L189】【F:kernel/irq.c†L90-L115】【F:include/fiwix/irq.h†L15-L38】
- Device handlers set `BH_ACTIVE` flags on bottom halves for deferred work.
  When the stub epilogue calls `do_bh()`, each active bottom half clears its
  flag and runs with interrupts enabled, using the saved `sigcontext` if
  needed.【F:kernel/irq.c†L123-L138】
- Unknown or spurious IRQs reach `unknown_irq_handler()`/`spurious_interrupt()`,
  leaving the rest of the epilogue flow unchanged.【F:kernel/irq.c†L96-L121】

## 4. Bottom Half Producers and Effects

- The system timer registers two bottom halves: one for periodic accounting and
  one for callout execution. Initialization wires them into the global list and
  hooks the timer interrupt handler.【F:kernel/timer.c†L178-L186】【F:kernel/timer.c†L442-L466】
- The timer IRQ top half merely marks `timer_bh` active; the bottom half then
  updates CPU usage statistics, decrements per-process timers, wakes sleeping
  tasks, schedules POSIX interval timers, and drives callouts. It also sets
  `need_resched` when a task exhausts its quantum, triggering a scheduler run on
  exit from the interrupt.【F:kernel/timer.c†L178-L336】
- Other drivers (e.g., keyboard, serial) register their own bottom halves via
  `add_bh()`, reusing the same epilogue pipeline when they flag work items.

## 5. Signal Delivery Handshake

1. Kernel subsystems translate events into signals with `send_sig()`, which sets
   `sigpending`, wakes targets that are not blocking the signal, and marks
   `need_resched` when SIGCONT/SIGKILL-type transitions require a different
   runnable task.【F:kernel/signal.c†L31-L119】
2. On return to user mode, `issig()` filters pending bits against the blocked
   mask. `psig()` consumes one signal at a time, either vectoring to a user
   handler (saving/restoring the `sigcontext`) or performing default actions
   such as stopping the task, reparent notifications, or terminating the
   process.【F:kernel/signal.c†L122-L243】
3. Signals that stop or resume tasks also manipulate `need_resched`, so the
   stub epilogue will call `do_sched()` before returning to user space when the
   signal changes run state.【F:kernel/signal.c†L202-L226】

## 6. Scheduler Trigger Points

- `need_resched` is a global flag cleared inside `do_sched()` after the next
  runnable task is selected via the round-robin algorithm.【F:kernel/sched.c†L20-L82】
- The flag is raised from timer bottom halves when a task’s quantum expires, by
  `send_sig()`/`psig()` when signals change run states, and by other subsystems
  that need a scheduling decision. These paths rely on the stub epilogue to
  notice the flag and call `do_sched()` before returning to user mode, which we
  must replicate in the Arm64 vector return logic.【F:kernel/timer.c†L332-L335】【F:kernel/signal.c†L48-L50】【F:kernel/signal.c†L211-L225】

This mapping confirms that the x86 entry/exit stubs provide a tightly coupled
sequence—deferred work, signal delivery, then scheduling—that Arm64 vectors must
emulate so higher-level semantics remain unchanged.
