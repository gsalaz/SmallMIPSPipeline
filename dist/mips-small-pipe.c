#include "mips-small-pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************************************************/
int main(int argc, char *argv[]) {
  short i;
  char line[MAXLINELENGTH];
  state_t state;
  FILE *filePtr;

  if (argc != 2) {
    printf("error: usage: %s <machine-code file>\n", argv[0]);
    return 1;
  }

  memset(&state, 0, sizeof(state_t));

  state.pc = state.cycles = 0;
  state.IFID.instr = state.IDEX.instr = state.EXMEM.instr = state.MEMWB.instr =
      state.WBEND.instr = NOPINSTRUCTION; /* nop */

  /* read machine-code file into instruction/data memory (starting at address 0)
   */

  filePtr = fopen(argv[1], "r");
  if (filePtr == NULL) {
    printf("error: can't open file %s\n", argv[1]);
    perror("fopen");
    exit(1);
  }

  for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
       state.numMemory++) {
    if (sscanf(line, "%x", &state.dataMem[state.numMemory]) != 1) {
      printf("error in reading address %d\n", state.numMemory);
      exit(1);
    }
    state.instrMem[state.numMemory] = state.dataMem[state.numMemory];
    printf("memory[%d]=%x\n", state.numMemory, state.dataMem[state.numMemory]);
  }

  printf("%d memory words\n", state.numMemory);

  printf("\tinstruction memory:\n");
  for (i = 0; i < state.numMemory; i++) {
    printf("\t\tinstrMem[ %d ] = ", i);
    printInstruction(state.instrMem[i]);
  }

  run(&state);

  return 0;
}
/************************************************************/

/************************************************************/
void run(Pstate state) {
  int regA, regAIndex;
  int regB, regBIndex;
  int pendingReg[] = {-1, -1, -1};

  state_t new;
  memset(&new, 0, sizeof(state_t));

  while (1) {

    int instr;
    int op;
    int fun;
    int takeBranch; /* 0 = false and 1 = true*/
    
    int i;

    printState(state);

    /* copy everything so all we have to do is make changes.
       (this is primarily for the memory and reg arrays) */
    memcpy(&new, state, sizeof(state_t));
    
    new.cycles++;

    /* --------------------- IF stage --------------------- */
    
    instr = state->instrMem[state->pc / 4];

    new.IFID.instr = state->instrMem[state->pc / 4];
    new.IFID.pcPlus1 = state->pc + 4;
    
    if(opcode(instr) == BEQZ_OP){
      /*printf("offset = %d", offset(instr));*/
      if(offset(instr) < 0){
        new.pc = new.IFID.pcPlus1 + offset(instr);
      }
      else{
        new.pc = state->pc + 4;
      }
    }
    else{
      new.pc = state->pc + 4;
    }
    

    /* --------------------- ID stage --------------------- */
    op = opcode(state->IFID.instr);
    fun = func(state->IFID.instr);

    new.IDEX.instr = state->IFID.instr;
    new.IDEX.pcPlus1 = state->IFID.pcPlus1;

    new.IDEX.readRegA = state->reg[field_r1(state->IFID.instr)];
    new.IDEX.readRegB = state->reg[field_r2(state->IFID.instr)];
    
    new.IDEX.offset = offset(state->IFID.instr);

    /* --------------------- Stalling --------------------- */
    if(opcode(state->IDEX.instr) == LW_OP && field_r2(state->IDEX.instr) != 0){
      /*if destination of lw is needed for the next instruc.*/
      if(field_r1(state->IFID.instr) == field_r2(state->IDEX.instr) 
      || (field_r2(state->IFID.instr) == field_r2(state->IDEX.instr) &&
      opcode(state->IFID.instr) != LW_OP && opcode(state->IFID.instr) != ADDI_OP)){
        new.pc = state->pc;

        new.IFID.instr = state->IFID.instr;
        new.IFID.pcPlus1 = state->IFID.pcPlus1;

        new.IDEX.instr = NOPINSTRUCTION;
        new.IDEX.pcPlus1 = 0;
        new.IDEX.offset = 32;
        new.IDEX.readRegA = 0;
        new.IDEX.readRegB = 0;
      }
    }
    /*------------------------------------------------------*/
    


    /* --------------------- EX stage --------------------- */
    new.EXMEM.instr = state->IDEX.instr;
    /*moved from bot to top*/
    /*new.EXMEM.readRegB = state->IDEX.readRegB;*/
    op = opcode(state->IDEX.instr);
    fun = func(state->IDEX.instr);

    regA = state->IDEX.readRegA;
    regB = state->IDEX.readRegB;

    regAIndex = field_r1(state->IDEX.instr);
    regBIndex = field_r2(state->IDEX.instr);

    /* ------CHECK FOR FORWARDING------ */
    
    /*printf("\nEXMEM = %d -> %d\nMEMWB = %d -> %d\nWBEND = %d -> %d\n", 
    pendingReg[0], state->EXMEM.aluResult, pendingReg[1], 
    state->MEMWB.writeData, pendingReg[2], state->WBEND.writeData);*/
    

    for(i = 0; i < 3; i++){
      if(regAIndex == pendingReg[i]){
        if(i == 0)/*EXMEM*/{
          regA = state->EXMEM.aluResult;
          break;
        }
        else if(i == 1){/*MEMWB*/
          regA = state->MEMWB.writeData;
          break;
        }
        else{/*WBEND*/
          regA = state->WBEND.writeData;
          break;
        }
      }
    }

    for(i = 0; i < 3; i++){
      if(regBIndex == pendingReg[i]){
        if(i == 0)/*EXMEM*/{
          regB = state->EXMEM.aluResult;
          break;
        }
        else if(i == 1){
          regB = state->MEMWB.writeData;
          break;
        }
        else{
          regB = state->WBEND.writeData;
          break;
        }
      }
    }
      
    /*printf("\nRegA = %d\nRegB = %d\n", regA, regB);*/

    new.EXMEM.readRegB = regB;
      /*------------------------------*/

    if (op == REG_REG_OP) {
            
      if (fun == ADD_FUNC) {
        new.EXMEM.aluResult = regA + regB;
      } else if (fun == SLL_FUNC) {
        new.EXMEM.aluResult = regA << regB;
      } else if (fun == SRL_FUNC) {
        new.EXMEM.aluResult = regA >> regB;
      } else if (fun == SUB_FUNC) {
        new.EXMEM.aluResult = regA - regB;
      } else if (fun == AND_FUNC) {
        new.EXMEM.aluResult = regA & regB;
      } else if (fun == OR_FUNC) {
        new.EXMEM.aluResult = regA | regB;
      }  
      if(field_r3(state->IDEX.instr) == 0){
        new.EXMEM.aluResult = 0;
      }       
    } else if (op == ADDI_OP) {
      new.EXMEM.aluResult = regA + field_imm(state->IDEX.instr);
      new.EXMEM.readRegB = 0;
      if(regBIndex == 0){
        new.EXMEM.aluResult = 0;
      } 
    } else if (op == LW_OP) {
      new.EXMEM.aluResult = regA + field_imm(state->IDEX.instr);
      new.EXMEM.readRegB = 0;;
    } else if (op == SW_OP) {
      new.EXMEM.aluResult = regA + field_imm(state->IDEX.instr);
    } else if (op == BEQZ_OP) {
      new.EXMEM.aluResult = state->IDEX.pcPlus1 + convertNum(field_imm(state->IDEX.instr));
      new.EXMEM.readRegB = regB;
      regAIndex = field_r1(state->IDEX.instr);
      
      /*printf("\nReg[%d] = %d\nimm = %d\noffset = %d\n", 
      field_r1(state->IDEX.instr), state->IDEX.readRegA, 
      field_imm(state->IDEX.instr), offset(state->IDEX.instr));*/
      /*Predit wrong and is taken*/
      if (regA == 0 && offset(state->IDEX.instr) >= 0) {
        /*printf("\nIn IF 1\n");*/
        takeBranch = 1;
        new.pc =  state->IDEX.pcPlus1 + state->IDEX.offset;

        new.IFID.instr = instr = NOPINSTRUCTION;
        new.IFID.pcPlus1 = 0;

        new.IDEX.instr = NOPINSTRUCTION;
        new.IDEX.pcPlus1 = 0;
        new.IDEX.offset = 32;
        new.IDEX.readRegA = 0;
        new.IDEX.readRegB = 0;
      }
      /*Prediction wrong it not taken*/
      if(regA != 0 && offset(state->IDEX.instr) < 0){
        /*printf("\nIn IF 2\n");*/
        
        new.pc =  state->IDEX.pcPlus1;
        
        new.IFID.instr = instr = NOPINSTRUCTION;
        new.IFID.pcPlus1 = 0;

        new.IDEX.instr = NOPINSTRUCTION;
        new.IDEX.pcPlus1 = 0;
        new.IDEX.offset = 32;
        new.IDEX.readRegA = 0;
        new.IDEX.readRegB = 0;
      }
    } else {
      /*insert*/
      new.EXMEM.aluResult = 0;
    }

    /* --------------------- MEM stage --------------------- */
  
    new.MEMWB.instr = state->EXMEM.instr;

    if(opcode(state->EXMEM.instr) == REG_REG_OP || opcode(state->EXMEM.instr) == ADDI_OP
    || opcode(state->EXMEM.instr) == BEQZ_OP){
      new.MEMWB.writeData = state->EXMEM.aluResult;

    }
    else if(opcode(state->EXMEM.instr) == LW_OP){
      if(field_r2(state->EXMEM.instr) == 0){
        new.MEMWB.writeData = 0;
      }
      else{
        new.MEMWB.writeData = state->dataMem[state->EXMEM.aluResult >> 2];
      }
    }
    else if(opcode(state->EXMEM.instr) == SW_OP){
      new.dataMem[state->EXMEM.aluResult / 4] = state->EXMEM.readRegB;
      new.MEMWB.writeData = state->EXMEM.readRegB;
    }
    else{
      new.MEMWB.writeData = 0;
    }

    /*Update pending array*/
    if(opcode(new.EXMEM.instr) == REG_REG_OP){
      if(field_r3(new.EXMEM.instr) != 0){
        pendingReg[0] = field_r3(new.EXMEM.instr);
      }
      else{
        pendingReg[0] = -1;
      }
    }
    if(opcode(new.EXMEM.instr) == ADDI_OP || opcode(new.EXMEM.instr) == LW_OP){
      if(field_r2(new.EXMEM.instr) != 0){
        pendingReg[0] = field_r2(new.EXMEM.instr);
      }
      else{
        pendingReg[0] = -1;
      }
    }

    /* --------------------- WB stage --------------------- */
    if(opcode(state->MEMWB.instr) == REG_REG_OP){
      new.reg[field_r3(state->MEMWB.instr)] = state->MEMWB.writeData;
    }
    if(opcode(state->MEMWB.instr) == ADDI_OP){
      new.reg[field_r2(state->MEMWB.instr)] = state->MEMWB.writeData;
    }
    /*LW Instru: rd = writedata*/
    if(opcode(state->MEMWB.instr) == LW_OP && field_r2(state->MEMWB.instr) != 0){
      new.reg[field_r2(state->MEMWB.instr)] = state->MEMWB.writeData;
    }
    if(opcode(state->MEMWB.instr) == HALT_OP){
      printf("machine halted\n");
      printf("total of %d cycles executed\n", state->cycles);
      exit(0);
    } 

    /*Update pending array*/
    if(opcode(new.MEMWB.instr) == REG_REG_OP){
      if(field_r3(new.MEMWB.instr) != 0){
        pendingReg[1] = field_r3(new.MEMWB.instr);
      }
      else{
        pendingReg[1] = -1;
      }
    }
    if(opcode(new.MEMWB.instr) == ADDI_OP || opcode(new.MEMWB.instr) == LW_OP){
      if(field_r2(new.MEMWB.instr) != 0){
        pendingReg[1] = field_r2(new.MEMWB.instr);
      }
      else{
        pendingReg[1] = -1;
      }
    }

    new.WBEND.instr = state->MEMWB.instr;
    new.WBEND.writeData = state->MEMWB.writeData;

    /* --------------------- end stage --------------------- */

    /*Update pending array*/
    if(opcode(new.WBEND.instr) == REG_REG_OP){
      if(field_r3(new.WBEND.instr) != 0){
        pendingReg[2] = field_r3(new.WBEND.instr);
      }
      else{
        pendingReg[2] = -1;
      }
    }
    else if(opcode(new.WBEND.instr) == ADDI_OP || opcode(new.WBEND.instr) == LW_OP){
      if(field_r2(new.WBEND.instr) != 0){
        pendingReg[2] = field_r2(new.WBEND.instr);
      }
      else{
        pendingReg[2] = -1;
      }
    }
    else{
      pendingReg[2] = -1;
    }

    /*new.pc = state->pc + 4;*/


    /* transfer new state into current state */
    memcpy(state, &new, sizeof(state_t));

  }
}


/************************************************************/
int opcode(int instruction) { return (instruction >> OP_SHIFT) & OP_MASK; }
/************************************************************/

/************************************************************/
int func(int instruction) { return (instruction & FUNC_MASK); }
/************************************************************/

/************************************************************/
int field_r1(int instruction) { return (instruction >> R1_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_r2(int instruction) { return (instruction >> R2_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_r3(int instruction) { return (instruction >> R3_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_imm(int instruction) { return (instruction & IMMEDIATE_MASK); }
/************************************************************/

/************************************************************/
int offset(int instruction) {
  /* only used for lw, sw, beqz */
  return convertNum(field_imm(instruction));
}
/************************************************************/

/************************************************************/
int convertNum(int num) {
  /* convert a 16 bit number into a 32-bit Sun number */
  if (num & 0x8000) {
    num -= 65536;
  }
  return (num);
}
/************************************************************/

/************************************************************/
void printState(Pstate state) {
  short i;
  printf("@@@\nstate before cycle %d starts\n", state->cycles);
  printf("\tpc %d\n", state->pc);

  printf("\tdata memory:\n");
  for (i = 0; i < state->numMemory; i++) {
    printf("\t\tdataMem[ %d ] %d\n", i, state->dataMem[i]);
  }
  printf("\tregisters:\n");
  for (i = 0; i < NUMREGS; i++) {
    printf("\t\treg[ %d ] %d\n", i, state->reg[i]);
  }
  printf("\tIFID:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IFID.instr);
  printf("\t\tpcPlus1 %d\n", state->IFID.pcPlus1);
  printf("\tIDEX:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IDEX.instr);
  printf("\t\tpcPlus1 %d\n", state->IDEX.pcPlus1);
  printf("\t\treadRegA %d\n", state->IDEX.readRegA);
  printf("\t\treadRegB %d\n", state->IDEX.readRegB);
  printf("\t\toffset %d\n", state->IDEX.offset);
  printf("\tEXMEM:\n");
  printf("\t\tinstruction ");
  printInstruction(state->EXMEM.instr);
  printf("\t\taluResult %d\n", state->EXMEM.aluResult);
  printf("\t\treadRegB %d\n", state->EXMEM.readRegB);
  printf("\tMEMWB:\n");
  printf("\t\tinstruction ");
  printInstruction(state->MEMWB.instr);
  printf("\t\twriteData %d\n", state->MEMWB.writeData);
  printf("\tWBEND:\n");
  printf("\t\tinstruction ");
  printInstruction(state->WBEND.instr);
  printf("\t\twriteData %d\n", state->WBEND.writeData);
}
/************************************************************/

/************************************************************/
void printInstruction(int instr) {

  if (opcode(instr) == REG_REG_OP) {

    if (func(instr) == ADD_FUNC) {
      print_rtype(instr, "add");
    } else if (func(instr) == SLL_FUNC) {
      print_rtype(instr, "sll");
    } else if (func(instr) == SRL_FUNC) {
      print_rtype(instr, "srl");
    } else if (func(instr) == SUB_FUNC) {
      print_rtype(instr, "sub");
    } else if (func(instr) == AND_FUNC) {
      print_rtype(instr, "and");
    } else if (func(instr) == OR_FUNC) {
      print_rtype(instr, "or");
    } else {
      printf("data: %d\n", instr);
    }

  } else if (opcode(instr) == ADDI_OP) {
    print_itype(instr, "addi");
  } else if (opcode(instr) == LW_OP) {
    print_itype(instr, "lw");
  } else if (opcode(instr) == SW_OP) {
    print_itype(instr, "sw");
  } else if (opcode(instr) == BEQZ_OP) {
    print_itype(instr, "beqz");
  } else if (opcode(instr) == HALT_OP) {
    printf("halt\n");
  } else {
    printf("data: %d\n", instr);
  }
}
/************************************************************/

/************************************************************/
void print_rtype(int instr, const char *name) {
  printf("%s %d %d %d\n", name, field_r3(instr), field_r1(instr),
         field_r2(instr));
}
/************************************************************/

/************************************************************/
void print_itype(int instr, const char *name) {
  printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
         offset(instr));
}
/************************************************************/
