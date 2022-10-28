# cpuinfo-arch
A small utility for collecting information from arm, aarch64 and riscv processors.

This utility is designed to work in a situation where we can't rely on the conventional methods and status register bits, but need to get actual information using the try-and-see approach.

On armv7 and riscv64 processors, only the NEON/VECTOR IP presence is checked.
