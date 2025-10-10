# Architectural Data Structures and ABI Expectations (x86 Baseline)

## Segmentation and Descriptor Tables
- `include/fiwix/segments.h` hard-codes selector values for kernel and user code/data, the TSS slot, and assumes privilege level transitions are controlled through x86 ring bits (`USER_PL`).【F:include/fiwix/segments.h†L13-L33】
- The file fixes the table sizes (`NR_GDT_ENTRIES = 6`, `NR_IDT_ENTRIES = 256`) and encodes x86-specific descriptor flag constants (`SD_32INTRGATE`, `SD_OPSIZE32`, etc.), implying the Arm64 port must replace the descriptor format entirely with EL/exception-vector metadata.【F:include/fiwix/segments.h†L27-L55】
- Descriptor layouts are represented by packed bitfields (`struct seg_desc`, `struct gate_desc`) tied to the Intel descriptor bit ordering, highlighting that all code using these structures (e.g., GDT/IDT initialisation) will need Arm64-specific equivalents.【F:include/fiwix/segments.h†L57-L74】

## Task State Segment and Process Save Areas
- Every process embeds an `i386tss` structure with x86 stack pointers for three privilege levels, CR3, segment registers, and an 8 KB I/O permission bitmap; this TSS design is central to context switching and stack switching expectations.【F:include/fiwix/process.h†L63-L114】
- The scheduler stores the TSS inside each `struct proc`, reinforcing that Arm64 context-switch bookkeeping must replace the TSS with EL1 register state and per-thread stack pointers rather than hardware task gates.【F:include/fiwix/process.h†L116-L179】

## Trap Frames and Signal Context
- Signal delivery and exception handlers rely on `struct sigcontext`, which mirrors the push order established by the assembly save macros (segment registers, general-purpose registers, EIP/CS/EFLAGS, and optional ring-transition stack slots).【F:include/fiwix/sigcontext.h†L11-L28】【F:kernel/core386.S†L16-L71】
- The `SAVE_ALL` macro in `kernel/core386.S` assumes `pusha` semantics and that the CPU pushes an error code and x86-specific state before invoking `trap_handler`, indicating we must define a new trap frame layout for Arm64’s vector entries.【F:kernel/core386.S†L16-L66】

## Interrupt and Exception Return Flow
- Exception and IRQ stubs finish with `iret` after restoring segment registers and general-purpose state, and they test the saved CS to detect privilege changes (`CHECK_IF_NESTED_INTERRUPT`). An Arm64 design will need to map this logic to SPSR/ELR handling and vector return sequences.【F:kernel/core386.S†L36-L98】

## System Call ABI
- User space invokes syscalls by loading arguments into `eax`/`ebx`/`ecx`/`edx` and executing `int 0x80` via the `USER_SYSCALL` macro; TCC support shows register allocation nuances that must be documented when designing an Arm64 `svc` wrapper.【F:include/fiwix/asm.h†L87-L133】
- The kernel entry point `syscall` saves the call number, pushes up to six arguments in a specific order, and later writes the return value back to the saved `EAX` slot before issuing `iret`. Arm64 needs an equivalent frame layout and return path, likely using `x0`–`x5` and `eret`.【F:kernel/core386.S†L218-L269】

## Context Switching and Control Registers
- `do_switch` stores the outgoing task’s stack pointer and instruction pointer into the TSS, loads the incoming task’s CR3, performs an `ltr`, and depends on x86 CR3 semantics for address space switching. This exposes the need to define Arm64’s TTBR updates and per-thread stacks in software.【F:kernel/core386.S†L270-L299】
- Low-level helpers in `include/fiwix/asm.h` manipulate control registers (`GET_CR2`, `activate_kpage_dir`, `load_tr`) and rely on CLI/STI for interrupt masking; Arm64 replacements must cover MMU register access and interrupt masking primitives such as `msr daifset`/`daifclr`.【F:include/fiwix/asm.h†L65-L106】

## Research Questions Raised for Arm64
- Which Arm64 data structures (vector tables, exception stack frames, per-thread SPSR copies) will replace the GDT/IDT/TSS trio noted above?
- How should we model the signal/trap context so that user space receives register dumps equivalent to the current `sigcontext`? Arm64’s `rt_sigframe` equivalent likely needs a new layout.
- What is the precise calling convention for Arm64 syscalls (`x8` number, `svc 0`) and how will the kernel capture/restore registers without hardware task state support?
- How do Arm64’s translation table base registers and context switch requirements map onto the existing expectations that each process owns a hardware-managed TSS and CR3 value?
