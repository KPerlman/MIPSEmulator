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
	uint64_t result;
    int32_t rs_signed;
    int32_t rt_signed;
	TypeR valsR;
	TypeI valsI;
	TypeJ valsJ;

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);

		//PRINT CONTENTS OF THE REGISTER FILE	
		printRegFile();
		
		/********************************/
		/* ADD YOUR IMPLEMENTATION HERE */
		/********************************/


		uint32_t opcode = (CurrentInstruction >> 26);
		
		//Decode instruction for all types of instructions. Only use the relevant type for each instruction
		valsR = readTypeR(CurrentInstruction);
		valsI = readTypeI(CurrentInstruction);
		valsJ = readTypeJ(CurrentInstruction);
		


		if (opcode == 0b000000) { //R type instructions

			switch(valsR.funct)
			{
			  ///////////////////////////
			 /*R TYPE ALU INSTRUCTIONS*/
			///////////////////////////
				case 0b100000: //add
					int32_t rs_signed = (int32_t)RegFile[valsR.rs];  
					int32_t rt_signed = (int32_t)RegFile[valsR.rt];  
					//rd = rs + rt
					int32_t result = rs_signed + rt_signed;       
					RegFile[valsR.rd] = (uint32_t)result;   
					break;

				case 0b100010: //subtract
					// Sign extension
					int32_t rs_signed = (int32_t)RegFile[valsR.rs];  
					int32_t rt_signed = (int32_t)RegFile[valsR.rt];  
					int32_t result = rs_signed - rt_signed;    
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
					int32_t rs_signed = (int32_t)RegFile[valsR.rs];
					int32_t rt_signed = (int32_t)RegFile[valsR.rt];

					if (rs_signed < rt_signed) {
						RegFile[valsR.rd] = 1;
					} else {
						RegFile[valsR.rd] = 0;
					}
					break;
		
				case 0b101011: // Set Less than unsigned
					if (RegFile[valsR.rs] < RegFile[valsR.rt]){
						RegFile[valsR.rd] = 1;
					}else{
						RegFile[valsR.rd] = 0;
					}
					break;
		

			  ///////////////////////////
			 /*   Shift Instructions  */
			///////////////////////////


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



			  ////////////////////////////
			 /*R TYPE JUMP INSTRUCTIONS*/
			////////////////////////////

	
				case 0b001000: // jr (Jump Register)
					//jump to address specified by rs
					ProgramCounter = RegFile[valsR.rs]; 
					break;
	
				case 0b001001: // jalr (Jump and Link Register)
					RegFile[31] = ProgramCounter + 4; // Save address of next instruction into RA
					 // Jump to address specified by rs
					ProgramCounter = RegFile[valsR.rs];
					break;					
					




			} //end of R type switch
		}//end of R type if statement  (if opcode 000000)

			
			
	  ///////////////////////////
	 /*I TYPE ALU INSTRUCTIONS*/
	///////////////////////////
		
		
		if (opcode == 0b001001){ //add immediate unsigned
			//rt = rs + immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] + valsI.imm;
		}

		if (opcode == 0b001000){ //add immediate (signed)
			//Sign extension
			int32_t rs_signed = (int32_t)RegFile[valsI.rs];  
			int32_t imm_signed = (int32_t)(int16_t)valsI.imm;  
			int32_t result = rs_signed + imm_signed;
			//rt = rs + immediate
    		RegFile[valsI.rt] = (uint32_t)result;
		}

		if (opcode == 0b001010){ //slti (set less than immediate)
			//Sign extension
			int32_t rs_signed = (int32_t)RegFile[valsI.rs];
			int32_t imm_signed = (int32_t)(int16_t)valsI.imm;
			if (rs_signed < valsI.imm){
				RegFile[valsI.rt] = 1;
			}else{
				RegFile[valsI.rt] = 0;
			
			}
		}

		if (opcode == 0b001011) { // sltiu (Set Less than unsigned immediate)
			if (RegFile[valsI.rs] < valsI.imm){
				RegFile[valsI.rt] = 1;
			}else{
				RegFile[valsI.rt] = 0;
			}
		}

		if (opcode == 0b001000) { // andi (And Immediate)
			// Zero-Extend to 32 bits
			int32_t imm_ext = (int32_t)valsI.imm;
			//rt is bitwise and between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] & imm_ext;
		}

		if (opcode == 0b001101) { // ori (or Immediate)
			// Zero-Extend to 32 bits
			int32_t imm_ext = (int32_t)valsI.imm;
			//rt is bitwise or between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] | imm_ext;
		}

		if (opcode == 0b001110) { // xori (exclusive or Immediate) 
			// Zero-Extend to 32 bits
			int32_t imm_ext = (int32_t)valsI.imm;
			//rt is bitwise xor between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] ^ imm_ext;
		}

		if (opcode == 0b001111) { // lui (Load Upper Immediate)
			// Extract upper 16 bits of the immediate value and left-shift it by 16 bits
			uint32_t imm_upper = (valsI.imm & 0xFFFF) << 16;
			RegFile[valsI.rt] = imm_upper;
		}



	  //////////////////////////////
	 /*I TYPE BRANCH INSTRUCTIONS*/
	//////////////////////////////


		 if (opcode == 0b00001){ // for all I type instructions with opcode 000001


			switch(valsI.rt) 
			{			
				case 0b000000: //BLTZ (Branch if Less Than Zero)
					if (RegFile[valsI.rs] < 0){
						//sign extension
						int32_t offset = (int32_t)valsI.imm; 						//<><><><><><><><><><><><><><><><><><><><><
						ProgramCounter += 4 + (offset << 2);                        //MIGHT HAVE TO DEFINE "offset" ELSEWHERE
					}
					break;

				case 0b000001: // BGEZ (Branch if Greater Than or Equal to Zero)
					if ((int32_t)RegFile[valsI.rs] >= 0) {
						// Sign extension
						int32_t offset = (int32_t)valsI.imm;
						ProgramCounter += 4 + (offset << 2); // Calculate branch target address
					}
					break;

				case 0b010000: //BLTZAL (Branch on Less Than Zero and Link)
					if (RegFile[valsI.rs] < 0){
						// Save the return address in register $31 (RA)
						RegFile[31] = ProgramCounter + 4;
						//sign extension
						int32_t offset = (int32_t)valsI.imm;
						ProgramCounter += 4 + (offset << 2);                        
					}
					break;

				case 0b010001: //BGEZAL (Branch on Greater Than or Equal to Zero and Link)
					if ((int32_t)RegFile[valsI.rs] >= 0) {
						// Save the return address in register $31 (RA)
						RegFile[31] = ProgramCounter + 4;
						//sign extension
						int32_t offset = (int32_t)valsI.imm;
						ProgramCounter += 4 + (offset << 2);                       
					}
					break;
			}//end of switch
		 }//end of if opcode==00001

		if (opcode == 0b000100){//BEQ (branch on equal)
			if (RegFile[valsI.rs] == RegFile[valsI.rt]){
				//sign extension on offset
				int32_t offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		} 
		if (opcode == 0b000101){//BNE (branch on not equal)
			if (RegFile[valsI.rs] != RegFile[valsI.rt]){
				//sign extension on offset
				int32_t offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		} 
		if (opcode == 0b000110) { // BLEZ (branch on less than or equal to zero)
    		if (RegFile[valsI.rs] <= 0) {
				// Sign extension on offset
				int32_t offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		}
		if (opcode == 0b000111) { // BGTZ (branch on greater than zero)
			if (RegFile[valsI.rs] > 0) {
				// Sign extension on offset
				int32_t offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		}


	  ///////////////////////
	 /*J Type INSTRUCTIONS*/
	///////////////////////

		if (opcode == 0b000010){ // J (jump)
			//concatenate the first 4 bits of the current PC with the lower 26 bits of target address.
			//The last two bits of the address can be assumed to be 0 since instructions are word-aligned
			ProgramCounter = (ProgramCounter & 0xF0000000) | (valsJ.target_address << 2);
		}
		if (opcode == 0b000011) { // jal (jump and link)
			// Store the return address (address of the next instruction) in register $ra (31)
			RegFile[31] = ProgramCounter + 4; 
			
			// Jump to the target address
			ProgramCounter = (ProgramCounter & 0xF0000000) | (valsJ.target_address << 2);
		}




		RegFile[0] = 0;	   
	}//end of instruction loop

	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;


}//end of main

//Function Definitions
TypeI readTypeI(uint32_t instruction) {
    TypeI result;
    result.rs = (instruction >> 21) & 0x1F;
    result.rt = (instruction >> 16) & 0x1F;
    result.imm = instruction & 0xFFFF;
    return result;
}

TypeR readTypeR(uint32_t instruction) {
    TypeR result;
    result.rs = (instruction >> 21) & 0x1F;
    result.rt = (instruction >> 16) & 0x1F;
    result.rd = (instruction >> 11) & 0x1F;
    result.shamt = (instruction >> 6) & 0x1F;
    result.funct = instruction & 0x3F;
    return result;
}

TypeJ readTypeJ(uint32_t instruction) {
    TypeJ result;
    result.target_address = instruction & 0x03FFFFFF; // 26 bits
    return result;
}