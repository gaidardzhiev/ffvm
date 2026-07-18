# FFVM

FFVM is a virtual machine whose runtime is an FFmpeg filtergraph. It executes a stack-based bytecode language by encoding the complete machine state into video frames and applying a fixed filter graph as the transition function. Each output frame is one execution step. The program is data. The graph is the CPU.

The machine has registers, a stack, a heap, and an instruction pointer. All of it lives in pixels. A 256x256 frame holds 65536 addressable bytes. The filter graph reads frame N, decodes the instruction at the current IP, executes it, writes updated state, and emits frame N+1. The graph topology never changes. Behavior changes because state changes. Control flow, including dynamic jumps, is implemented by writing a new value into the IP pixel. No static unrolling, no bounded loops, no predicated execution trees. The interpreter handles it uniformly because the interpreter is the graph.

The output is a video file. Watching it play is watching the machine run.


## The Problem

FFmpeg filtergraphs are static. The topology of which filters connect to which is fixed at launch and cannot change at runtime based on computed values. This appears to rule out dynamic control flow: a jump whose target is a runtime value seems to require rewiring the graph, which is impossible.

The standard response to this constraint is to compile control flow away. Loops get unrolled. Conditionals become mux trees where both branches execute and the result is selected by a condition register. This works for programs whose control flow structure is known at compile time and produces graphs that grow in proportion to program complexity. It does not work for a general-purpose VM that must execute arbitrary programs without knowing their structure in advance.

FFVM takes a different approach. Rather than compiling programs into graph topology, it compiles one fixed interpreter into graph topology and feeds programs as data. The graph is universal. Programs are streams. This is the standard trick of stored-program computation, applied to a substrate that has never hosted it before.

The price is that every execution step runs the full interpreter pipeline regardless of what the current instruction is. The gain is that any program runs on the same graph, including programs that were not written when the graph was built.


## Frame Layout

The machine state is encoded into a 256x256 RGB frame. Each pixel holds one byte in the red channel. The green and blue channels are reserved for debug annotation and carry no machine state. The frame is divided into four regions.

The register file occupies the top row. Pixels 0 through 31 hold the 32 general-purpose registers. Pixels 32 through 39 hold the instruction pointer, stack pointer, frame pointer, flags register, and four reserved system registers. Each register is one byte in this initial encoding, extending to two pixels for 16-bit values and four pixels for 32-bit values as the design scales.

The stack grows downward from pixel 256 at the start of row 1. The stack pointer in the register file holds the current top-of-stack address as a frame-relative pixel index. Push decrements the stack pointer and writes the value. Pop reads the value and increments the stack pointer. Stack overflow is detected when the stack pointer reaches the boundary of the heap region.

The heap occupies rows 2 through 191, giving 48640 addressable bytes of random-access memory. Heap addresses are pixel indices into this region. Load and store instructions take an address operand and read or write the corresponding pixel. The encoding is flat and direct: address N maps to pixel N offset from the start of the heap region.

The program occupies rows 192 through 255, giving 16384 bytes of program memory. The instruction pointer indexes into this region. Instructions are variable width: one byte for the opcode, followed by zero or more immediate bytes depending on the instruction. The program is written into the frame before execution begins and is not modified during execution. Self-modifying code is not supported in this version.

    Frame layout (256x256, one byte per pixel in red channel):
    Row 0:         [ registers 0..31 | IP SP FP FL sys0..3 | reserved  ]
    Rows 1..1:     [ stack, grows downward from pixel 256              ]
    Rows 2..191:   [ heap, 48640 bytes, random access                  ]
    Rows 192..255: [ program ROM, 16384 bytes, read-only at runtime    ]

The green channel of every pixel in the register row holds a live numeric annotation rendered by the filter. The blue channel of the IP pixel is set to 255 at every frame, making the current instruction pointer visible as a bright blue marker in the program region when the frame is displayed at full color. These annotations cost nothing in terms of machine state because they occupy separate channels.


## Instruction Set

The instruction set is minimal. Every instruction is one byte for the opcode. Instructions that take an immediate value are followed by one byte for 8-bit immediates or two bytes in little-endian order for 16-bit immediates. Instructions that take a register operand encode the register index in the low five bits of the opcode byte.

Stack instructions: `push imm8`, `pop`, `dup`, `swap`, `rot`.

Arithmetic instructions: `add`, `sub`, `mul`, `div`, `mod`, `neg`. All operate on the top two stack values and push the result. Division by zero pushes zero and sets the fault flag.

Comparison instructions: `eq`, `lt`, `gt`, `le`, `ge`. Consume two values, push 1 or 0.

Logical instructions: `and`, `or`, `xor`, `not`. Bitwise on 8-bit values.

Register instructions: `loadr reg`, `storer reg`. Move between top of stack and a named register.

Memory instructions: `load addr16`, `store addr16`. Absolute heap access. `loadx`, `storex`. Indirect heap access using top of stack as address.

Control instructions: `jmp addr16`, `jz addr16`, `jnz addr16`. Unconditional and conditional jump. Address is written into the IP pixel. `call addr16`, `ret`. Push return address onto stack, jump; pop return address and jump. `halt`. Set the halt flag in the flags register. The filter detects this and stops emitting frames.

IO instructions: `emit`. Pop top of stack and write it into the output channel as an audio sample value in the paired audio stream. `input`. Push the next value from the input stream onto the stack.

Debug instructions: `trace`. Write current register state into the green channel of the register row. `probe label8`. Write an 8-bit label value into a dedicated probe pixel in the register row, visible in the video output as a color change.


## The Interpreter Graph

The filtergraph implements one function: given frame N as input, produce frame N+1 as output. It does this in five pipeline stages that run as a single filter complex.

Stage one reads the IP register from the known pixel offset in the top row and extracts the current instruction byte from the program region at that address. Both reads use `geq` expressions that index into the frame by computing pixel coordinates from the address value.

Stage two decodes the opcode. A cascade of `if` expressions selects the instruction class and extracts operand bytes from the following pixels. The result is a set of decoded fields: opcode class, operand A, operand B, immediate value, target address.

Stage three executes the instruction. This is the largest stage. For each instruction class there is a corresponding expression that computes the new values of all affected state: stack pointer delta, value to push or pop, register to write, memory address and value for load or store, new IP value for control instructions. For non-control instructions the new IP is the old IP plus the instruction width. For control instructions the new IP is the target address or the old IP plus width depending on the condition value.

Stage four writes the updated state back into the frame. `geq` expressions write computed values into the appropriate pixel coordinates. The stack pointer pixel is updated. If a register was written, the register pixel is updated. If a memory store occurred, the heap pixel at the target address is updated. The IP pixel is updated with the new instruction pointer.

Stage five renders the debug annotation. The green channel of the register row receives the numeric value of each register rendered as an 8-bit intensity. The IP pixel blue channel is set to 255. The `probe` instruction, if it was the current instruction, writes its label into the probe pixel.

The entire graph is generated programmatically by the FFVM assembler. It is not written by hand. The assembler takes bytecode as input and emits the `-filter_complex` string as output. The string is long. For a complete instruction set implementation it runs to several thousand characters. This is not a problem: FFmpeg imposes no practical limit on filter complex length, and the string is generated once and reused for every program that runs on this VM.


## Pixel Encoding

The initial encoding is 8-bit unsigned integers, one value per pixel in the red channel. This limits register width, immediate values, heap addresses, and program addresses to 8 bits, which is sufficient to prove the execution model and run small programs.

The 16-bit extension encodes each value across two adjacent pixels: low byte in pixel N, high byte in pixel N+1. Heap addresses extend to 16 bits, giving access to the full 48640-byte heap. Program addresses extend to 16 bits, covering the full 16384-byte program region. Register values extend to 16 bits. The filter expressions for reading and writing two-pixel values are straightforward: `val = px[N] + px[N+1] * 256` for read, and the reverse decomposition for write.

The 32-bit extension follows the same pattern across four pixels. It is not implemented in the first version.

Arithmetic is performed in the floating-point domain of the FFmpeg expression evaluator and rounded back to integer on write. Overflow is not detected in the 8-bit version; it wraps naturally through the modulo behavior of the byte encoding. The 16-bit version adds explicit overflow detection via the flags register.


## Audio Stream

FFVM runs with a paired audio stream at the same frame rate, one audio sample per video frame. The audio stream carries program output. The `emit` instruction writes its value as the sample amplitude for the current frame. Frames where `emit` was not executed carry a zero sample. The output of a program is the sequence of non-zero samples in the audio stream, extractable with `ffmpeg -vn` after the fact or piped directly to another process.

The input stream, consumed by the `input` instruction, is a pre-rendered audio file supplied alongside the program frame. The assembler produces both the initial state frame and the input audio file from the source program and its input data.

Using audio for IO rather than additional video pixels keeps the frame layout clean and makes IO extraction trivial. It also means a complete FFVM execution is a self-contained media file: video carries computation, audio carries output, and any standard media player shows both simultaneously.


## Assembler

The assembler takes source in FFVM assembly language and produces three artifacts: the initial state frame as a PNG, the input audio stream as a WAV file if the program uses `input`, and the FFmpeg invocation as a shell script.

The shell script runs FFmpeg with the initial state frame as the first input, the input audio as the second input if present, and the generated `-filter_complex` string defining the interpreter graph. It specifies the output as a video file with lossless encoding. Lossy encoding corrupts pixel values and is not permitted.

The assembler is written in C in the first version. The self-hosting goal is to rewrite it in FFVM assembly and compile it with itself, producing a new assembler binary that is an FFVM execution. This requires that FFVM assembly be expressive enough to implement a parser, a symbol table, and a code generator. The instruction set as described is sufficient for this.

FFVM assembly syntax is postfix. Labels are defined with a colon suffix. Instructions are lowercase mnemonics. Immediates follow their instruction on the same line. Comments begin with a semicolon.

    ; compute 3 + 4 and emit the result
    start:
        push 3
        push 4
        add
        emit
        halt


## Implementation

The implementation lives in the repository root alongside this document. Each stage is a self-contained C program or shell script that proves one property of the execution model before the next stage builds on it. No stage assumes anything the previous stage did not verify.

The [Makefile](Makefile) detects the available compiler in order: gcc, musl-gcc, tcc, clang. It falls back to cc if none are found. All binaries are built static. Each stage C file compiles independently with no shared headers or libraries beyond libc!

[verify.sh](./verify.sh) runs all stages in sequence and reports pass or fail for each with a timestamp. It builds any missing binary via make before running. A single failure stops the chain and the exit code is zero only if every stage passes.

[Stage 0](./ffvm_stage0.sh) is a shell script, not a C program. It drives ffmpeg directly with an aevalsrc filter expression that computes Fibonacci numbers using st() and ld() slots across audio samples. It takes an optional argument N for the number of terms. The output is one term per line in the form F(n) = value. This stage proves that stateful computation across samples is possible and establishes the two idioms everything else depends on: backslash-escaped commas inside filter expressions, and 0*f(x) for sequencing side effects without polluting the return value.

[Stage 1](./ffvm_stage1.c) is a C program that proves the pixel memory model. It writes known values into row 0 of a 256x256 PPM frame, feeds the frame through ffmpeg with a geq filter that reads row 0 and writes doubled values into row 1, then reads the output frame back and verifies all 65536 pixels exactly. This stage establishes that r(X,Y), g(X,Y), b(X,Y) are the correct channel read functions in geq RGB mode, and that interpolation=nearest is mandatory. The default bilinear interpolation blends adjacent pixel values and silently corrupts exact integer state. PPM is used throughout because it requires no external library, is trivially written and read in C, and ffmpeg reads and writes it natively.

## Build Sequence

FFVM is not yet fully implemented. This document describes the design.

Stage 0 proves the execution model. A single `aevalsrc` filter computes Fibonacci numbers iteratively using `st()` and `ld()` across audio samples. This confirms that stateful computation across samples is possible, establishes the precision characteristics of the expression evaluator, and identifies the comma-escaping requirement and the `0*(...)` sequencing idiom. Stage 0 is complete.

Stage 1 proves the memory model. A Python script generates an initial state PNG with known pixel values. A hand-written filter complex reads specific pixels and writes computed values into an output frame using `geq` expressions. This confirms that pixel-level read-write works correctly and establishes the encoding conventions for the rest of the implementation.

Stage 2 implements the assembler for the arithmetic and stack instruction subset with no control flow. The assembler generates a filter complex that executes a fixed sequence of instructions. Programs that compute and emit a result are runnable at this stage.

Stage 3 adds control flow. The IP register, the jump instructions, and the instruction fetch pipeline are implemented. Programs with loops and conditionals run at this stage. This is where the full interpreter graph is first exercised and where the stored-program model is validated end to end.

Stage 4 adds the heap and the load and store instructions. Programs with dynamic memory access run at this stage.

Stage 5 implements the debug visualization. The green channel annotation, the IP marker, and the probe instruction are implemented. An FFVM execution becomes visually inspectable frame by frame.

Stage 6 is self-hosting. The assembler is rewritten in FFVM assembly. It is assembled by the Stage 5 assembler, producing an FFVM execution that assembles FFVM programs. The output of assembling the assembler source with itself must match the output of assembling it with the C assembler.


## Constraints

FFmpeg's expression evaluator uses double-precision floating point internally. Integer arithmetic is exact for values up to 2^53. Within the 16-bit value range used by FFVM there is no precision loss. Division produces a floating-point result that is truncated to integer on write; this matches the behavior of integer division in the source language.

The `geq` filter evaluates an expression independently for every pixel in the output frame at every frame. For a 256x256 frame this is 65536 independent evaluations per frame per filter. The interpreter graph applies several filters in sequence, each doing 65536 evaluations. Execution speed is a function of filter count, expression complexity, and CPU throughput, not of the program being executed. All programs run at the same speed: one frame per filtergraph evaluation cycle.

Lossless output encoding is mandatory. The libx264 codec in lossless mode (`-crf 0 -preset ultrafast`) preserves pixel values exactly and is the recommended output format. FFV1 is an alternative. Any lossy codec silently corrupts the machine state and produces incorrect results.

The filter complex string grows with the number of implemented instructions because each instruction requires its own expression branch in the decode and execute stages. The full instruction set as described generates a filter complex string of approximately 50 to 100 kilobytes. This is within FFmpeg's practical limits.

Graph construction time, the time FFmpeg spends parsing and compiling the filter complex before the first frame is produced, scales with string length. For the full instruction set this is expected to be under one second on any modern machine. It is a one-time cost per execution.


## License

This project is provided under the [GPL3 License](./COPYING) Copyright (C) 2026 Ivan Gaydardzhiev
