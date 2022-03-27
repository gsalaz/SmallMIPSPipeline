# SmallMIPSPipeline
C code that converts binary instruction into assembly language that is executed. Implemented the basic assembly instructions like ADD, MULT, SW and AND. Then used a standard five stage pipeline structure with forwarding and branch predition to improve performance.

# What I Learned
- Functions to break up instructions into opcode, function and fields.
- What happens in the Fetch -> Decode -> Execute -> Memory -> Write back stages of a MIPS pipeline.
- Retrieving the necessary data and executing upon it depending the type of function the instruction is asking to perform.
- How to execute forwarding so that an intruction doesn't need to wait for registers to be updated in the write back stage to be used with the correct numbers.
- How to execute a branch prediction that is always taken and adjusts itself it notices it is wrong in the Memory stage.

![alt text](https://www.google.com/url?sa=i&url=https%3A%2F%2Fcommons.wikimedia.org%2Fwiki%2FFile%3AMIPS_Architecture_(Pipelined).svg&psig=AOvVaw3HGvHnA-7e_bRPwtrIh1m8&ust=1648433583939000&source=images&cd=vfe&ved=0CAsQjRxqFwoTCLi5iOSb5fYCFQAAAAAdAAAAABAJ![image](https://user-images.githubusercontent.com/71102542/160263737-8a0c1800-724b-4208-a434-86bc664e4df5.png))
