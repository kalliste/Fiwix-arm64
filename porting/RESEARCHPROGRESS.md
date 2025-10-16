# Porting Research Progress

## Entry 1
- Reviewed `kernel/boot.S` to map the Multiboot v1 entry flow, temporary GDT usage, paging enablement, and early stack setup.
- Captured x86-specific instructions and assumptions in `porting/X86INSTRUCTIONS.md` for future Arm64 planning.

## Entry 2
- Documented the existing linker script, memory map, and paging setup constraints in `porting/LINKERMEMORYPAGING.md` to inform Arm64 planning.

## Entry 3
- Catalogued x86 architectural data structures (GDT/IDT descriptors, TSS layout, trap frames, syscall ABI) and documented Arm64 replacement questions in `porting/STRUCTSANDABI.md`.

## Entry 4
- Studied `kernel/core386.S`, `kernel/pic.c`, and supporting headers to document how x86 interrupt/exception stubs, PIC flows, and syscall paths operate today.
- Recorded findings and Arm64 follow-up questions in `porting/INTERUPTSEXCEPTIONS.md` for future planning.

## Entry 5
- Traced legacy PC hardware controller dependencies (PIC, PIT, CMOS, PS/2, DMA, storage, console) to understand which subsystems rely on ISA-style I/O and chipset features.
- Captured the findings in `porting/PCHARDWARE.md` to guide Arm64 replacements for timers, interrupt controllers, storage buses, and console I/O.

## Entry 6
- Used `rg` to inventory all inline `__asm__` blocks across the tree, capturing helper macros in `include/fiwix/asm.h` and the kexec trampoline logic in `kernel/kexec.c`.
- Summarized the porting implications of these x86-specific instructions in `porting/X86INSTRUCTIONS.md` to inform the Arm64 design for interrupt masking, syscall invocation, control register manipulation, and hand-off mechanics.

## Entry 7
- Searched the tree for hard-coded paging, segment selector, and descriptor constants to gauge x86 assumptions that will block an Arm64 port.
- Captured the key findings (selector values, GDT/IDT sizing, page flag bits, and PAGE_OFFSET/GDT_BASE layout coupling) in `porting/HARDCODED.md` for follow-up design work.

## Entry 8
- Surveyed the source tree to catalog subsystems that operate purely on VFS,
  scheduler, or data-structure logic without x86 hardware dependencies.
- Recorded the architecture-neutral directory list and representative examples
  in `porting/ARCHNEUTRAL.md` to scope what can transfer unchanged to Arm64.

## Entry 9
- Compiled a status report for Sections 1 and 4 of the porting research plan in `porting/RESEARCHPASS01.md`, summarizing completed audits, remaining tasks, and follow-on investigations.
- Reviewed existing research notes to identify outstanding questions on trap frame design, boot protocol translation, and Arm64 hardware targets for upcoming deep-dive work.

## Entry 10
- Investigated how the x86 trap frame is consumed after interrupt/exception entry, tracing `SAVE_ALL` through signal delivery, scheduler hooks, and `/proc` reporting.
- Documented the findings and open Arm64 design questions in `porting/TRAPFRAME.md` to guide the eventual exception-frame redesign.

## Entry 11
- Mapped the full path from x86 trap and IRQ stubs through bottom-half dispatch,
  signal delivery, and scheduler hooks, capturing the call graph and design
  dependencies in `porting/IRQSIGNAL.md` for Arm64 exception-vector planning.【F:porting/IRQSIGNAL.md†L1-L84】
- Reviewed timer, signal, and scheduler sources to confirm how `need_resched`
  and `sigpending` are raised during interrupt handling so the Arm64 port can
  preserve identical return-to-user semantics.【F:kernel/timer.c†L178-L336】【F:kernel/signal.c†L31-L226】【F:kernel/sched.c†L20-L82】
