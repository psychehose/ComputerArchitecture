/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   run.c                                                     */
/*   Adapted from CS311@KAIST                                  */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <math.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc)
{
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction()
{
	/** Implement this function */



    if (INSTRUCTION_COUNT >= NUM_INST) {
        RUN_BIT = FALSE;
        return;
    }


    int curr = INSTRUCTION_COUNT; //Count 값으로 curr를 조정

    switch(INST_INFO[curr].opcode) {
        //Type I

        case 0x9://(0x001001)ADDIU
            CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] + INST_INFO[curr].r_t.r_i.r_i.imm;

            if (CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] < 0) CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = ~(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt]) +1; // problem 5

            INST_INFO[INST_INFO[curr].r_t.r_i.rt].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt];
            break;

        case 0xc:		//(0x001100)ANDI
            CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] & INST_INFO[curr].r_t.r_i.r_i.imm;
            INST_INFO[INST_INFO[curr].r_t.r_i.rt].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt];
            break;

        case 0xf:		//(0x001111)LUI
            CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = INST_INFO[curr].r_t.r_i.r_i.imm << 16; // 상위 16비트
            INST_INFO[INST_INFO[curr].r_t.r_i.rt].value = mem_read_32(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt]);
            break;


        case 0xd:		//(0x001101)ORI
            CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] | INST_INFO[curr].r_t.r_i.r_i.imm;
            INST_INFO[INST_INFO[curr].r_t.r_i.rt].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt];
            break;

        case 0xb:		//(0x001011)SLTIU

            if(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] < INST_INFO[curr].r_t.r_i.r_i.imm) {
                CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = 1;

            } else {
                CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = 0;
            }

            if (CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] < 0) CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = ~(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt]) +1;

            INST_INFO[INST_INFO[curr].r_t.r_i.rt].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt];
            break;

        case 0x23:		//(0x100011)LW
            CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] = mem_read_32(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] + (INST_INFO[curr].r_t.r_i.r_i.imm));
            INST_INFO[INST_INFO[curr].r_t.r_i.rt].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt];
            break;

        case 0x2b:		//(0x101011)SW
            mem_write_32(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] + INST_INFO[curr].r_t.r_i.r_i.imm, CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt]);
            break;

        case 0x4:		//(0x000100)BEQ
            if(INST_INFO[INST_INFO[curr].r_t.r_i.rt].value == INST_INFO[INST_INFO[curr].r_t.r_i.rs].value) {

                CURRENT_STATE.PC += 4*(INST_INFO[curr].r_t.r_i.r_i.imm);

                INSTRUCTION_COUNT += INST_INFO[curr].r_t.r_i.r_i.imm;

                //Count INSTRUCTION_COUNT을 조절 해야 Curr가 변해서 조정
            }

            break;
        case 0x5:		//(0x000101)BNE
            if(INST_INFO[INST_INFO[curr].r_t.r_i.rt].value != INST_INFO[INST_INFO[curr].r_t.r_i.rs].value) {

                CURRENT_STATE.PC += 4*(INST_INFO[curr].r_t.r_i.r_i.imm);
                //I는 count로 결정되기 때문
                INSTRUCTION_COUNT += INST_INFO[curr].r_t.r_i.r_i.imm;
            }

            break;

            //TYPE R
        case 0x0:		//(0x000000)ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU  if JR

            switch (INST_INFO[curr].func_code) {

                case 0x21: //addu
                    CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] + CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs];
                    if (CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] < 0) CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] =  ~(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd]) +1;
                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];


                    break;
                case 0x24: //and
                    CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] & CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs];
                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];

                    break;
                case 0x27: //nor
                    CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = ~CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] & ~CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs];
                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];

                    break;
                case 0x25: //or
                    CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] | CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs];
                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];

                    break;
                case 0x2b: //sltu
                    if(CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] < CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs]) {
                        CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = 1;
                    } else {
                        CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = 0;
                    }

                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];
                    break;
                case 0x00: // sll
                    CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] << INST_INFO[curr].r_t.r_i.r_i.r.shamt;
                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];

                    break;
                case 0x02: // srl
                    CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt] >> INST_INFO[curr].r_t.r_i.r_i.r.shamt;
                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];

                    break;
                case 0x23: //subu
                    CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd] = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] - CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rt];
                    INST_INFO[INST_INFO[curr].r_t.r_i.r_i.r.rd].value = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.r_i.r.rd];

                    break;
                case 0x08: //jr
                    CURRENT_STATE.PC = CURRENT_STATE.REGS[INST_INFO[curr].r_t.r_i.rs] - 0x4;

//                    printf("%2d - Current PC: %x\n", INSTRUCTION_COUNT, CURRENT_STATE.PC);

                    INSTRUCTION_COUNT = (int)(CURRENT_STATE.PC - MEM_TEXT_START)/4;

//                    printf("next %2d - Current PC: %x\n", INSTRUCTION_COUNT, CURRENT_STATE.PC);
                    break;
            }
            // 모든 jump는 label에 도착하기 떄문에 -0x4를 해줘야 정상적으로 돌아가는 듯함 -4를 안해주면 한 줄씩 밀린 채로 출력
            break;
            //TYPE J
        case 0x2:		//(0x000010)J
            CURRENT_STATE.PC = (CURRENT_STATE.PC >> 28 ) | (INST_INFO[curr].r_t.target << 2) - 0x4;
//            printf("%2d - Current PC: %x\n", INSTRUCTION_COUNT, CURRENT_STATE.PC);

            INSTRUCTION_COUNT = (CURRENT_STATE.PC - MEM_TEXT_START)/4;

//            printf("next %2d - Current PC: %x\n", INSTRUCTION_COUNT, CURRENT_STATE.PC);
            break;

        case 0x3:		//(0x000011)JAL
            CURRENT_STATE.REGS[31] = CURRENT_STATE.PC + 0x8;


            CURRENT_STATE.PC = ((int) CURRENT_STATE.PC >>28)| ((int)INST_INFO[curr].r_t.target << 2) - 0x4;
//            printf("%2d - Current PC: %x\n", INSTRUCTION_COUNT, CURRENT_STATE.PC);

            INSTRUCTION_COUNT = (int)(CURRENT_STATE.PC - MEM_TEXT_START)/4;

//            printf("next %2d - Current PC: %x\n", INSTRUCTION_COUNT, CURRENT_STATE.PC);

            break;

        default:
            printf("Not available instruction\n");
            assert(0);
    }
    CURRENT_STATE.PC += 0x4;

}


// example02 n = 16일 때 pc가 차이나는 이유 runbit.