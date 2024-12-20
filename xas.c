#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include "instruction.h"


void usage() {
    fprintf(stderr, "Usage: ./xas file");
    exit(1);
}

typedef struct label{
    char label_name[100];
    int line_num;
}label_t;

uint16_t obtain_literal(char* parsed_instruc, int index) {
    if (parsed_instruc[index] != '$') {
        fprintf(stderr, "ERROR INSIDE OBTAIN_LITERAL");
        return 0;
    } else{
        int length = strlen(parsed_instruc) - 1;
        char* retval;
        int count = index + 1;
        for (int i = 0; i < length - (index + 1) + 1; i ++) {
            retval[i] = parsed_instruc[count];
            count++;
        }
        int ret = atoi(retval);
        return ret;
    }
}

int obtain_pcoffset(char* buf, struct label* la, int lac, int lnum){
    int PCoffset = -10000;
    for (int i = 0; i < lac; i++) {
        if (strstr(buf, la[i].label_name) != NULL) {
            if (PCoffset != -10000){
                continue;
            } else{
                PCoffset = la[i].line_num - lnum;
            }
        }
    }
    if (PCoffset == -10000) {
        // checks that the label actually existed
        fprintf(stderr, "PCoffset IS NULL\n");
        exit(2);
    }
    return PCoffset;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage();
    }
    // Open up new a.obj file and write the first address needed in big endian.
    FILE* aobj = fopen("a.obj", "a");
    fseek(aobj, 0, SEEK_SET);
    uint16_t origin = 0x3000;
    origin = ntohs(origin);
    fwrite(&origin, sizeof(origin), 1, aobj);

    // Open up input file and get ready to read lines through it.
    FILE* inpfile = fopen(argv[1], "r");
    fseek(inpfile, 0, SEEK_SET);

    char buffer[100];
    int fplinenum = 0;
    int spln = 0;
    struct label* label_arr = malloc(sizeof(label_t)*5);
    int lcnt = 0;

    while (fgets(buffer, sizeof(buffer), inpfile) != NULL) {
        // write everything to a.obj with bad offsets and bad regs.
        int tempsize = strlen(buffer) - 2;  // indicie for the last char in str

        // use a for loop and loop through until you find the instruction start.
        int instruc_start = 0;
        while (isspace(buffer[instruc_start]) != 0) {
            instruc_start++;
        }

        unsigned char check_colon = buffer[tempsize];
        unsigned char check_comment = buffer[instruc_start];
        unsigned char check_comment2 = buffer[instruc_start + 4];

        if (buffer[instruc_start] == '\0') {
            continue;
        } else if ((check_comment == '#') || (check_comment2 == '#')){
            continue;
        } else if (check_colon == ':') {
            strcpy(label_arr[lcnt].label_name, buffer);
            label_arr[lcnt].label_name[tempsize] = '\0';
            label_arr[lcnt].line_num = fplinenum;
            lcnt++;
        } else {  // IT IS AN INSTRUCTION SO DO THINGS ACCORDINGLY
            uint16_t temp_instruction;
            if (strstr(buffer, "add") != NULL) {
                if (strchr(buffer, '$') != NULL) {
                    temp_instruction = emit_add_imm(0, 0, 0);
                } else{
                    temp_instruction = emit_add_reg(0, 0, 0);
                }
            } else if (strstr(buffer, "and") != NULL) {
                if (strchr(buffer, '$') != NULL) {
                    temp_instruction = emit_and_imm(0, 0, 0);
                } else{
                    temp_instruction = emit_and_reg(0, 0, 0);
                }
            } else if (strstr(buffer, "br") != NULL) {
                if (strstr(buffer, "brnzp") != NULL) {
                    temp_instruction = emit_br(true, true, true, 0);
                } else if (strstr(buffer, "brnz") != NULL) {
                    temp_instruction = emit_br(true, true, false, 0);
                } else if (strstr(buffer, "brnp") != NULL) {
                    temp_instruction = emit_br(true, false, true, 0);
                } else if (strstr(buffer, "brzp") != NULL) {
                    temp_instruction = emit_br(false, true, true, 0);
                } else if (strstr(buffer, "brn") != NULL) {
                    temp_instruction = emit_br(true, false, false, 0);
                } else if (strstr(buffer, "brz") != NULL) {
                    temp_instruction = emit_br(false, true, false, 0);
                } else if (strstr(buffer, "brp") != NULL) {
                    temp_instruction = emit_br(false, false, true, 0);
                } else {
                    temp_instruction = emit_br(false, false, false, 0);
                }
            } else if (strstr(buffer, "ret") != NULL) {
                temp_instruction = emit_jmp(0);
            } else if (strstr(buffer, "jmp") != NULL) {
                temp_instruction = emit_jmp(0);
            } else if (strstr(buffer, "jsr") != NULL) {
                if (strstr(buffer, "jsrr") != NULL) {
                    temp_instruction = emit_jsrr(0);
                } else{
                    temp_instruction = emit_jsr(0);
                }
            } else if (strstr(buffer, "val") != NULL) {
                temp_instruction = emit_value(0);
            } else if (strstr(buffer, "ld") != NULL) {
                if (strstr(buffer, "ldi") != NULL) {
                    temp_instruction = emit_ldi(0, 0);
                } else if (strstr(buffer, "ldr") != NULL) {
                    temp_instruction = emit_ldr(0, 0, 0);
                } else {
                    temp_instruction = emit_ld(0, 0);
                }
            } else if (strstr(buffer, "lea") != NULL) {
                temp_instruction = emit_lea(0, 0);
            } else if (strstr(buffer, "not") != NULL) {
                temp_instruction = emit_not(0, 0);
            } else if (strstr(buffer, "st") != NULL) {
                if (strstr(buffer, "sti") != NULL) {
                    temp_instruction = emit_sti(0, 0);
                } else if (strstr(buffer, "str") != NULL) {
                    temp_instruction = emit_str(0, 0, 0);
                } else {
                    temp_instruction = emit_st(0, 0);
                }
            } else if (strstr(buffer, "getc") != NULL) {
                temp_instruction = emit_trap(TRAP_GETC);
            } else if (strstr(buffer, "putc") != NULL) {
                temp_instruction = emit_trap(TRAP_OUT);
            } else if (strstr(buffer, "putsp") != NULL) {
                temp_instruction = emit_trap(TRAP_PUTSP);
            } else if (strstr(buffer, "enter") != NULL) {
                temp_instruction = emit_trap(TRAP_IN);
            } else if (strstr(buffer, "puts") != NULL) {
                temp_instruction = emit_trap(TRAP_PUTS);
            } else if (strstr(buffer, "halt") != NULL) {
                temp_instruction = emit_trap(TRAP_HALT);
            }
            // write the blank intruction to mem and increase line num count.
            temp_instruction = ntohs(temp_instruction);
            fwrite(&temp_instruction, sizeof(temp_instruction), 1, aobj);
            fplinenum++;
        }
    }

    // START SECOND PASS THROUGH!
    // go back to start of the a.obj after the origin entry and input file
    fclose(aobj);
    FILE* newaobj = fopen("a.obj", "w");
    uint16_t neworigin = 0x3000;
    neworigin = ntohs(neworigin);
    fwrite(&neworigin, sizeof(neworigin), 1, newaobj);
    fseek(inpfile, 0, SEEK_SET);

    int dst, src1, src2, src3, offset;
    uint16_t literal;

    while (fgets(buffer, sizeof(buffer), inpfile) != NULL) {
        int tempsize = strlen(buffer) - 2;

        int instruc_start = 0;
        while (isspace(buffer[instruc_start]) != 0) {
            instruc_start++;
        }

        unsigned char check_colon = buffer[tempsize];
        unsigned char check_comment = buffer[instruc_start];
        unsigned char check_comment2 = buffer[instruc_start + 4];
        int PCoff;

        if (buffer[instruc_start] == '\0') {
            continue;
        } else if ((check_comment == '#') || (check_comment2 == '#')){
            continue;
        } else if (check_colon == ':') {
            // already got all the labels from the first run through
            continue;
        } else if (strstr(buffer, "br") != NULL) {  // little parsing so push up
            spln++;
            char* phrase;
            char* sep = ", ";
            char p_instruc[50] = "";
            phrase = strtok(buffer, sep);
            while (phrase != NULL) {
                strcat(p_instruc, phrase);
                phrase = strtok(NULL, sep);
            }
            PCoff = obtain_pcoffset(p_instruc, label_arr, lcnt, spln);
            uint16_t temp_instruction;

            if (strstr(p_instruc, "brnzp") != NULL) {
                temp_instruction = emit_br(true, true, true, PCoff);
            } else if (strstr(p_instruc, "brnz") != NULL) {
                temp_instruction = emit_br(true, true, false, PCoff);
            } else if (strstr(p_instruc, "brnp") != NULL) {
                temp_instruction = emit_br(true, false, true, PCoff);
            } else if (strstr(p_instruc, "brzp") != NULL) {
                temp_instruction = emit_br(false, true, true, PCoff);
            } else if (strstr(p_instruc, "brn") != NULL) {
                temp_instruction = emit_br(true, false, false, PCoff);
            } else if (strstr(p_instruc, "brz") != NULL) {
                temp_instruction = emit_br(false, true, false, PCoff);
            } else if (strstr(p_instruc, "brp") != NULL) {
                temp_instruction = emit_br(false, false, true, PCoff);
            } else {
                temp_instruction = emit_br(false, false, false, PCoff);
            }
            temp_instruction = ntohs(temp_instruction);
            fwrite(&temp_instruction, sizeof(temp_instruction), 1, newaobj);
        } else {
            spln++;
            char* phrase;
            char* sep = ", ";
            char p_instruc[50] = "";
            phrase = strtok(buffer, sep);
            while (phrase != NULL) {
                strcat(p_instruc, phrase);
                phrase = strtok(NULL, sep);
            }
            // p_instruc has parsed instruction

            // find out where to start reading registers

            uint16_t temp_instruction;
            if (strstr(buffer, "add") != NULL) {
                if (p_instruc[3] != '%' || p_instruc[6] != '%') {
                    fprintf(stderr, "Usage: ./xas file");
                    exit(2);
                }
                if (strchr(p_instruc, '$') != NULL) {
                    literal = obtain_literal(p_instruc, 9);
                    dst = atoi(&p_instruc[5]);
                    src1 = atoi(&p_instruc[8]);
                    temp_instruction = emit_add_imm(dst, src1, literal);
                } else{
                    if (p_instruc[9] != '$' && p_instruc[9] != '%') {
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    dst = atoi(&p_instruc[5]);
                    src1 = atoi(&p_instruc[8]);
                    src2 = atoi(&p_instruc[11]);
                    temp_instruction = emit_add_reg(dst, src1, src2);
                }
            } else if (strstr(buffer, "and") != NULL) {
                if (p_instruc[3] != '%' || p_instruc[6] != '%') {
                    fprintf(stderr, "Usage: ./xas file");
                    exit(2);
                }
                if (strchr(p_instruc, '$') != NULL) {
                    literal = obtain_literal(p_instruc, 9);
                    dst = atoi(&p_instruc[5]);
                    src1 = atoi(&p_instruc[8]);
                    temp_instruction = emit_add_imm(dst, src1, literal);
                } else{
                    if (p_instruc[9] != '$' && p_instruc[9] != '%') {
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    dst = atoi(&p_instruc[5]);
                    src1 = atoi(&p_instruc[8]);
                    src2 = atoi(&p_instruc[11]);
                    temp_instruction = emit_add_reg(dst, src1, src2);
                }
            } else if (strstr(buffer, "ret") != NULL) {
                temp_instruction = emit_jmp(R_R7);
            } else if (strstr(buffer, "jmp") != NULL) {
                if (p_instruc[3] != '%') {
                    fprintf(stderr, "Usage: ./xas file");
                    exit(2);
                }
                dst = atoi(&p_instruc[5]);
                temp_instruction = emit_jmp(dst);
            } else if (strstr(buffer, "jsr") != NULL) {
                if (strstr(buffer, "jsrr") != NULL) {
                    if (p_instruc[4] != '%') {
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    dst = atoi(&p_instruc[6]);
                    temp_instruction = emit_jsrr(dst);
                } else{
                    PCoff = obtain_pcoffset(p_instruc, label_arr, lcnt, spln);
                    temp_instruction = emit_jsr(PCoff);
                }
            } else if (strstr(buffer, "val") != NULL) {
                literal = obtain_literal(p_instruc, 3);
                temp_instruction = emit_value(literal);
            } else if (strstr(buffer, "ld") != NULL) {
                if (strstr(buffer, "ldi") != NULL) {
                    if (p_instruc[3] != '%') {
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    PCoff = obtain_pcoffset(p_instruc, label_arr, lcnt, spln);
                    dst = atoi(&p_instruc[5]);
                    temp_instruction = emit_ldi(dst, PCoff);
                } else if (strstr(buffer, "ldr") != NULL) {
                    if (p_instruc[3] != '%') {
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    literal = obtain_literal(p_instruc, 9);
                    dst = atoi(&p_instruc[5]);
                    src1 = atoi(&p_instruc[8]);
                    temp_instruction = emit_ldr(dst, src1 , literal);
                } else {
                    if (p_instruc[2] != '%'){
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    PCoff = obtain_pcoffset(p_instruc, label_arr, lcnt, spln);
                    dst = atoi(&p_instruc[4]);
                    temp_instruction = emit_ld(dst, PCoff);
                }
            } else if (strstr(buffer, "lea") != NULL) {
                if (p_instruc[3] != '%') {
                    fprintf(stderr, "Usage: ./xas file");
                    exit(2);
                }
                PCoff = obtain_pcoffset(p_instruc, label_arr, lcnt, spln);
                dst = atoi(&p_instruc[5]);
                temp_instruction = emit_lea(dst, PCoff);
            } else if (strstr(buffer, "not") != NULL) {
                if (p_instruc[3] != '%' && p_instruc[6]) {
                    fprintf(stderr, "Usage: ./xas file");
                    exit(2);
                }
                dst = atoi(&p_instruc[5]);
                src1 = atoi(&p_instruc[8]);
                temp_instruction = emit_not(dst, src1);
            } else if (strstr(buffer, "st") != NULL) {
                if (strstr(buffer, "sti") != NULL) {
                    if (p_instruc[3] != '%') {
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    PCoff = obtain_pcoffset(p_instruc, label_arr, lcnt, spln);
                    dst = atoi(&p_instruc[5]);
                    temp_instruction = emit_sti(dst, PCoff);
                } else if (strstr(buffer, "str") != NULL) {
                    if (p_instruc[3] != '%') {
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    literal = obtain_literal(p_instruc, 9);
                    dst = atoi(&p_instruc[5]);
                    src1 = atoi(&p_instruc[8]);
                    temp_instruction = emit_str(dst, src1, literal);
                } else {
                    if (p_instruc[2] != '%'){
                        fprintf(stderr, "Usage: ./xas file");
                        exit(2);
                    }
                    PCoff = obtain_pcoffset(p_instruc, label_arr, lcnt, spln);
                    dst = atoi(&p_instruc[4]);
                    temp_instruction = emit_st(dst, PCoff);
                }
            } else if (strstr(p_instruc, "getc") != NULL) {
                temp_instruction = emit_trap(TRAP_GETC);
            } else if (strstr(p_instruc, "putc") != NULL) {
                temp_instruction = emit_trap(TRAP_OUT);
            } else if (strstr(p_instruc, "putsp") != NULL) {
                temp_instruction = emit_trap(TRAP_PUTSP);
            } else if (strstr(p_instruc, "enter") != NULL) {
                temp_instruction = emit_trap(TRAP_IN);
            } else if (strstr(p_instruc, "puts") != NULL) {
                temp_instruction = emit_trap(TRAP_PUTS);
            } else if (strstr(p_instruc, "halt") != NULL) {
                temp_instruction = emit_trap(TRAP_HALT);
            }
            temp_instruction = ntohs(temp_instruction);
            fwrite(&temp_instruction, sizeof(temp_instruction), 1, newaobj);
        }
    }
    return 0;
}
