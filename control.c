#include <stdio.h>
#include <stdlib.h>
#include "bits.h"
#include "control.h"
#include "instruction.h"
#include "x16.h"
#include "trap.h"
#include "decode.h"


// Update condition code based on result
void update_cond(x16_t* machine, reg_t reg) {
    uint16_t result = x16_reg(machine, reg);
    if (result == 0) {
        x16_set(machine, R_COND, FL_ZRO);
    } else if (is_negative(result)) {
        x16_set(machine, R_COND, FL_NEG);
    } else {
        x16_set(machine, R_COND, FL_POS);
    }
}



// Execute a single instruction in the given X16 machine. Update
// memory and registers as required. PC is advanced as appropriate.
// Return 0 on success, or -1 if an error or HALT is encountered.
int execute_instruction(x16_t* machine) {
    // Fetch the instruction and advance the program counter
    uint16_t pc = x16_pc(machine);
    uint16_t instruction = x16_memread(machine, pc);
    x16_set(machine, R_PC, pc + 1);

    if (LOG) {
        fprintf(LOGFP, "0x%x: %s\n", pc, decode(instruction));
    }

    // Variables we might need in various instructions
    reg_t dst, src1, src2, base;
    uint16_t result, indirect, offset, imm, cond,
            jsrflag, op1, op2, zer, pos, neg;

    // Decode the instruction
    uint16_t opcode = getopcode(instruction);
    switch (opcode) {
        case OP_ADD:
            dst = (reg_t) getbits(instruction, 9, 3);
            src1 = (reg_t) getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);
            if (getimmediate(instruction) == 0){
                src2 = (reg_t) getbits(instruction, 0, 3);
                op2 = x16_reg(machine, src2);
                result = op1 + op2;
                x16_set(machine, dst, result);
            } else{
                imm = sign_extend(getbits(instruction, 0, 5), 5);
                result = op1 + imm;
                x16_set(machine, dst, result);
            }
            update_cond(machine, dst);
            break;

        case OP_AND:
            dst = (reg_t) getbits(instruction, 9, 3);
            src1 = (reg_t) getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);
            if (getimmediate(instruction) == 0){
                src2 = (reg_t) getbits(instruction, 0, 3);
                op2 = x16_reg(machine, src2);
                result = op1 & op2;
                x16_set(machine, dst, result);
            } else{
                imm = sign_extend(getbits(instruction, 0, 5), 5);
                result = op1 & imm;
                x16_set(machine, dst, result);
            }
            update_cond(machine, dst);
            break;

        case OP_NOT:
            dst = (reg_t) getbits(instruction, 9, 3);
            src1 = (reg_t) getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);
            result = ~(op1);
            x16_set(machine, dst, result);
            update_cond(machine, dst);
            break;

        case OP_BR:
            pos = getbit(instruction, 9);
            zer = getbit(instruction, 10);
            neg = getbit(instruction, 11);
            cond = x16_reg(machine, R_COND);
            pc = x16_reg(machine, R_PC);
            offset = getbits(instruction, 0, 9);
            bool nch = (FL_NEG == cond);
            bool zch = (FL_ZRO == cond);
            bool pch = (FL_POS == cond);
            if ((neg && (nch)) || (zer && (zch)) || (pos && (pch))){
                pc = pc + sign_extend(offset, 9);
                x16_set(machine, R_PC, pc);
            } else if ((neg == 0) && (zer == 0) && (pos == 0)) {
                pc = pc + sign_extend(offset, 9);
                x16_set(machine, R_PC, pc);
            }
            break;

        case OP_JMP:
            pc = x16_reg(machine, R_PC);
            base = (reg_t) getbits(instruction, 6, 3);
            op1 = x16_reg(machine, base);
            pc = op1;
            x16_set(machine, R_PC, pc);
            break;

        case OP_JSR:
            pc = x16_reg(machine, R_PC);
            x16_set(machine, R_R7, pc);
            if (getbit(instruction, 11) == 0) {
                base = (reg_t) getbits(instruction, 6, 3);
                op1 = x16_reg(machine, base);
                pc = op1;
                x16_set(machine, R_PC, pc);
            } else{
                offset = sign_extend(getbits(instruction, 0, 11), 11);
                pc = pc + offset;
                x16_set(machine, R_PC, pc);
            }
            break;

        case OP_LD:
            dst = (reg_t) getbits(instruction, 9, 3);
            pc = x16_reg(machine, R_PC);
            result = x16_reg(machine, dst);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            result = x16_memread(machine, (pc + offset));
            x16_set(machine, dst, result);
            update_cond(machine, dst);
            break;

        case OP_LDI:
            dst = (reg_t) getbits(instruction, 9, 3);
            pc = x16_reg(machine, R_PC);
            result = x16_reg(machine, dst);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            result = x16_memread(machine, x16_memread(machine, (pc + offset)));
            x16_set(machine, dst, result);
            update_cond(machine, dst);
            break;

        case OP_LDR:
            dst = (reg_t) getbits(instruction, 9, 3);
            base = (reg_t) getbits(instruction, 6, 3);
            op1 = x16_reg(machine, base);
            offset = sign_extend(getbits(instruction, 0, 6), 6);
            result = x16_reg(machine, dst);
            result = x16_memread(machine, (offset + op1));
            x16_set(machine, dst, result);
            update_cond(machine, dst);
            break;

        case OP_LEA:
            dst = (reg_t) getbits(instruction, 9, 3);
            pc = x16_reg(machine, R_PC);
            result = x16_reg(machine, dst);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            result = pc + offset;
            x16_set(machine, dst, result);
            update_cond(machine, dst);
            break;

        case OP_ST:
            src1 = (reg_t) getbits(instruction, 9, 3);
            op1 = x16_reg(machine, src1);
            pc = x16_reg(machine, R_PC);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            x16_memwrite(machine, offset + pc, op1);
            break;

        case OP_STI:
            src1 = (reg_t) getbits(instruction, 9, 3);
            op1 = x16_reg(machine, src1);
            pc = x16_reg(machine, R_PC);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            result = x16_memread(machine, (pc + offset));
            x16_memwrite(machine, result , op1);
            break;

        case OP_STR:
            src1 = (reg_t) getbits(instruction, 9, 3);
            indirect = x16_reg(machine, src1);
            base = (reg_t) getbits(instruction, 6, 3);
            op1 = x16_reg(machine, base);
            offset = sign_extend(getbits(instruction, 0, 6), 6);
            x16_memwrite(machine, op1+offset, indirect);
            break;

        case OP_TRAP:
            // Execute the trap -- do not rewrite
            return trap(machine, instruction);

        case OP_RES:
        case OP_RTI:
        default:
            // Bad codes, never used
            abort();
    }

    return 0;
}
