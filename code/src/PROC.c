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
	
	/***************************/
	/* ADD YOUR VARIABLES HERE */
	/***************************/
	

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);
		//CurrentInstruction = 0b00100100000000010000000000000110;

		//PRINT CONTENTS OF THE REGISTER FILE	
		printRegFile();
		
		/********************************/
		/* ADD YOUR IMPLEMENTATION HERE */
		/********************************/

		uint32_t opcode = (CurrentInstruction >> 26);

		
		/********************************/
		/* 	     	ALU Instructions	 */
		/********************************/

		
		if (opcode == 0b000000) { //add
			TypeR vals = readTypeR(CurrentInstruction);
			// Sign extension
			int32_t rs_signed = (int32_t)RegFile[vals.rs];  
			int32_t rt_signed = (int32_t)RegFile[vals.rt];  
			//rd = rs + rt
			int32_t result = rs_signed + rt_signed;       
			RegFile[vals.rd] = (uint32_t)result;        
		}
		
		if (opcode == 0b001001){ //add immediate unsigned
			TypeI vals = readTypeI(CurrentInstruction);
			//rt = rs + immediate
			RegFile[vals.rt] = RegFile[vals.rs] + vals.imm;
		}

		if (opcode == 0b001000){ //add immediate (signed)
			TypeI vals = readTypeI(CurrentInstruction);
			//Sign extension
			int32_t rs_signed = (int32_t)RegFile[vals.rs];  
			int32_t imm_signed = (int32_t)(int16_t)vals.imm;  
			int32_t result = rs_signed + imm_signed;
			//rt = rs + immediate
    		RegFile[vals.rt] = (uint32_t)result;
		}

		
		if (opcode == 0b100010) { //subtract
			TypeR vals = readTypeR(CurrentInstruction); 
			// Sign extension
			int32_t rs_signed = (int32_t)RegFile[vals.rs];  
			int32_t rt_signed = (int32_t)RegFile[vals.rt];  
			int32_t result = rs_signed - rt_signed;    
			//rd = rs - rt   
			RegFile[vals.rd] = (uint32_t)result;        
		}

		
		if (opcode == 0b100011){ //subtract unsigned
			TypeR vals = readTypeR(CurrentInstruction);
			// rd = rs - rt
			RegFile[vals.rd] = RegFile[vals.rs] - RegFile[vals.rt];
			
		}

		if (opcode == 0b100001){ //add unsigned
			TypeR vals = readTypeR(CurrentInstruction);
			// rd = rs + rt
			RegFile[vals.rd] = RegFile[vals.rs] + RegFile[vals.rt];
		}
		

		
		if (opcode ==0b100100){//AND
			TypeR vals = readTypeR(CurrentInstruction); 
			RegFile[vals.rd] = RegFile[vals.rs] & RegFile[vals.rt];
		}

		
		if (opcode == 0b100101) {//OR
			TypeR vals = readTypeR(CurrentInstruction);
			RegFile[vals.rd] = RegFile[vals.rs] | RegFile[vals.rt];
		}

		
		if (opcode == 0b100110) { //XOR
			TypeR vals = readTypeR(CurrentInstruction);
			RegFile[vals.rd] = RegFile[vals.rs] ^ RegFile[vals.rt];
		}

		if (opcode == 0b100111) { // NOR
			TypeR vals = readTypeR(CurrentInstruction);
			RegFile[vals.rd] = ~(RegFile[vals.rs] | RegFile[vals.rt]);
		}

		if (opcode == 0b101010) { // Set Less Than (signed)
			TypeR vals = readTypeR(CurrentInstruction);
			//Sign extension
			int32_t rs_signed = (int32_t)RegFile[vals.rs];
			int32_t rt_signed = (int32_t)RegFile[vals.rt];

			if (rs_signed < rt_signed) {
				RegFile[vals.rd] = 1;
			} else {
				RegFile[vals.rd] = 0;
			}
		}

		if (opcode == 0b101011) { // Set Less than unsigned
			TypeR vals = readTypeR(CurrentInstruction);
			if (RegFile[vals.rs] < RegFile[vals.rt]){
				RegFile[vals.rd] = 1;
			}else{
				RegFile[vals.rd] = 0;
			}
		}

		if (opcode == 0b001010){ //slti (set less than immediate)
			TypeI vals = readTypeI(CurrentInstruction);
			//Sign extension
			int32_t rs_signed = (int32_t)RegFile[vals.rs];
			int32_t imm_signed = (int32_t)(int16_t)vals.imm;
			if (rs_signed < vals.imm){
				RegFile[vals.rt] = 1;
			}else{
				RegFile[vals.rt] = 0;
			
			}
		}

		if (opcode == 0b001011) { // sltiu (Set Less than unsigned immediate)
			TypeI vals = readTypeI(CurrentInstruction);
			if (RegFile[vals.rs] < vals.imm){
				RegFile[vals.rt] = 1;
			}else{
				RegFile[vals.rt] = 0;
			}
		}

		if (opcode == 0b001000) { // andi (And Immediate)
			TypeI vals = readTypeI(CurrentInstruction); 
			// Zero-Extend to 32 bits
			int32_t imm_ext = (int32_t)vals.imm;
			//rt is bitwise and between rs and immediate
			RegFile[vals.rt] = RegFile[vals.rs] & imm_ext;
		}

		if (opcode == 0b001101) { // ori (or Immediate)
			TypeI vals = readTypeI(CurrentInstruction); 
			// Zero-Extend to 32 bits
			int32_t imm_ext = (int32_t)vals.imm;
			//rt is bitwise or between rs and immediate
			RegFile[vals.rt] = RegFile[vals.rs] | imm_ext;
		}

		if (opcode == 0b001110) { // xori (exclusive or Immediate)
			TypeI vals = readTypeI(CurrentInstruction); 
			// Zero-Extend to 32 bits
			int32_t imm_ext = (int32_t)vals.imm;
			//rt is bitwise xor between rs and immediate
			RegFile[vals.rt] = RegFile[vals.rs] ^ imm_ext;
		}

		if (opcode == 0b001111) { // lui (Load Upper Immediate)
			TypeI vals = readTypeI(CurrentInstruction); 

			// Extract upper 16 bits of the immediate value and left-shift it by 16 bits
			uint32_t imm_upper = (vals.imm & 0xFFFF) << 16;
			RegFile[vals.rt] = imm_upper;
		}

		/********************************/
		/*  	Multiply and Divide     */
		/********************************/

		if (opcode == 0b010000) // mfhi (Move from HI)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			RegFile[vals.rd] = RegFile[32];
		}

		if (opcode == 0b010010) // mflo (Move from LO)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			RegFile[vals.rd] = RegFile[33];
		}

		if (opcode == 0b010001) // mthi (Move to HI)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			RegFile[32] = RegFile[vals.rs];
		}

		if (opcode == 0b010011) // mtlo (Move to LO)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			RegFile[33] = RegFile[vals.rs];
		}

		if (opcode == 0b011000) // mult (Multiply)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			int64_t result = (int64_t)((int32_t)RegFile[vals.rs]) * (int64_t)((int32_t)RegFile[vals.rt]);
			RegFile[32] = (uint32_t)(result >> 32);
			RegFile[33] = (uint32_t)(result & 0xFFFFFFFF);
		}

		if (opcode == 0b011001) // multu (Multiply unsigned)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			uint64_t result = (uint64_t)RegFile[vals.rs] * (uint64_t)RegFile[vals.rt];
			RegFile[32] = (uint32_t)(result >> 32);
			RegFile[33] = (uint32_t)(result & 0xFFFFFFFF);
		}

		if (opcode == 0b011010) // div (Divide)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			int32_t rs_signed = (int32_t)RegFile[vals.rs];
			int32_t rt_signed = (int32_t)RegFile[vals.rt];
			if (rt_signed != 0)
			{
				RegFile[32] = (uint32_t)(rs_signed / rt_signed);
				RegFile[33] = (uint32_t)(rs_signed % rt_signed);
			}
		}

		if (opcode == 0b011011) // divu (Divide unsigned)
		{
			TypeR vals = readTypeR(CurrentInstruction);
			if (RegFile[vals.rt] != 0)
			{
				RegFile[32] = RegFile[vals.rs] / RegFile[vals.rt];
				RegFile[33] = RegFile[vals.rs] % RegFile[vals.rt];
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