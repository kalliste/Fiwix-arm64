# Porting Research Plan for Arm64

This document lists the investigation and research tasks we need to complete before drafting a detailed implementation plan to port the Fiwix kernel from its current 32-bit x86 focus to the Arm64 architecture. The goal is to build a knowledge base that covers internal code assumptions, build and boot flows, hardware dependencies, and external references.

## 1. Map Architecture-Specific Assumptions in the Current Tree

1. **Catalog boot and early initialization code**
   - Review `kernel/boot.S` to document the existing Multiboot v1 entry path, temporary GDT setup, paging enablement, and early stack usage so we can identify every x86-only instruction and expectation.【F:kernel/boot.S†L13-L145】
   - Inspect `kernel/core386.S` for the interrupt and exception stubs that depend on the x86 register save layout, `iret`, and PIC-style interrupt flow.【F:kernel/core386.S†L36-L146】
2. **Summarize linker, memory layout, and paging constraints**
   - Note that the linker script is hard-coded for `elf32-i386` output, a 1 MB physical load address, and the existing virtual memory offset; record all of these as assumptions to revisit for Arm64.【F:fiwix.ld†L10-L51】
   - Cross-reference the documented i386 kernel memory map (`docs/kmem_layout.txt`) to enumerate the implicit layout guarantees (page tables, stacks, initrd placement) that will change on Arm64.【F:docs/kmem_layout.txt†L1-L40】
3. **Identify architectural data structures and ABI expectations**
   - Analyze segment descriptor definitions, IDT/GDT sizing, and descriptor formats used throughout the code base to understand which parts must be replaced by Arm64-specific exception level and vector handling.【F:include/fiwix/segments.h†L13-L79】
   - Review inline assembly helpers and CPU feature probes in `include/fiwix/asm.h` to list all opcodes and control registers that are x86-specific (e.g., `cli`, `int 0x80`, CRx access) and will require new Arm64 equivalents.【F:include/fiwix/asm.h†L11-L142】
4. **Trace hardware controller dependencies**
   - Document interactions with legacy PC hardware such as the 8259 PIC, CMOS, PIT, and ISA I/O ports (e.g., `kernel/pic.c`) to assess which subsystems need Arm64/interrupt-controller alternatives.【F:kernel/pic.c†L15-L104】
   - Survey the `drivers/`, `lib/`, and `net/` trees to flag any additional inline assembly, port I/O, or BIOS expectations that would not exist on Arm64 platforms.

## 2. Inventory Build, Toolchain, and Configuration Requirements

1. **Current build process**
   - Record the existing `make` workflow and configuration toggles noted in the README so we understand the baseline build inputs and tunables before introducing a cross-compilation toolchain.【F:README.md†L33-L55】【F:include/fiwix/config.h†L11-L66】
2. **Toolchain changes to research**
   - Determine which cross-compiler (e.g., `aarch64-elf-gcc`) and binutils we must use, how to produce an Arm64-compatible ELF image, and whether we need a different linker script format (e.g., `OUTPUT_ARCH("aarch64")).`
   - Investigate emulator and hardware targets (QEMU `virt`, Raspberry Pi, etc.) to decide which platform we will treat as the reference Arm64 machine for initial bring-up.

## 3. Formulate Questions for External Research

1. **Boot and firmware environment**
   - Research Arm64 boot protocols relevant to hobby OS development (UEFI, PSCI, device tree, spin tables) and map them against the kernel’s current Multiboot v1 expectations.【F:kernel/boot.S†L63-L145】
   - Identify resources on constructing Arm64 exception vectors, translation tables, and EL1 entry points analogous to the x86 GDT/IDT setup currently employed.【F:include/fiwix/segments.h†L13-L79】【F:include/fiwix/asm.h†L87-L142】
2. **Memory management architecture**
   - Gather documentation on Arm64 paging structures (TTBRs, MAIR, TCR) and translation granules so we can plan how to adapt or rewrite the paging subsystem that now assumes x86 CR3/CR0 semantics.【F:kernel/boot.S†L121-L133】【F:fiwix.ld†L10-L44】
   - Review Arm64 cache/TLB maintenance requirements to replace usages of x86-specific instructions like `invalidate_tlb` and `get_rdtsc`.【F:include/fiwix/asm.h†L87-L151】
3. **Interrupt and exception handling**
   - Study Arm GIC architecture and exception vector table design to replace dependencies on the 8259 PIC and `int 0x80` syscall interface.【F:kernel/core386.S†L36-L114】【F:kernel/pic.c†L15-L104】【F:include/fiwix/asm.h†L65-L142】
   - Research Arm64 approaches for delivering synchronous exceptions, IRQs, and FIQs to ensure the scheduler and signal code paths can be reworked from their current x86 stack frame expectations.【F:kernel/core386.S†L21-L112】
4. **System call ABI and user-space interface**
   - Identify Arm64 calling conventions and syscall mechanisms (e.g., `svc`) to evaluate how to translate the `USER_SYSCALL` macro and trap handlers used today.【F:include/fiwix/asm.h†L125-L141】
   - Catalog existing user-space ABIs that rely on 32-bit structures or packing so we can determine compatibility implications when switching to 64-bit Arm.

## 4. Plan Codebase Audits Supporting the Research

1. **Automated searches**
   - Use `rg` to locate inline assembly blocks (`__asm__`) and port I/O helpers across the tree, creating a spreadsheet of sites that require Arm64 equivalents.
   - Scan for hard-coded constants tied to x86 paging, descriptor tables, or segment selectors (e.g., `KERNEL_CS`, `PAGE_OFFSET`) to quantify refactoring scope.【F:include/fiwix/segments.h†L13-L33】【F:kernel/boot.S†L100-L145】
2. **Dependency mapping**
   - Trace how low-level routines (interrupt handlers, context switches, system calls) interact with higher-level subsystems, and document assumptions about register save areas and stack layouts originating in `core386.S` so we can match them to Arm64 ABI expectations.【F:kernel/core386.S†L21-L114】
   - Identify which subsystems are architecture-neutral and can remain untouched (e.g., filesystem code in `fs/`), helping to bound the porting effort once the research plan is complete.

## 5. Deliverables from the Research Phase

1. **Annotated architecture inventory** summarizing every x86-specific dependency, grouped by subsystem, with references to source files and functions.【F:kernel/core386.S†L36-L146】【F:kernel/pic.c†L15-L104】
2. **External research digest** compiling links and notes on Arm64 boot, MMU, interrupt, and syscall mechanisms aligned with the questions outlined above.
3. **Preliminary feasibility memo** evaluating whether existing design choices (e.g., reliance on Multiboot, 32-bit assumptions) can map directly to Arm64 or require substantial redesign, using insights gathered from both the repository audit and external resources.【F:fiwix.ld†L10-L44】【F:docs/kmem_layout.txt†L1-L40】

Completing these research steps will equip us with the necessary information to draft a realistic and scoped implementation roadmap for the Arm64 port.
