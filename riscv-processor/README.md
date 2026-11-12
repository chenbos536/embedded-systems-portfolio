# 32-bit RISC-V Processor

A Verilog course project that connects instruction decode, register access, immediate generation, ALU operations, and memory access in a 32-bit datapath. The design implements a subset of RISC-V instructions.

## Simulate in Vivado

1. Add the files in `rtl/` as design sources.
2. Add `sim/tb_processor.v` as a simulation source and set `tb_processor` as the simulation top.
3. Run Behavioral Simulation. Inspect the testbench output and trace the datapath and control signals in the waveform window.

The testbench applies a clock and reset and compares observed processor outputs with expected values for the program in `rtl/InstMem.v`.

The testbench reported 20 correct output checks in Vivado Simulator 2025.2.
