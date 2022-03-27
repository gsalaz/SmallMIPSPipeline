# SmallMIPSPipeline
C code that converts binary instruction into assembly language that is executed. Implemented the basic assembly instructions like ADD, MULT, SW and AND. Then used a standard five stage pipeline structure with forwarding and branch predition to improve performance.

# What I Learned
- Functions to break up instructions into opcode, function and fields.
- What happens in the Fetch -> Decode -> Execute -> Memory -> Write back stages of a MIPS pipeline.
- Retrieving the necessary data and executing upon it depending the type of function the instruction is asking to perform.
- How to execute forwarding so that an intruction doesn't need to wait for registers to be updated in the write back stage to be used with the correct numbers.
- How to execute a branch prediction that is always taken and adjusts itself it notices it is wrong in the Memory stage.

data:image/png;base64,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