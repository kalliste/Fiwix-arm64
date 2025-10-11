# Trap Frame Usage Notes

This document records how subsystems above the low-level assembly stubs assume the current x86 trap frame layout. The goal is to expose every dependency we must revisit when designing an Arm64 exception frame.

## Saved Register Layout

The interrupt/exception stubs in `kernel/core386.S` push general-purpose and segment registers in a fixed order using `pusha` followed by explicit `pushl` instructions for `%ds`, `%es`, `%fs`, and `%gs`. The CPU-provided error code (or a simulated `0`) ends up just above the processor-supplied `EIP`, `CS`, `EFLAGS`, `OLDESP`, and `OLDSS` slots.【F:kernel/core386.S†L17-L112】 The in-kernel representation mirrors this exact ordering via `struct sigcontext`, so any code that receives a `sigcontext` pointer can index registers by field name without additional translation.【F:include/fiwix/sigcontext.h†L11-L29】

The common trap handler receives a full `struct sigcontext` by value, forwards a pointer to the exception-specific routine, and then negates the saved error code to disambiguate restarted system calls. This by-value handoff relies on the structure layout matching the stack image verbatim.【F:kernel/traps.c†L244-L250】

## Signal Delivery Path

The assembly epilogue calls `issig()` and, on a pending signal, passes the current `%esp` to `psig()`. That `%esp` value is the trap frame base, so the signal code expects `struct sigcontext` to be packed exactly like the stack image built by `SAVE_ALL`.【F:kernel/core386.S†L61-L112】 Inside `psig()` the kernel copies the active trap frame into a per-signal slot, adjusts `oldesp` to carve out space for the user-mode trampoline, writes the handler PC into `eip`, and stores arguments in `eax`/`ecx`. These operations all assume `oldesp`, `eip`, and general-purpose registers sit at the same offsets as the push sequence established in assembly.【F:kernel/signal.c†L164-L205】

Signal restarts lean on two additional conventions. First, the page-fault handler checks `sc->oldesp` to determine whether a fault near the user stack should grow the stack VMA, so the trap frame must expose the interrupted user stack pointer.【F:mm/fault.c†L87-L110】 Second, `psig()` interprets positive `sc->err` values as saved syscall numbers when deciding to rewind the `int 0x80` instruction, so the trap handler’s negation of `err` and the position of `eip` in the frame are baked into the restart logic.【F:kernel/traps.c†L244-L250】【F:kernel/signal.c†L236-L242】

The `/proc/<pid>/stat` renderer also depends on this layout: it reads `p->sp` (populated with the trap-frame address during system call entry) as a `struct sigcontext` pointer to expose user `EIP` and `ESP` values. Without a compatible frame organization those fields could not be recovered for procfs output.【F:kernel/syscalls.c†L485-L512】【F:fs/procfs/data.c†L804-L866】

## Scheduler Hooks and Deferred Work

After handling an interrupt or exception, the assembly shim calls `do_sched()` if `need_resched` is set. Because `%esp` still points at the saved trap frame, the scheduler runs with the interrupted context fully preserved and simply returns to the same stack frame when done.【F:kernel/core386.S†L71-L133】【F:kernel/sched.c†L33-L76】 Any Arm64 port must ensure the equivalent scheduler hook can safely run while the exception frame remains active.

The same frame pointer is passed into `do_bh()` for bottom halves and into device IRQ handlers, even though those routines currently ignore the register snapshot. Maintaining a consistent structure keeps that ABI available if future handlers ever inspect register state.【F:kernel/core386.S†L53-L100】【F:kernel/irq.c†L62-L118】

## System Calls and Privileged Helpers

System calls reuse the trap frame in several places. The syscall dispatcher saves the address of the on-stack `sigcontext` in `current->sp` so helpers can locate register state; the fork implementation copies the entire frame to seed the child’s kernel stack and zeroes out the child’s return value by touching `eax`. These behaviors demand that the stack image be byte-for-byte compatible with `struct sigcontext`.【F:kernel/syscalls.c†L485-L512】【F:kernel/syscalls/fork.c†L100-L158】

Individual syscalls also mutate specific slots: `sys_iopl` patches the saved `eflags`, while `sys_sigreturn` restores the frame from a per-signal backup. These direct field accesses tie those helpers to the exact register ordering established in the assembly stubs.【F:kernel/syscalls/iopl.c†L32-L54】【F:kernel/syscalls/sigreturn.c†L17-L28】

---

### Outstanding Questions

* Which Arm64 exception level and SPSR/ELR fields map onto the current `oldesp`/`oldss` combination so signal delivery can keep reporting user stacks?
* How will we arrange for syscall restarts when the Arm64 exception entry path does not automatically push an error code slot above the saved PC?
