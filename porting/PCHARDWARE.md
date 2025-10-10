# PC Hardware Dependency Trace

This document inventories the legacy PC hardware controllers that the current
Fiwix kernel expects to find on x86 and highlights the files and call sites we
must revisit for an Arm64 port. Each subsection notes how the subsystem is
configured, which port-I/O helpers it uses, and any higher-level code that
relies on the controller.

## 8259 Programmable Interrupt Controller
- `kernel/pic.c` programs the master/slave 8259 pair, remaps IRQ vectors, and
  gates interrupts through the PIC-specific `enable_irq`, `disable_irq`, and
  EOI paths, all driven via `outport_b`/`inport_b` port cycles.【F:kernel/pic.c†L15-L104】
- `include/fiwix/pic.h` hard-codes the PIC I/O base addresses and control word
  constants, tying IRQ delivery to the legacy ISA interrupt fabric.【F:include/fiwix/pic.h†L11-L32】

## 8253/8254 Programmable Interval Timer and PC Speaker Latch
- `kernel/pit.c` configures PIT channels 0 and 2, toggles the PC speaker, and
  reads counter values using direct port access to 0x40–0x43 and the PS/2 system
  control port at 0x61.【F:kernel/pit.c†L11-L39】
- `include/fiwix/pit.h` enumerates the PIT channels, mode bits, and PS/2 system
  control register semantics that the kernel assumes exist.【F:include/fiwix/pit.h†L11-L49】
- CPU speed calibration in `kernel/cpu.c` pulses channel 2 and polls 0x61 to
  time the TSC, so the early CPU init path depends on the PIT/PS2 combo as
  currently wired.【F:kernel/cpu.c†L46-L65】

## CMOS Real-Time Clock and NVRAM
- `kernel/cmos.c` serializes accesses to CMOS index/data ports (0x70/0x71) and
  converts between BCD and binary date formats, assuming the IBM PC RTC layout.【F:kernel/cmos.c†L11-L56】
- `include/fiwix/cmos.h` defines the RTC register map (time/date fields,
  diagnostic/status bits, floppy/IDE descriptors, century byte).【F:include/fiwix/cmos.h†L11-L56】
- Boot-time timekeeping pulls the CMOS date into the kernel clock, while
  `set_system_time()` pushes updates back to the RTC.【F:kernel/timer.c†L361-L429】
- `/proc/rtc` exposes RTC state directly by re-reading CMOS registers in the
  procfs handler.【F:fs/procfs/data.c†L372-L403】

## i8042 PS/2 Controller and Keyboard/Mouse Path
- The low-level PS/2 driver performs controller self-tests, channel enablement,
  and byte I/O through ports 0x60/0x64 plus the 0x92 “system control A” latch to
  trigger resets.【F:drivers/char/ps2.c†L33-L200】
- `include/fiwix/ps2.h` embeds the PS/2 command set and status bit masks, so the
  stack presumes a PC-compatible i8042.【F:include/fiwix/ps2.h†L11-L60】
- Higher-level keyboard logic issues device commands (`identify`, `set scan
  code`, LED updates) via the PS/2 helper routines, meaning input bring-up is
  tightly coupled to the controller protocol.【F:drivers/char/keyboard.c†L178-L218】

## ISA DMA Controller (8237)
- `drivers/block/dma.c` programs the 8237 DMA channels, managing mask, mode,
  base, count, and page registers through fixed port tables; DMA setup is used
  by floppy and ISA devices.【F:drivers/block/dma.c†L12-L95】
- `include/fiwix/dma.h` documents the DMA mode bits and exposes registration
  helpers for devices that grab a channel.【F:include/fiwix/dma.h†L11-L31】

## Floppy Disk Controller (NEC 765 compatible)
- The floppy driver drives FDC ports 0x3F0–0x3F7, relies on CMOS drive-type
  bytes, and integrates with ISA DMA channel 2 during initialization.【F:drivers/block/floppy.c†L748-L830】
- `include/fiwix/floppy.h` codifies the FDC register map, IRQ/DMA assignments,
  and command opcodes assumed by the block driver.【F:include/fiwix/floppy.h†L14-L101】

## ATA/ATAPI IDE Controllers
- `drivers/block/ata.c` accesses legacy IDE primary/secondary ports (0x1F0/0x170
  data windows and 0x3F4/0x374 control) for resets, status polling, and PIO data
  transfers, including per-drive feature detection via port reads.【F:drivers/block/ata.c†L439-L513】
- `include/fiwix/ata.h` establishes controller base addresses, IRQ lines, and
  register layouts that map directly to ISA-style IDE hardware.【F:include/fiwix/ata.h†L17-L200】
- `drivers/block/ata_pci.c` adds PCI bus-master DMA support but still assumes
  BARs expose I/O port ranges and programs the IDE bus master registers with
  `outport_l/outport_b` cycles.【F:drivers/block/ata_pci.c†L21-L135】

## PCI Configuration Space Access
- `drivers/pci/pci.c` uses mechanism #1 (ports 0xCF8/0xCFC) to enumerate and
  configure devices, implying the port-based configuration cycle must be
  emulated or replaced on Arm64 platforms.【F:drivers/pci/pci.c†L244-L304】
- `include/fiwix/pci.h` embeds the configuration address/data port numbers and
  related register offsets used throughout the PCI stack.【F:include/fiwix/pci.h†L15-L132】

## Serial (16550) and Parallel Ports
- The serial driver identifies UART generations and programs baud rates, FIFOs,
  and interrupt masks by touching 0x3F8/0x2F8-style register windows.【F:drivers/char/serial.c†L159-L340】
- `include/fiwix/serial.h` provides the register offsets and IRQ mappings for
  standard PC COM ports.【F:include/fiwix/serial.h†L11-L115】
- The parallel port driver probes and toggles ports 0x378/0x37A to transmit
  data, with control/status semantics matching the PC-compatible printer port
  hardware.【F:drivers/char/lp.c†L102-L213】
- `include/fiwix/lp.h` encodes the printer port base address, status bits, and
  control flags currently assumed.【F:include/fiwix/lp.h†L13-L46】

## VGA Text Console
- The VGA console backend manipulates CRT controller registers at 0x3D4/0x3D5 to
  move the cursor and read/write display memory, directly depending on IBM VGA
  hardware semantics.【F:drivers/video/vgacon.c†L120-L213】
- `include/fiwix/vgacon.h` lists the video memory addresses and CRT register
  indices wired into the console subsystem.【F:include/fiwix/vgacon.h†L13-L49】

## Port-I/O Helper Layer
- The generic `inport/outport` helpers live in `kernel/core386.S` and emit x86
  `in`/`out` instructions, so every subsystem above implicitly depends on these
  opcodes and the presence of ISA-style I/O space.【F:kernel/core386.S†L560-L640】

## Cross-Subsystem Touchpoints
- Multiple subsystems cross-reference shared controller state: e.g. timekeeping
  code pulls RTC data during boot, floppy initialization reads CMOS drive
  descriptors, and PIT routines are used both for scheduling (`timer_init`) and
  CPU calibration. These ties emphasize the need to either emulate the legacy
  devices or design Arm64-native replacements while updating all call sites to
  the new abstractions.【F:kernel/timer.c†L361-L450】【F:drivers/block/floppy.c†L748-L830】

## Arm64 Porting Considerations
- Every component above presumes x86 ISA I/O space and the availability of
  specific southbridge peripherals. For Arm64 we must map each dependency to an
  equivalent (e.g., GIC for interrupt routing, generic timer for PIT, RTC
  framework, PL011 for UART, etc.) or create shims in our early platform code to
  translate these legacy APIs. The catalog here provides the baseline list of
  call sites that will require rework once we choose the target Arm64 platform.
