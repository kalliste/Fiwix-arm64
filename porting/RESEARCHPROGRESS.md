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
