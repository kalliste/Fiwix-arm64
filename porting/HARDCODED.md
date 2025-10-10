# Hard-Coded x86 Paging and Segmentation Constants

## Segment selectors and descriptor definitions
- `include/fiwix/segments.h` hard-codes the selector values for the kernel and user segments (`KERNEL_CS` = 0x08, `KERNEL_DS` = 0x10, `USER_CS` = 0x18, `USER_DS` = 0x20) plus the TSS selector at 0x28, all of which reflect the current x86 GDT layout.【F:include/fiwix/segments.h†L13-L18】
- The same header fixes descriptor counts (`NR_GDT_ENTRIES` = 6, `NR_IDT_ENTRIES` = 256) and encodes 32-bit descriptor flag patterns (`SD_OPSIZE32`, `SD_PAGE4KB`, `SD_32INTRGATE`, etc.) that assume x86 segmentation semantics.【F:include/fiwix/segments.h†L26-L44】
- x86-specific page flag bit assignments (`PAGE_PRESENT`, `PAGE_RW`, `PAGE_USER`, `PAGE_NOALLOC`) are also baked into `include/fiwix/segments.h`, tying the memory management layer to the i386 page-table format.【F:include/fiwix/segments.h†L20-L24】

## Kernel linear address layout and GDT mirroring
- `include/fiwix/linker.h` defines `PAGE_OFFSET` as either 0xC0000000 or 0x80000000 (depending on `CONFIG_VM_SPLIT22`), `KERNEL_ADDR` as 1 MB, a 4 KB `KERNEL_STACK`, and a `GDT_BASE` derived from `0xFFFFFFFF - (PAGE_OFFSET - 1)`, all of which are tuned for a 32-bit address space and segmentation model.【F:include/fiwix/linker.h†L13-L21】
- The boot code in `kernel/boot.S` relies on these selectors and offsets, performing far jumps with `$KERNEL_CS` and loading `%ss` with `$KERNEL_DS` while setting up the initial stack at `PAGE_OFFSET + 0x10000`, showing how deeply the startup path assumes the current linear layout.【F:kernel/boot.S†L93-L115】
- Memory initialization mirrors page tables into the kernel mapping by adding `GDT_BASE` to physical addresses, reinforcing the baked-in dependency on the i386 high-memory trick (`mm/memory.c`).【F:mm/memory.c†L92-L123】

These constants and their call sites must be replaced with Arm64-appropriate translation table, vector base, and privilege level definitions during the port.
