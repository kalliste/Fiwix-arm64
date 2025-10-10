# x86 Boot Path Overview

This document summarizes the existing x86-specific assumptions in `kernel/boot.S` so we can plan the Arm64 port.

## Multiboot v1 Entry Expectations
- `_start` is declared as the Multiboot entry point after the bootloader jumps to the kernel in 32-bit protected mode.
- Execution begins with interrupts disabled via `cli`, relying on the x86 interrupt flag.
- The Multiboot magic (`%eax`) and info structure pointer (`%ebx`) are preserved by avoiding any destructive use of these registers prior to saving them.
- The implementation assumes the Multiboot header layout defined by `<fiwix/multiboot1.h>` and uses `MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO` flags.

## Temporary GDT Setup
- A three-entry temporary GDT is defined in the `.setup` section containing: null descriptor, kernel code segment, and kernel data segment.
- `_start` executes `lgdt` with `tmp_gdtr` to load this table, an x86 instruction that assumes segment-based memory management.
- Segment registers `%ds`, `%es`, `%fs`, `%gs`, and `%ss` are set to the kernel data selector, and a far `ljmp` reloads `%cs` with the kernel code selector. All of these steps rely on x86 segmentation semantics.

## Early Stack Establishment
- `setup_kernel` immediately sets `%esp` to `PAGE_OFFSET + 0x10000`, creating a temporary stack in the kernel's virtual address space.
- Stack operations like `pushl`/`popl` are used to save and restore Multiboot registers and call arguments, assuming the standard x86 downward-growing stack.

## Paging Enablement Flow
- `setup_tmp_pgdir` is called and its return value is moved into `%cr3` to load the temporary page directory, using the x86-specific control register.
- `%cr0` is read, masked to keep GRUB's existing PE/ET bits, and then ORed with x86 paging-related flags (`CR0_PG`, `CR0_AM`, `CR0_WP`, `CR0_NE`, `CR0_MP`).
- The final `movl %eax, %cr0` enables paging, relying on the x86 paging model and control register layout.

## Other x86-Specific Instructions and Assumptions
- The code uses 32-bit general-purpose registers (`%eax`, `%ebx`, `%ecx`, `%esi`, `%edi`) and assumes their availability and calling conventions.
- `call`/`ret` flow follows the x86 32-bit ABI with implicit stack-based argument passing.
- `popf` is used to restore a cleared EFLAGS state, depending on the x86 flag register format.
- `jmp cpu_idle` assumes `cpu_idle` is an x86 label reachable by direct jump.

These architectural dependencies will need alternatives or emulation when designing the Arm64 boot sequence.

## Inline Assembly Call Sites Identified with `rg`
- `include/fiwix/asm.h` defines a set of helper macros that emit raw x86 instructions. These include interrupt flag control (`cli`, `sti`), idle/serialization primitives (`nop`, `hlt`), and privileged register moves to access `%cr2`/`%esp`. Porting to Arm64 will require equivalents for interrupt masking, low-power waits, and stack pointer management that respect the Arm64 privilege model.
- The same header implements `SAVE_FLAGS`/`RESTORE_FLAGS` wrappers around `pushfl`/`popfl`, meaning our Arm64 plan must determine how to capture and restore PSTATE/DAIF bits without relying on stack-based flag instructions.
- The `USER_SYSCALL` macro emits an `int $0x80` software interrupt. A portable Arm64 design needs to replace this with the appropriate `svc` invocation and calling convention, while also handling the TinyCC-specific register shuffling that the current macro accommodates.
- `kernel/kexec.c` contains extensive inline assembly for the transition path when jumping into a replacement kernel. The sequences manipulate descriptor tables with `lidt`/`lgdt`, program control registers (`mov %cr0`, `mov %cr3`), reload segment registers, and perform a far `ljmp` through the stack. We must map each of these to Arm64's exception level transitions, MMU configuration, and branch mechanics when recreating the kexec trampoline.
- Within the same file, temporary register setups move Multiboot magic values into `%eax`/`%ebx` prior to the jump. Arm64 support will need an alternative hand-off strategy for boot protocol registers (likely `x0`/`x1`) and a compatibility shim if Multiboot handoff must be preserved.
