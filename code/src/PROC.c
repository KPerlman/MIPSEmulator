#include <stdio.h>	/* fprintf(), printf() */
#include <stdlib.h>	/* atoi() */
#include <stdint.h>	/* uint32_t */

#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

// Define structures for each instruction type
typedef struct {
    uint32_t rs;
    uint32_t rt;
    uint32_t imm;
} TypeI;

typedef struct {
    uint32_t rs;
    uint32_t rt;
    uint32_t rd;
    uint32_t shamt;
    uint32_t funct;
} TypeR;

typedef struct {
    uint32_t target_address;
} TypeJ;

// Function prototypes
TypeI readTypeI(uint32_t instruction);
TypeR readTypeR(uint32_t instruction);
TypeJ readTypeJ(uint32_t instruction);


int main(int argc, char * argv[]) {

	/*
	 * This variable will store the maximum
	 * number of instructions to run before
	 * forcibly terminating the program. It
	 * is set via a command line argument.
	 */
	uint32_t MaxInstructions;

	/*
	 * This variable will store the address
	 * of the next instruction to be fetched
	 * from the instruction memory.
	 */
	uint32_t ProgramCounter;

	/*
	 * This variable will store the instruction
	 * once it is fetched from instruction memory.
	 */
	uint32_t CurrentInstruction;

	//IF THE USER HAS NOT SPECIFIED ENOUGH COMMAND LINE ARUGMENTS
	if(argc < 3){

		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Input argument missing!\n");
		fprintf(stderr, "Expected: file-name, max-instructions\n");
		return -1;

	}

     	//CONVERT MAX INSTRUCTIONS FROM STRING TO INTEGER	
	MaxInstructions = atoi(argv[2]);	

	//Open file pointers & initialize Heap & Regsiters
	initHeap();
	initFDT();
	initRegFile(0);

	//LOAD ELF FILE INTO MEMORY AND STORE EXIT STATUS
	int status = LoadOSMemory(argv[1]);

	//IF LOADING FILE RETURNED NEGATIVE EXIT STATUS
	if(status < 0){ 
		
		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Unable to open file at %s!\n", argv[1]);
		return status; 
	
	}

	printf("\n ----- BOOT Sequence ----- \n");
	printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", exec.GSP, exec.GP, exec.GPC_START);

	RegFile[28] = exec.GP;
	RegFile[29] = exec.GSP;
	RegFile[31] = exec.GPC_START;

	printRegFile();

	printf("\n ----- Execute Program ----- \n");
	printf("Max Instruction to run = %d \n",MaxInstructions);
	fflush(stdout);
	ProgramCounter = exec.GPC_START;
	
	//DECLARE VARIABLES FOR INSTRUCTION EXECUTION
	int32_t result;
	uint64_t result64;
	int32_t rs_signed;
	int32_t rt_signed;
	int32_t imm_signed;
	int32_t imm_ext;
	uint32_t imm_upper;
	TypeR valsR;
	TypeI valsI;
	TypeJ valsJ;

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);

		//PRINT CONTENTS OF THE REGISTER FILE	
		printRegFile();

		uint32_t opcode = (CurrentInstruction >> 26);
		
		if (opcode == 0b000000) { //add
			valsR = readTypeR(CurrentInstruction);
			// Sign extension
			rs_signed = (int32_t)RegFile[valsR.rs];  
			rt_signed = (int32_t)RegFile[valsR.rt];  
			//rd = rs + rt
			result = rs_signed + rt_signed;       
			RegFile[valsR.rd] = (uint32_t)result;        
		} if (opcode == 0b001001){ //add immediate unsigned
			valsI = readTypeI(CurrentInstruction);
			//rt = rs + immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] + valsI.imm;
		} if (opcode == 0b001000){ //add immediate (signed)
			printf("ADDI\n");
			valsI = readTypeI(CurrentInstruction);
			//Sign extension
			rs_signed = (int32_t)RegFile[valsI.rs];  
			imm_signed = (int32_t)(int16_t)valsI.imm;  
			result = rs_signed + imm_signed;
    		RegFile[valsI.rt] = (uint32_t)result;
		} if (opcode == 0b100010) { //subtract
			valsR = readTypeR(CurrentInstruction); 
			// Sign extension
			rs_signed = (int32_t)RegFile[valsR.rs];  
			rt_signed = (int32_t)RegFile[valsR.rt];  
			result = rs_signed - rt_signed;    
			//rd = rs - rt   
			RegFile[valsR.rd] = (uint32_t)result;        
		} if (opcode == 0b100011){ //subtract unsigned
			valsR = readTypeR(CurrentInstruction);
			// rd = rs - rt
			RegFile[valsR.rd] = RegFile[valsR.rs] - RegFile[valsR.rt];	
		} if (opcode == 0b100001){ //add unsigned
			valsR = readTypeR(CurrentInstruction);
			// rd = rs + rt
			RegFile[valsR.rd] = RegFile[valsR.rs] + RegFile[valsR.rt];
		} if (opcode ==0b100100){ //AND
			valsR = readTypeR(CurrentInstruction); 
			RegFile[valsR.rd] = RegFile[valsR.rs] & RegFile[valsR.rt];
		} if (opcode == 0b100101) {//OR
			valsR = readTypeR(CurrentInstruction);
			RegFile[valsR.rd] = RegFile[valsR.rs] | RegFile[valsR.rt];
		} if (opcode == 0b100110) { //XOR
			valsR = readTypeR(CurrentInstruction);
			RegFile[valsR.rd] = RegFile[valsR.rs] ^ RegFile[valsR.rt];
		} if (opcode == 0b100111) { // NOR
			valsR = readTypeR(CurrentInstruction);
			RegFile[valsR.rd] = ~(RegFile[valsR.rs] | RegFile[valsR.rt]);
		} if (opcode == 0b101010) { // Set Less Than (signed)
			valsR = readTypeR(CurrentInstruction);
			//Sign extension
			rs_signed = (int32_t)RegFile[valsR.rs];
			rt_signed = (int32_t)RegFile[valsR.rt];
			if (rs_signed < rt_signed) {
				RegFile[valsR.rd] = 1;
			} else {
				RegFile[valsR.rd] = 0;
			}
		} if (opcode == 0b101011) { //sltu (Set Less than unsigned)
			valsR = readTypeR(CurrentInstruction);
			if (RegFile[valsR.rs] < RegFile[valsR.rt]){
				RegFile[valsR.rd] = 1;
			}else{
				RegFile[valsR.rd] = 0;
			}
		} if (opcode == 0b001010){ //slti (set less than immediate)
			valsI = readTypeI(CurrentInstruction);
			//Sign extension
			rs_signed = (int32_t)RegFile[valsI.rs];
			imm_signed = (int32_t)(int16_t)valsI.imm;
			if (rs_signed < valsI.imm){
				RegFile[valsI.rt] = 1;
			}else{
				RegFile[valsI.rt] = 0;
			
			}
		} if (opcode == 0b001011) { //sltiu (Set Less than unsigned immediate)
			valsI = readTypeI(CurrentInstruction);
			if (RegFile[valsI.rs] < valsI.imm){
				RegFile[valsI.rt] = 1;
			}else{
				RegFile[valsI.rt] = 0;
			}
		} if (opcode == 0b001000) { //andi (And Immediate)
			valsI = readTypeI(CurrentInstruction); 
			// Zero-Extend to 32 bits
			imm_ext = (int32_t)valsI.imm;
			//rt is bitwise and between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] & imm_ext;
		} if (opcode == 0b001101) { //ori (or Immediate)
			valsI = readTypeI(CurrentInstruction); 
			// Zero-Extend to 32 bits
			imm_ext = (int32_t)valsI.imm;
			//rt is bitwise or between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] | imm_ext;
		} if (opcode == 0b001110) { //xori (exclusive or Immediate)
			valsI = readTypeI(CurrentInstruction); 
			// Zero-Extend to 32 bits
			imm_ext = (int32_t)valsI.imm;
			//rt is bitwise xor between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] ^ imm_ext;
		} if (opcode == 0b001111) { //lui (Load Upper Immediate)
			valsI = readTypeI(CurrentInstruction); 
			// Extract upper 16 bits of the immediate value and left-shift it by 16 bits
			imm_upper = (valsI.imm & 0xFFFF) << 16;
			RegFile[valsI.rt] = imm_upper;
		}

		// R TYPE INSTRUCTIONS
		if (opcode == 0b000000)
		{
			valsR = readTypeR(CurrentInstruction);
			switch (valsR.funct)
			{
				// ALU INSTRUCTIONS
                case 0b100000: //add
                    rs_signed = (int32_t)RegFile[valsR.rs];  
                    rt_signed = (int32_t)RegFile[valsR.rt];  
                    //rd = rs + rt
                    result = rs_signed + rt_signed;      
                    RegFile[valsR.rd] = (uint32_t)result;  
                    break;
                case 0b100010: //subtract
                    // Sign extension
                    rs_signed = (int32_t)RegFile[valsR.rs];  
                    rt_signed = (int32_t)RegFile[valsR.rt];  
                    result = rs_signed - rt_signed;    
                    //rd = rs - rt  
                    RegFile[valsR.rd] = (uint32_t)result;
                    break;
                case 0b100011: //subtract unsigned
                    // rd = rs - rt
                    RegFile[valsR.rd] = RegFile[valsR.rs] - RegFile[valsR.rt];
                    break;
                case 0b100001: //add unsigned
                    // rd = rs + rt
                    RegFile[valsR.rd] = RegFile[valsR.rs] + RegFile[valsR.rt];
                    break;
                case 0b100100://AND
                    RegFile[valsR.rd] = RegFile[valsR.rs] & RegFile[valsR.rt];
                    break;
                case 0b100101://OR
                    RegFile[valsR.rd] = RegFile[valsR.rs] | RegFile[valsR.rt];
                    break;      
                case 0b100110: //XOR (exclusive or)
                    //rd is bitwise xor of rs and rt
                    RegFile[valsR.rd] = RegFile[valsR.rs] ^ RegFile[valsR.rt];
                    break;
                case 0b100111: // NOR
                    //rd is bitwise nor of rs and rt
                    RegFile[valsR.rd] = ~(RegFile[valsR.rs] | RegFile[valsR.rt]);
                    break;
                case 0b101010: // Set Less Than (signed)
                    //Sign extension
                    rs_signed = (int32_t)RegFile[valsR.rs];
                    rt_signed = (int32_t)RegFile[valsR.rt];
                    if (rs_signed < rt_signed) {
                        RegFile[valsR.rd] = 1;
                    } else {
                        RegFile[valsR.rd] = 0;
                    }
                    break;
                case 0b101011: // Set Less Than (unsigned)
                    if (RegFile[valsR.rs] < RegFile[valsR.rt]){
                        RegFile[valsR.rd] = 1;
                    }else{
                        RegFile[valsR.rd] = 0;
                    }
                    break;

				// SHIFT INSTRUCTIONS
                case 0b000000: //sll (shift left logical)
                    RegFile[valsR.rd] = RegFile[valsR.rt] << valsR.shamt;
                    break;
                case 0b000010: //srl (shift right logical)
                    RegFile[valsR.rd] = RegFile[valsR.rt] >> valsR.shamt;
                    break;
                case 0b000011: //sra (shift right arithmetic)
                    RegFile[valsR.rd] = (int32_t)RegFile[valsR.rt] >> valsR.shamt;
                    break;
                case 0b000100: //sllv (shift left logical variable)
                    RegFile[valsR.rd] = RegFile[valsR.rt] >> RegFile[valsR.rs];
                    break;
                case 0b000110: // srlv (shift right logical variable)
                    RegFile[valsR.rd] = RegFile[valsR.rt] >> RegFile[valsR.rs];
                    break;
                case 0b000111: // srav (shift right arithmetic variable)
                    RegFile[valsR.rd] = (int32_t)RegFile[valsR.rt] >> RegFile[valsR.rs];
                    break;                  

				// MULT/DIV INSTRUCTIONS
				case 0b010000: // mfhi (Move from HI)
					RegFile[valsR.rd] = RegFile[32];
					break;
				case 0b010010: // mflo (Move from LO)
					RegFile[valsR.rd] = RegFile[33];
					break;
				case 0b010001 : // mthi (Move to HI)
					RegFile[32] = RegFile[valsR.rs];
					break;
				case 0b010011 : // mtlo (Move to LO)
					RegFile[33] = RegFile[valsR.rs];
					break;
				case 0b011000: // mult (Multiply)
					result64 = (int64_t)((int32_t)RegFile[valsR.rs]) * (int64_t)((int32_t)RegFile[valsR.rt]);
					RegFile[32] = (uint32_t)(result64 >> 32);
					RegFile[33] = (uint32_t)(result64 & 0xFFFFFFFF);
					break;
				case 0b011001: // multu (Multiply unsigned)
					result64 = (uint64_t)RegFile[valsR.rs] * (uint64_t)RegFile[valsR.rt];
					RegFile[32] = (uint32_t)(result64 >> 32);
					RegFile[33] = (uint32_t)(result64 & 0xFFFFFFFF);
					break;
				case 0b011010: // div (Divide)
					rs_signed = (int32_t)RegFile[valsR.rs];
					rt_signed = (int32_t)RegFile[valsR.rt];
					if (rt_signed != 0)
					{
						RegFile[32] = (uint32_t)(rs_signed / rt_signed);
						RegFile[33] = (uint32_t)(rs_signed % rt_signed);
					}
					break;
				case 0b011011: // divu (Divide unsigned)
					if (RegFile[valsR.rt] != 0)
					{
						RegFile[32] = RegFile[valsR.rs] / RegFile[valsR.rt];
						RegFile[33] = RegFile[valsR.rs] % RegFile[valsR.rt];
					}
					break;
				default:
					break;
			}
		}

		RegFile[0] = 0;

	}   


	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;

}

//Function Definitions
TypeI readTypeI(uint32_t instruction) {
    TypeI result;
	result.rs = (instruction << 6) >> 27;
	result.rt = (instruction << 11) >> 27;
	result.imm = (instruction << 16) >> 16;	
    return result;
}

TypeR readTypeR(uint32_t instruction) {
    TypeR result;
	result.rs = (instruction << 6) >> 27;
	result.rt = (instruction << 11) >> 27;
	result.rd = (instruction << 16) >> 27;
	result.shamt = (instruction << 21) >> 27;
	result.funct = (instruction << 26) >> 26;
    return result;
}

TypeJ readTypeJ(uint32_t instruction) {
    TypeJ result;
    result.target_address = instruction & 0x03FFFFFF; // 26 bits
    return result;
}