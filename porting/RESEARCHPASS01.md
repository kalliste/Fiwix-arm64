# Research Pass 01 – Sections 1 and 4 Status

## 1. Map Architecture-Specific Assumptions in the Current Tree

### Coverage so far
- Boot and early bring-up flows in `kernel/boot.S` are fully catalogued, covering Multiboot expectations, temporary GDT setup, stack establishment, and paging enablement details in `porting/X86INSTRUCTIONS.md` so the Arm64 plan can target every x86-only instruction and register dependency.【F:porting/X86INSTRUCTIONS.md†L1-L31】
- Linker and memory map constraints, including the 32-bit `elf32-i386` output, fixed physical load address, virtual offset, and coupled page-table layout, have been summarized in `porting/LINKERMEMORYPAGING.md` with cross-references into `fiwix.ld`, `include/fiwix/linker.h`, and `mm/memory.c`.【F:porting/LINKERMEMORYPAGING.md†L3-L13】
- Architectural data structures and ABI contracts (segment descriptors, TSS, trap frames, syscall calling conventions, context-switch mechanics) are inventoried in `porting/STRUCTSANDABI.md`, highlighting every x86-specific structure and helper that must be redesigned for Arm64.【F:porting/STRUCTSANDABI.md†L3-L25】
- Legacy PC hardware dependencies spanning PIC/PIT/CMOS/PS2/DMA/IDE/PCI/serial/VGA are traced in `porting/PCHARDWARE.md`, giving a subsystem-by-subsystem map of ISA I/O assumptions to replace on Arm64.【F:porting/PCHARDWARE.md†L3-L113】

### Outstanding items
- Need to document how higher-level kernel services (e.g., signal delivery, scheduler hooks) assume the current trap frame layout and to draft Arm64 equivalents, extending the questions already raised in `porting/INTERUPTSEXCEPTIONS.md` and `porting/STRUCTSANDABI.md`.【F:porting/INTERUPTSEXCEPTIONS.md†L15-L19】【F:porting/STRUCTSANDABI.md†L12-L25】
- Still have to decide how Multiboot hand-off requirements translate to Arm64 firmware/boot protocols; existing notes call out register preservation but do not yet evaluate UEFI/DT/PSCI options or alternate bootloaders.【F:porting/X86INSTRUCTIONS.md†L5-L24】
- Hardware dependency research flagged ISA-centric devices, but we have not yet prioritized Arm64 replacements (e.g., specific GIC version, generic timer, PL011 UART) or outlined platform-specific enablement steps beyond the high-level to-do list.【F:porting/PCHARDWARE.md†L107-L113】

### Follow-on investigations discovered
- The inline-assembly survey surfaced `kernel/kexec.c` as a major x86 trampoline that will require a dedicated Arm64 design, suggesting a new research thread on hand-off semantics beyond the normal boot path.【F:porting/X86INSTRUCTIONS.md†L33-L38】
- Questions about syscall ABI translation and preserving scheduler/signal sequencing on `eret` return were recorded and should drive deeper dives into Arm64 exception level design.【F:porting/INTERUPTSEXCEPTIONS.md†L15-L19】

## 4. Plan Codebase Audits Supporting the Research

### Coverage so far
- Automated searches catalogued inline `__asm__` blocks, flagging interrupt control, system-call stubs, and kexec trampolines that are tightly bound to x86 opcodes, as summarized in `porting/X86INSTRUCTIONS.md`.【F:porting/X86INSTRUCTIONS.md†L33-L38】
- Hard-coded paging, selector, and descriptor constants (e.g., `KERNEL_CS`, `PAGE_OFFSET`, GDT/IDT sizing) are collected in `porting/HARDCODED.md`, giving a checklist of macros and headers that need Arm64-safe replacements.【F:porting/HARDCODED.md†L3-L13】
- Dependency mapping spans both interrupt/exception flow analysis and peripheral inventories, tying low-level stubs to higher-level handlers in `porting/INTERUPTSEXCEPTIONS.md` and enumerating ISA device usage in `porting/PCHARDWARE.md` to show which subsystems depend on x86 hardware behavior.【F:porting/INTERUPTSEXCEPTIONS.md†L7-L13】【F:porting/PCHARDWARE.md†L3-L113】
- Architecture-neutral subsystems were surveyed in `porting/ARCHNEUTRAL.md`, helping to bound the audit scope by listing directories that can transition unchanged once low-level hooks are ported.【F:porting/ARCHNEUTRAL.md†L1-L41】

### Outstanding items
- Need a deeper call-graph style mapping from trap/IRQ stubs into scheduler, signal, and bottom-half code to validate that every assumption is captured before designing Arm64 exception vectors.【F:porting/INTERUPTSEXCEPTIONS.md†L7-L13】
- Still pending: an audit of architecture-specific build system logic (e.g., make targets, config headers) to complement the source-level scans, so that toolchain and configuration assumptions are captured alongside code dependencies.【F:porting/HARDCODED.md†L8-L13】
- Must plan how to translate the identified ISA-device touchpoints into a concrete Arm64 platform strategy (selecting target SoC/IP blocks and defining shims), which extends beyond the current dependency catalog.【F:porting/PCHARDWARE.md†L107-L113】

### Follow-on investigations discovered
- The architecture-neutral survey points to reusable subsystems; pairing it with dependency data suggests creating a tracker that links neutral code to the new low-level interfaces required for Arm64, preventing regressions as we modify shared APIs.【F:porting/ARCHNEUTRAL.md†L22-L41】
- Research notes now highlight the need to design replacement port-I/O and interrupt helper layers that emulate existing kernel APIs while targeting memory-mapped Arm64 peripherals, motivating a future audit focused on API surface compatibility.【F:porting/PCHARDWARE.md†L94-L113】
