#include <stdio.h>	/* fprintf(), printf() */
#include <stdlib.h>	/* atoi() */
#include <stdint.h>	/* uint32_t */

#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

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
	
	uint32_t opcode;
	int32_t rs;
	int32_t rt;
	int32_t rd;
	int32_t shamt;
	int32_t funct;
	int32_t imm;
	int32_t targetaddress;
	int32_t branchtype;
	uint32_t code;
	bool PC_changed = false;

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);

		//PRINT INSTRUCTION AND PC
		printf("Instruction: 0x%08x\n", CurrentInstruction);
		printf("PC: 0x%08x\n", ProgramCounter);

		//PRINT CONTENTS OF THE REGISTER FILE	
		printRegFile();
		
		//GET OPCODE FROM INSTRUCTION
		uint32_t opcode = (CurrentInstruction >> 26);

		// RESET PC_CHANGED FLAG
		PC_changed = false;

		//EXECUTE INSTRUCTIONS
		switch (opcode)
		{
			// R-Type and Special Instructions
			case 0b00000:
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				rd = (CurrentInstruction >> 11) & 0x1F;
				shamt = (CurrentInstruction >> 6) & 0x1F;
				funct = CurrentInstruction & 0x3F;
				switch (funct)
				{
					// ALU Instructions
					case 0b100000: // ADD
						RegFile[rd] = RegFile[rs] + RegFile[rt];
						break;
					case 0b100001: // ADDU
						RegFile[rd] = (uint32_t)RegFile[(rs)] + (uint32_t)RegFile[(rt)];
						break;
					case 0b100010: // SUB
						RegFile[rd] = RegFile[rs] - RegFile[rt];
						break;
					case 0b100011: // SUBU
						RegFile[rd] = (uint32_t)RegFile[rs] - (uint32_t)RegFile[rt];
						break;
					case 0b100100: // AND
						RegFile[rd] = RegFile[rs] & RegFile[rt];
						break;
					case 0b100101: // OR
						RegFile[rd] = RegFile[rs] | RegFile[rt];
						break;
					case 0b100110: // XOR
						RegFile[rd] = RegFile[rs] ^ RegFile[rt];
						break;
					case 0b100111: // NOR
						RegFile[rd] = ~(RegFile[rs] | RegFile[rt]);
						break;
					case 0b101010: // SLT
						RegFile[rd] = (RegFile[rs] < RegFile[rt]) ? 1 : 0;
						break;
					case 0b101011: // SLTU
						RegFile[rd] = ((uint32_t)RegFile[rs] < (uint32_t)RegFile[rt]) ? 1 : 0;
						break;

					// Shift Instructions
					case 0b000000: // SLL
						RegFile[rd] = RegFile[rt] << shamt;
						break;
					case 0b000010: // SRL
						RegFile[rd] = RegFile[rt] >> shamt;
						break;
					case 0b000011: // SRA
						RegFile[rd] = (int32_t)RegFile[rt] >> shamt;
						break;
					case 0b000100: // SLLV
						RegFile[rd] = RegFile[rt] << RegFile[rs];
						break;
					case 0b000110: // SRLV
						RegFile[rd] = RegFile[rt] >> RegFile[rs];
						break;
					case 0b000111: // SRAV
						RegFile[rd] = (int32_t)RegFile[rt] >> RegFile[rs];
						break;

					// Multiplication and Division Instructions
					case 0b010000: // MFHI
						RegFile[rd] = RegFile[32];
						break;
					case 0b010001: // MTHI
						RegFile[32] = RegFile[rs];
						break;
					case 0b010010: // MFLO
						RegFile[rd] = RegFile[33];
						break;
					case 0b010011: // MTLO
						RegFile[33] = RegFile[rs];
						break;
					case 0b011000: // MULT
						RegFile[32] = (int64_t)(RegFile[rs] * RegFile[rt]) >> 32;
						RegFile[33] = (int64_t)(RegFile[rs] * RegFile[rt]) & 0xFFFFFFFF;
						break;
					case 0b011001: // MULTU
						RegFile[32] = (uint64_t)((uint64_t)RegFile[rs] * (uint64_t)RegFile[rt]) >> 32;
						RegFile[33] = (uint64_t)((uint64_t)RegFile[rs] * (uint64_t)RegFile[rt]) & 0xFFFFFFFF;
						break;
					case 0b011010: // DIV
						RegFile[33] = (int32_t)RegFile[rs] / (int32_t)RegFile[rt];
						RegFile[32] = (int32_t)RegFile[rs] % (int32_t)RegFile[rt];
						break;
					case 0b011011: // DIVU
						RegFile[33] = (uint32_t)RegFile[rs] / (uint32_t)RegFile[rt];
						RegFile[32] = (uint32_t)RegFile[rs] % (uint32_t)RegFile[rt];
						break;

					// Jump and Branch Instructions
					case 0b001000: // JR
						ProgramCounter = RegFile[rs];
						PC_changed = true;
						break;
					case 0b001001: // JALR
						RegFile[31] = ProgramCounter + 4;
						ProgramCounter = RegFile[rs];
						PC_changed = true;
						break;

					// Exception Instructions
					case 0b001100: // SYSCALL
						code = RegFile[2] & 0x0000FFFF;
						SyscallExe(code);
						break;
					case 0b001101: // BREAK
						break;
					default:
						break;
				}
				break;
			
			// I-Type Instructions
			case 0b001000: // ADDI
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				RegFile[rt] = RegFile[rs] + imm;
				break;
			case 0b001001: // ADDIU
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				RegFile[rt] = (uint32_t)RegFile[rs] + (uint32_t)imm;
				break;
			case 0b001010: // SLTI
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				RegFile[rt] = (RegFile[rs] < imm) ? 1 : 0;
				break;
			case 0b001011: // SLTIU
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				RegFile[rt] = ((uint32_t)RegFile[rs] < (uint32_t)imm) ? 1 : 0;
				break;
			case 0b001100: // ANDI
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = CurrentInstruction & 0xFFFF;
				RegFile[rt] = RegFile[rs] & imm;
				break;
			case 0b001101: // ORI
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = CurrentInstruction & 0xFFFF;
				RegFile[rt] = RegFile[rs] | imm;
				break;
			case 0b001110: // XORI
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = CurrentInstruction & 0xFFFF;
				RegFile[rt] = RegFile[rs] ^ imm;
				break;
			case 0b001111: // LUI
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = CurrentInstruction & 0xFFFF;
				RegFile[rt] = imm << 16;
				break;

			// Jump and Branch Instructions
			// Specific Branches
			case 0b000001:
				branchtype = (CurrentInstruction >> 16) & 0x1F;
				switch (branchtype)
				{
					case 0b00000: // BLTZ
						rs = (CurrentInstruction >> 21) & 0x1F;
						imm = (int16_t)CurrentInstruction;
						if (RegFile[rs] < 0)
						{
							ProgramCounter += imm << 2;
							PC_changed = true;
						}
						break;
					case 0b00001: // BGEZ
						rs = (CurrentInstruction >> 21) & 0x1F;
						imm = (int16_t)CurrentInstruction;
						if (RegFile[rs] >= 0)
						{
							ProgramCounter += imm << 2;
							PC_changed = true;
						}
						break;
					case 0b10000: // BLTZAL
						rs = (CurrentInstruction >> 21) & 0x1F;
						imm = (int16_t)CurrentInstruction;
						if (RegFile[rs] < 0)
						{
							RegFile[31] = ProgramCounter + 4;
							ProgramCounter += imm << 2;
							PC_changed = true;
						}
						break;
					case 0b10001: // BGEZAL
						rs = (CurrentInstruction >> 21) & 0x1F;
						imm = (int16_t)CurrentInstruction;
						if (RegFile[rs] >= 0)
						{
							RegFile[31] = ProgramCounter + 4;
							ProgramCounter += imm << 2;
							PC_changed = true;
						}
						break;
				}
				break;
			case 0b000010: // J
				targetaddress = CurrentInstruction & 0x3FFFFFF;
				ProgramCounter = (ProgramCounter & 0xF0000000) | (targetaddress << 2);
				PC_changed = true;
				break;
			case 0b000011: // JAL
				targetaddress = CurrentInstruction & 0x3FFFFFF;
				RegFile[31] = ProgramCounter + 4;
				ProgramCounter = (ProgramCounter & 0xF0000000) | (targetaddress << 2);
				PC_changed = true;
				break;
			case 0b000100: // BEQ
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				if (RegFile[rs] == RegFile[rt])
				{
					ProgramCounter += imm << 2;
					PC_changed = true;
				}
				break;
			case 0b000101: // BNE
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				if (RegFile[rs] != RegFile[rt])
				{
					ProgramCounter += imm << 2;
					PC_changed = true;
				}
				break;
			case 0b000110: // BLEZ
				rs = (CurrentInstruction >> 21) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				if (RegFile[rs] <= 0)
				{
					ProgramCounter += imm << 2;
					PC_changed = true;
				}
				break;
			case 0b000111: // BGTZ
				rs = (CurrentInstruction >> 21) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				if (RegFile[rs] > 0)
				{
					ProgramCounter += imm << 2;
					PC_changed = true;
				}
				break;

			// Load and Store Instructions
			case 0b100000: // LB
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				RegFile[rt] = readByte(RegFile[rs] + imm, true);
				break;
			// LH
			// LWL
			case 0b100011: // LW
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				RegFile[rt] = readWord(RegFile[rs] + imm, true);
				break;
			case 0b100100: // LBU
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				RegFile[rt] = readByte(RegFile[rs] + imm, true);
				break;
			// LHU
			// LWR
			case 0b101000: // SB
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				writeByte(RegFile[rs] + imm, RegFile[rt], true);
				break;
			// SH
			// SWL
			case 0b101011: // SW
				rs = (CurrentInstruction >> 21) & 0x1F;
				rt = (CurrentInstruction >> 16) & 0x1F;
				imm = (int16_t)CurrentInstruction;
				writeWord(RegFile[rs] + imm, RegFile[rt], true);
				break;
			// SWR
			default:
				break;
		}

		if (PC_changed == false)
		{
			// Increment the Program Counter
			ProgramCounter += 4;
		}
		RegFile[0] = 0;
	}   

	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;

}
