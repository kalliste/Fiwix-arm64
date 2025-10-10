# CPU-Architecture Neutral Components

## Criteria
We classified code as architecture neutral when it relies only on kernel data
structures, scheduler or VFS APIs, and generic synchronization helpers that we
can reimplement on Arm64 without rewriting higher-level logic. Modules that
only touch memory buffers, generic device abstractions, or pure data tables are
included even if they call macros such as `SAVE_FLAGS()`/`CLI()` that will gain
new Arm64 implementations.

## Filesystem Layer
- `fs/` core registration logic manipulates filesystem tables and relies solely
  on VFS operations, making it portable once low-level helpers are retargeted.
  【F:fs/filesystems.c†L8-L74】
- `fs/ext2/`, `fs/minix/`, `fs/iso9660/`, and related subtrees operate purely on
  on-disk metadata and kernel buffers with no CPU-specific assembly
  dependencies, so their code can be reused as-is.【F:fs/ext2/super.c†L8-L120】
- Pseudo-filesystems like `procfs`, `pipefs`, `devpts`, and `sockfs` are built
  entirely from VFS hooks and scheduler interactions, keeping them independent
  from the underlying CPU architecture.【F:fs/procfs/super.c†L8-L82】

## Networking Stack
- The networking core (`net/`) manages socket lifecycle, queues, and protocol
  dispatching using portable data structures. Its only low-level coupling is
  through interrupt masking macros that will gain Arm64 backends.【F:net/socket.c†L8-L116】
- Domain-specific entry points such as UNIX sockets and packet routing sit atop
  the same abstractions, so the logic can remain intact when the trap entry path
  changes.【F:net/unix.c†L8-L118】

## Architecture-Independent Drivers
- The RAM disk block driver operates entirely on memory-resident images through
  generic buffer-cache helpers, so it carries no x86 hardware requirements and
  should port directly.【F:drivers/block/ramdisk.c†L8-L128】
- Partition table parsing is purely a metadata task that depends on buffer
  reads, not on ISA I/O primitives, making `drivers/block/part.c` portable across
  CPU families.【F:drivers/block/part.c†L8-L43】

## Shared Utility Libraries
- String and character utility routines are pure C helpers without hardware
  touch points, allowing the existing implementations in `lib/strings.c` and
  `lib/ctype.c` to be reused on Arm64.【F:lib/strings.c†L8-L47】【F:lib/ctype.c†L8-L48】

## Documentation and Research Artifacts
- The textual references under `docs/` (device lists, memory layout notes,
  toolchain guidance) remain valid inputs for the Arm64 effort regardless of the
  target CPU.【F:docs/devices.txt†L1-L32】
- Existing research notes in `porting/` capture broader planning context without
  binding to x86 specifics, so they can continue to guide the Arm64 porting
  roadmap.【F:porting/PORTINGRESEARCH.md†L1-L72】
