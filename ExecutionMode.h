#pragma once

enum class ExecutionMode {
    Educational,  // Unknown opcodes as NOP; soft memory failures
    Executable      // Illegal instructions and memory faults halt with error
};

enum class FaultCause {
    None = 0,
    IllegalInstruction,
    InstructionFetchFault,
    LoadAddressMisaligned,
    StoreAddressMisaligned,
    LoadFault,
    StoreFault
};
