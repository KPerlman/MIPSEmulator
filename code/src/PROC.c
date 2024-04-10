#include <stdio.h>	/* fprintf(), printf() */
#include <stdlib.h>	/* atoi() */
#include <stdint.h>	/* uint32_t */

#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

// Struct Definitions
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

// Function Prototypes
TypeR getRArguments(uint32_t CurrentInstruction);
TypeI getIArguments(uint32_t CurrentInstruction);
uint32_t getJArguments(uint32_t CurrentInstruction);

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

		//PRINT CONTENTS OF THE REGISTER FILE	
		printRegFile();
		
		/********************************/
		/* ADD YOUR IMPLEMENTATION HERE */
		/********************************/

		uint32_t opcode = (CurrentInstruction >> 26);

		switch (opcode) {
			case 0b00000:
				// R-type
				TypeR RArgs = getRArguments(CurrentInstruction);
		}

		// J-type

		RegFile[0] = 0;
		// RegFile[#] is how you access a register #

	}   

	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;

}

// Function Definitions
TypeR getRArguments(uint32_t CurrentInstruction) {
	TypeR result;
	result.rs = (CurrentInstruction << 6) >> 27;
	result.rt = (CurrentInstruction << 11) >> 27;
	result.rd = (CurrentInstruction << 16) >> 27;
	result.shamt = (CurrentInstruction << 21) >> 27;
	result.funct = (CurrentInstruction << 26) >> 26;
	return result;
}
TypeI getIArguments(uint32_t CurrentInstruction) {
	TypeI result;
	result.rs = (CurrentInstruction << 6) >> 27;
	result.rt = (CurrentInstruction << 11) >> 27;
	result.imm = (CurrentInstruction << 16) >> 27;
	return result;
}
uint32_t getJArguments(uint32_t CurrentInstruction) {
	uint32_t addr = (CurrentInstruction << 6) >> 6;
	return addr;
}