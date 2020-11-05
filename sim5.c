#include <stdio.h>
#include "sim5.h"

//This method gets every field inside an instructionFields
//For R format, we will get opcode, rs, rt, rd, shamt, funct
//For I format, we will get opcode, rs, rt, imm16
//For J format, we will get opcode, address
void extract_instructionFields(WORD instruction, InstructionFields *fieldsOut){
        fieldsOut -> opcode = (instruction >> 26) & 0x3f;
        fieldsOut -> rs = (instruction >> 21) & 0x1f;
        fieldsOut -> rt = (instruction >> 16) & 0x1f;
        fieldsOut -> rd = (instruction >> 11) & 0x1f;
        fieldsOut -> shamt = (instruction >> 6) & 0x1f;
        fieldsOut -> funct = (instruction) & 0x3f;
        fieldsOut -> imm16 = (instruction) & 0xffff;
        fieldsOut -> imm32 = signExtend16to32(fieldsOut -> imm16);
        fieldsOut -> address = (instruction) & 0x03ffffff;
}

//This method determines if a stall is needed for current instruction
int IDtoIF_get_stall(InstructionFields *fields,
                     ID_EX  *old_idex, EX_MEM *old_exmem){
        if (old_idex -> memRead == 1){
                if (old_idex -> regDst == 1 && old_idex -> rd == fields -> rs){
                        return 1;
                }
                if (old_idex -> regDst == 0 && old_idex -> rt == fields -> rs){
                        return 1;
                }
                if (fields -> opcode == 0){
                        if ((old_idex -> regDst == 1 && old_idex -> rd == fields -> rt) || (old_idex -> regDst == 0 && old_idex -> rt == fields -> rt)){
                                return 1;
                        }
                }
        }
        if (fields -> opcode == 0x2b){
                if (old_idex -> regDst == 1 && old_idex -> rd == fields -> rt){
                        return 0;
                }
                if (old_idex -> regDst == 0 && old_idex -> rt == fields -> rt){
                        return 0;
                }
                if (old_exmem -> regWrite == 1 && old_exmem -> writeReg == fields -> rt){
                        return 1;
                }
        }
        return 0;
}

//This method determines if current instruction needs branch or jump
//Return 1 means current instruction needs branch
//Return 2 means current instruction needs jump
//Return 0 means current instruction doesn't need branch and jump
int IDtoIF_get_branchControl(InstructionFields *fields, WORD rsVal, WORD rtVal){
        if (fields -> opcode == 0){
                return 0;
        }
        if (fields -> opcode == 0x04){
                if (rsVal == rtVal){
                        return 1;
                }
                else{
                        return 0;
                }
        }
        if (fields -> opcode == 0x05){
                if (rsVal != rtVal){
                        return 1;
                }
                else{
                        return 0;
                }
        }
        if (fields -> opcode == 0x02){
                return 2;
        }
        if (fields -> opcode != 0x04 || fields -> opcode != 0x05 || fields -> opcode != 0x02){
                return 0;
        }
}

//This method calculates branch address if current instruction needs brach
WORD calc_branchAddr(WORD pcPlus4, InstructionFields *fields){
        return ((fields -> imm32 << 2) + pcPlus4);
}

//This method calculates jump address if current instruction needs jump
WORD calc_jumpAddr  (WORD pcPlus4, InstructionFields *fields){
        return ((pcPlus4 & 0xf0000000) | ((fields -> address << 2) & 0x0fffffff));
}

//This method execute ID, assign proper value to all control bits
//Examine the instruction is valid or not
int execute_ID(int IDstall,
               InstructionFields *fieldsIn,
               WORD pcPlus4,
               WORD rsVal, WORD rtVal,
               ID_EX *new_idex){

        //insert stall
        if (IDstall == 1){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 0;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 0;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = 0;
                new_idex -> rt = 0;
                new_idex -> rd = 0;
                new_idex -> imm16 = 0;
                new_idex -> imm32 = 0;
                new_idex -> rsVal = 0;
                new_idex -> rtVal = 0;
                return 1;
        }
	//perform add
        //perform addu
        else if (fieldsIn -> opcode == 0 && (fieldsIn -> funct == 0x20 || fieldsIn -> funct == 0x21)){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 2;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform sub instruction
        //perform subu instruction
        else if (fieldsIn -> opcode == 0 && (fieldsIn -> funct == 0x22 || fieldsIn -> funct == 0x23)){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 2;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 1;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform addi instruction
        //perform addiu instruction
        else if (fieldsIn -> opcode == 0x08 || fieldsIn -> opcode == 0x09){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 2;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 1;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform and instruction
        else if (fieldsIn -> opcode == 0 && fieldsIn -> funct == 0x24){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 0;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform or instruction
        else if (fieldsIn -> opcode == 0 && fieldsIn -> funct == 0x25){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 1;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform xor instruction
        else if (fieldsIn -> opcode == 0 && fieldsIn -> funct == 0x26){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 4;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform slt instruction
        else if (fieldsIn -> opcode == 0 && fieldsIn -> funct == 0x2a){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 3;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 1;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform slti instruction
        else if (fieldsIn -> opcode == 0x0a){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 3;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 1;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 1;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform lw instruction
        else if (fieldsIn -> opcode == 0x23){
                new_idex -> regDst = 0;
                new_idex -> memRead = 1;
                new_idex -> memToReg = 1;
                new_idex -> ALU.op = 2;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 1;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform sw instruction
        else if (fieldsIn -> opcode == 0x2b){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 2;
                new_idex -> memWrite = 1;
                new_idex -> ALUsrc = 1;
                new_idex -> regWrite = 0;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform beq instruction
        else if (fieldsIn -> opcode == 0x04){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 0;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 0;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = 0;
                new_idex -> rt = 0;
                new_idex -> rd = 0;
                new_idex -> imm16 = 0;
                new_idex -> imm32 = 0;
                new_idex -> rsVal = 0;
                new_idex -> rtVal = 0;
                return 1;
        }
	//perform bne instruction
        else if (fieldsIn -> opcode == 0x05){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 0;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 0;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = 0;
                new_idex -> rt = 0;
                new_idex -> rd = 0;
                new_idex -> imm16 = 0;
                new_idex -> imm32 = 0;
                new_idex -> rsVal = 0;
                new_idex -> rtVal = 0;
                return 1;
        }
	 //perform j instruction
        else if (fieldsIn -> opcode == 0x02){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 0;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 0;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = 0;
                new_idex -> rt = 0;
                new_idex -> rd = 0;
                new_idex -> imm16 = 0;
                new_idex -> imm32 = 0;
                new_idex -> rsVal = 0;
                new_idex -> rtVal = 0;
                return 1;
        }
	//perform andi instruction
        else if (fieldsIn -> opcode == 0x0c){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 0;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 2;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform lui instruction
        else if (fieldsIn -> opcode == 0x0f){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 6;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 1;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	//perform ori instruction
        else if (fieldsIn -> opcode == 0x0d){
                new_idex -> regDst = 0;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 1;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 2;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                return 1;
        }
	 //perform nor instruction
        else if (fieldsIn -> opcode == 0 && fieldsIn -> funct == 0x27){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 1;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = fieldsIn -> rs;
                new_idex -> rt = fieldsIn -> rt;
                new_idex -> rd = fieldsIn -> rd;
                new_idex -> imm16 = fieldsIn -> imm16;
                new_idex -> imm32 = fieldsIn -> imm32;
                new_idex -> rsVal = rsVal;
                new_idex -> rtVal = rtVal;
                new_idex -> extra1 = 1;
                return 1;
        }
	//perform nop instruction
        else if (fieldsIn -> opcode == 0 && fieldsIn -> funct == 0){
                new_idex -> regDst = 1;
                new_idex -> memRead = 0;
                new_idex -> memToReg = 0;
                new_idex -> ALU.op = 5;
                new_idex -> memWrite = 0;
                new_idex -> ALUsrc = 0;
                new_idex -> regWrite = 1;
                new_idex -> ALU.bNegate = 0;
                new_idex -> rs = 0;
                new_idex -> rt = 0;
                new_idex -> rd = 0;
                new_idex -> imm16 = 0;
                new_idex -> imm32 = 0;
                new_idex -> rsVal = 0;
                new_idex -> rtVal = 0;
                return 1;
        }

	else{
		return 0;
	}
}

//This method returns the first input of ALU
WORD EX_getALUinput1(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb){
        if (old_exMem -> regWrite && old_exMem -> writeReg == in -> rs){
                return old_exMem -> aluResult;
        }
        else if (old_memWb -> regWrite && old_memWb -> writeReg == in -> rs){
                return old_memWb -> aluResult;
        }
        return in -> rsVal;
}

//This method returns the second input of ALU
WORD EX_getALUinput2(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb){
        if (old_exMem -> regWrite && old_exMem -> writeReg == in -> rt && in -> ALUsrc == 0){
                return old_exMem -> aluResult;
        }
        else if (old_memWb -> regWrite && old_memWb -> writeReg == in -> rt && in -> ALUsrc == 0){
                return old_memWb -> aluResult;
        }
        if (in -> ALUsrc == 1){
                return in -> imm32;
        }
        if (in -> ALUsrc == 2){
                return in -> imm16;
        }
        return in -> rtVal;
}

//This methods executes ALU and get the aluResult
//0 for and
//1 for or
//2 for add
//3 for less
//4 for xor
//5 for nop
//6 for lui
void execute_EX(ID_EX *in, WORD input1, WORD input2,
                EX_MEM *new_exMem){
        new_exMem -> memRead = in -> memRead;
        new_exMem -> memWrite = in -> memWrite;
        new_exMem -> memToReg = in -> memToReg;
        new_exMem -> regWrite = in -> regWrite;
        new_exMem -> rt = in -> rt;
        new_exMem -> rtVal = in -> rtVal;

        if (in -> ALU.op == 0){
                new_exMem -> aluResult = input1 & input2;
        }
        else if (in -> ALU.op == 1){
                if (in -> extra1 == 1){
                        new_exMem -> aluResult = ~(input1 | input2);
                }
                else{
                        new_exMem -> aluResult = input1 | input2;
                }
        }
        else if (in -> ALU.op == 2 && in -> ALU.bNegate == 0){
                new_exMem -> aluResult = input1 + input2;
        }
        else if (in -> ALU.op == 2 && in -> ALU.bNegate == 1){
                new_exMem -> aluResult = input1 - input2;
        }
        else if (in -> ALU.op == 3){
                if ((input1 - input2) < 0){
                        new_exMem -> aluResult = 1;
                }
                else{
                        new_exMem -> aluResult = 0;
                }
        }
        else if (in -> ALU.op == 4){
                new_exMem -> aluResult = input1 ^ input2;
        }
        else if (in -> ALU.op == 5){
                new_exMem -> aluResult = 0;
        }
        else if (in -> ALU.op == 6){
                new_exMem -> aluResult = input2 << 16;
        }
        if (in -> ALUsrc == 0){
                new_exMem -> writeReg = in -> rd;
        }
        else if (in -> ALUsrc != 0){
                new_exMem -> writeReg = in -> rt;
        }
}

//This method executes memory, if it is lw instruction
//read from memory. if it is sw instruction, it writes
//value back to memory
void execute_MEM(EX_MEM *in, MEM_WB *old_memWb,
                 WORD *mem, MEM_WB *new_memwb){
        new_memwb -> memToReg = in -> memToReg;
        new_memwb -> regWrite = in -> regWrite;
        new_memwb -> aluResult = in -> aluResult;
        new_memwb -> writeReg = in -> writeReg;
        if (in -> memRead == 1){
                new_memwb -> memResult = mem[(in -> aluResult)/4];
        }
        else if (in -> memWrite == 1){
                mem[(in -> aluResult)/4] = in -> rtVal;
                new_memwb -> memResult = 0;
        }
        else{
                new_memwb -> memResult = 0;
        }
	
	if (old_memWb -> regWrite == 1 && old_memWb -> writeReg == in -> rt){
		if (in -> memWrite == 1){
			if (old_memWb -> memToReg == 1){
				mem[(in -> aluResult)/4] = old_memWb -> memResult;
			}
			else{
				mem[(in -> aluResult)/4] = old_memWb -> aluResult;
			}
		}
	}
}

//This methods may update registers
void execute_WB (MEM_WB *in, WORD *regs){
        if (in -> memToReg == 1 && in -> regWrite == 1){
                regs[in -> writeReg] = in -> memResult;
        }
        else if (in -> regWrite == 1){
                regs[in -> writeReg] = in -> aluResult;
        }
}
