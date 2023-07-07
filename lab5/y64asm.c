/**
 * @file y64asm.c
 * @author BojunRen (albert.cauchy725@gmail.com)
 * @brief
 * @date 2023-03-07
 * @ref https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/6mkuavhra/index.html
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
 */
#define NDEBUG /* Disable all assert */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "y64asm.h"

/**
 * Undefined error message
 */
#define WRONG "Syntax Error"

line_t* line_head = NULL;
line_t* line_tail = NULL;
int lineno = 0;
#define err_print(_s, _a...)       \
    do {                           \
        if (lineno < 0)            \
            fprintf(stderr,        \
                    "[--]: "_s     \
                    "\n",          \
                    ##_a);         \
        else                       \
            fprintf(stderr,        \
                    "[L%d]: "_s    \
                    "\n",          \
                    lineno, ##_a); \
    } while (0);

int64_t vmaddr = 0; /* vm addr */

/* register table */
const reg_t reg_table[REG_NONE] = {
    {"%rax", REG_RAX, 4}, {"%rcx", REG_RCX, 4}, {"%rdx", REG_RDX, 4},
    {"%rbx", REG_RBX, 4}, {"%rsp", REG_RSP, 4}, {"%rbp", REG_RBP, 4},
    {"%rsi", REG_RSI, 4}, {"%rdi", REG_RDI, 4}, {"%r8", REG_R8, 3},
    {"%r9", REG_R9, 3},   {"%r10", REG_R10, 4}, {"%r11", REG_R11, 4},
    {"%r12", REG_R12, 4}, {"%r13", REG_R13, 4}, {"%r14", REG_R14, 4}};
const reg_t* find_register(char* name) {
    int i;
    for (i = 0; i < REG_NONE; i++)
        if (!strncmp(name, reg_table[i].name, reg_table[i].namelen))
            return &reg_table[i];
    return NULL;
}

/* instruction set */
instr_t instr_set[] = {
    {"nop", 3, HPACK(I_NOP, F_NONE), 1},
    {"halt", 4, HPACK(I_HALT, F_NONE), 1},
    {"rrmovq", 6, HPACK(I_RRMOVQ, F_NONE), 2},
    {"cmovle", 6, HPACK(I_RRMOVQ, C_LE), 2},
    {"cmovl", 5, HPACK(I_RRMOVQ, C_L), 2},
    {"cmove", 5, HPACK(I_RRMOVQ, C_E), 2},
    {"cmovne", 6, HPACK(I_RRMOVQ, C_NE), 2},
    {"cmovge", 6, HPACK(I_RRMOVQ, C_GE), 2},
    {"cmovg", 5, HPACK(I_RRMOVQ, C_G), 2},
    {"irmovq", 6, HPACK(I_IRMOVQ, F_NONE), 10},
    {"rmmovq", 6, HPACK(I_RMMOVQ, F_NONE), 10},
    {"mrmovq", 6, HPACK(I_MRMOVQ, F_NONE), 10},
    {"addq", 4, HPACK(I_ALU, A_ADD), 2},
    {"subq", 4, HPACK(I_ALU, A_SUB), 2},
    {"andq", 4, HPACK(I_ALU, A_AND), 2},
    {"xorq", 4, HPACK(I_ALU, A_XOR), 2},
    {"jmp", 3, HPACK(I_JMP, C_YES), 9},
    {"jle", 3, HPACK(I_JMP, C_LE), 9},
    {"jl", 2, HPACK(I_JMP, C_L), 9},
    {"je", 2, HPACK(I_JMP, C_E), 9},
    {"jne", 3, HPACK(I_JMP, C_NE), 9},
    {"jge", 3, HPACK(I_JMP, C_GE), 9},
    {"jg", 2, HPACK(I_JMP, C_G), 9},
    {"call", 4, HPACK(I_CALL, F_NONE), 9},
    {"ret", 3, HPACK(I_RET, F_NONE), 1},
    {"pushq", 5, HPACK(I_PUSHQ, F_NONE), 2},
    {"popq", 4, HPACK(I_POPQ, F_NONE), 2},
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1},
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2},
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4},
    {".quad", 5, HPACK(I_DIRECTIVE, D_DATA), 8},
    {".pos", 4, HPACK(I_DIRECTIVE, D_POS), 0},
    {".align", 6, HPACK(I_DIRECTIVE, D_ALIGN), 0},
    {NULL, 1, 0, 0}  // end
};

instr_t* find_instr(char* name) {
    int i;
    for (i = 0; instr_set[i].name; i++)
        if (strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
            return &instr_set[i];
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t* symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t* find_symbol(char* name) {
    symbol_t* stmp = symtab->next;
    while (stmp) {
        if (strncmp(name, stmp->name, strlen(stmp->name)) == 0)
            return stmp;
        stmp = stmp->next;
    }
    return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char* name) {
    if (find_symbol(name))
        return -1;

    symbol_t* stmp = (symbol_t*)malloc(sizeof(symbol_t));
    memset(stmp, 0, sizeof(symbol_t));
    int n = strlen(name);
    stmp->name = (char*)malloc(sizeof(char) * (n + 1));
    /**
     * FIXED: Use memcpy instead of strncpy to avoid warning
     */
    // strncpy(stmp->name, name, n);
    memcpy(stmp->name, name, n);
    stmp->name[n] = '\0';
    stmp->addr = vmaddr;

    /**
     * @note can also add stmp to tail
     */
    symbol_t* s_first = symtab->next;
    symtab->next = stmp;
    stmp->next = s_first;

    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t* reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 */
void add_reloc(char* name, bin_t* bin) {
    /* create new reloc_t (don't forget to free it)*/
    reloc_t* rtmp = (reloc_t*)malloc(sizeof(reloc_t));
    memset(rtmp, 0, sizeof(reloc_t));
    int n = strlen(name);
    rtmp->name = (char*)malloc(sizeof(char) * (n + 1));
    /**
     * FIXED: Use memcpy instead of strncpy to avoid warning
     */
    // strncpy(rtmp->name, name, n + 1);
    memcpy(rtmp->name, name, n);
    rtmp->name[n] = '\0';
    rtmp->y64bin = bin;

    /* add the new reloc_t to relocation table */
    reloc_t* r_first = reltab->next;
    reltab->next = rtmp;
    rtmp->next = r_first;
}

/* macro for parsing y64 assembly code */
#define IS_DIGIT(s) ((*(s) >= '0' && *(s) <= '9') || *(s) == '-' || *(s) == '+')
#define IS_LETTER(s) ((*(s) >= 'a' && *(s) <= 'z') || (*(s) >= 'A' && *(s) <= 'Z'))
#define IS_COMMENT(s) (*(s) == '#')
#define IS_REG(s) (*(s) == '%')
#define IS_IMM(s) (*(s) == '$')

#define IS_BLANK(s) (*(s) == ' ' || *(s) == '\t')
#define IS_END(s) (*(s) == '\0')

#define SKIP_BLANK(s)                     \
    do {                                  \
        while (!IS_END(s) && IS_BLANK(s)) \
            (s)++;                        \
    } while (0);

#define IS_SYMBOL_TEXT(s) (IS_DIGIT(s) || IS_LETTER(s) || *s == '.' || *s == '_')
/**
 * @brief fill the code array with number,
 *        begin with pc and little-endian
 */
#define FILL_IMM(array, pc, num, bytes) \
    for (int i = 0; i < bytes; ++i) {   \
        array[pc++] = num & 0xFF;       \
        num >>= 8;                      \
    }

/* return value from different parse_xxx function */
typedef enum {
    PARSE_ERR = -1,
    PARSE_REG,
    PARSE_DIGIT,
    PARSE_SYMBOL,
    PARSE_MEM,
    PARSE_DELIM,
    PARSE_INSTR,
    PARSE_LABEL,
} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovq')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char** ptr, instr_t** inst) {
    /* skip the blank */
    SKIP_BLANK(*ptr);

    /* find_instr and check end */
    instr_t* find_i = find_instr(*ptr);
    if (find_i == NULL)
        return PARSE_ERR;

    /* set 'ptr' and 'inst' */
    *ptr += find_i->len;
    *inst = find_i;

    return PARSE_INSTR;
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char** ptr, char delim) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (**ptr != delim) {
        err_print("Invalid \'%c\'", delim);
        return PARSE_ERR;
    }

    /* set 'ptr' */
    *ptr += 1;

    return PARSE_DELIM;
}

/*
 * parse_reg: parse an expected register token (e.g., '%rax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token,
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char** ptr, regid_t* regid) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (!IS_REG(*ptr)) {
        err_print("Invalid REG");
        return PARSE_ERR;
    }

    /* find register */
    const reg_t* find_r = find_register(*ptr);
    if (!find_r) {
        err_print("Invalid REG");
        return PARSE_ERR;
    }

    /* set 'ptr' and 'regid' */
    *ptr += find_r->namelen;
    *regid = find_r->id;

    return PARSE_REG;
}

/**
 * @brief only check symbol, don't err print
 * @param end: point at a space character, or ':'
*/
int check_symbol(char* begin, char* end) {
    /**
     * See function parse_label for more infomation.
     * @note numeric symbol not available in y64?
     *       Yes, there's no need to take it into consideration.
     */
    if (IS_DIGIT(begin))
        return -1;

    if (IS_LETTER(begin)) {
        /* check symbolic label */
        char* s;
        for (s = (begin + 1); s != end; ++s) {
            if (IS_SYMBOL_TEXT(s))
                continue;
            return -1;
        }
    }
    return 0;
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char** ptr, char** name) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    char* symbol_end = *ptr;
    /**
     * FIXED: comma
     */
    while (IS_SYMBOL_TEXT(symbol_end)) {
        ++symbol_end;
    }
    if (check_symbol(*ptr, symbol_end) < 0) {
        err_print("Invalid symbol");
        return PARSE_ERR;
    }

    int name_len = symbol_end - *ptr;

    /* allocate name and copy to it */
    *name = (char*)malloc(sizeof(char) * (name_len + 1));
    /**
     * FIXED: Use memcpy instead of strncpy to avoid warning
     */
    // strncpy(*name, *ptr, name_len);
    memcpy(*name, *ptr, name_len);
    (*name)[name_len] = '\0';

    /* set 'ptr' and 'name' */
    *ptr += name_len;

    return PARSE_SYMBOL;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char** ptr, word_t* value) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (!IS_DIGIT(*ptr)) {
        err_print("Invalid Immediate");
        return PARSE_ERR;
    }

    /* calculate the digit, (NOTE: see strtoll()) */
    char* s_end;
    /**
     * @note A number may overflow if converted as a signed number.
     */
    *value = strtoull(*ptr, &s_end, 0); /* auto base */

    /* set 'ptr' and 'value' */
    *ptr = s_end;

    return PARSE_DIGIT;
}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char** ptr, char** name, word_t* value) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (!IS_IMM(*ptr) && !IS_LETTER(*ptr)) {
        err_print("Invalid DEST");
        return PARSE_ERR;
    }

    /* if IS_IMM, then parse the digit */
    if (IS_IMM(*ptr)) {
        ++*ptr;
        return parse_digit(ptr, value);
    }

    /* if IS_LETTER, then parse the symbol */
    return parse_symbol(ptr, name);

    /* set 'ptr' and 'name' or 'value' */
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%rbp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char** ptr, word_t* value, regid_t* regid) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (**ptr != '(' && !IS_DIGIT(*ptr)) {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }

    /* calculate the digit and register, (ex: (%rbp) or 8(%rbp)) */
    *value = 0;
    if (IS_DIGIT(*ptr) && parse_digit(ptr, value) == PARSE_ERR) {
        /* already err_printed in parse_digit */
        return PARSE_ERR;
    }
    SKIP_BLANK(*ptr);

    /**
     * @todo support a immediate as memory address?
     *       It seems that we don't need to consider it.
     */
    if (**ptr != '(') {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }
    ++*ptr;

    if (parse_reg(ptr, regid) == PARSE_ERR) {
        /* already err_printed in parse_reg */
        return PARSE_ERR;
    }

    if (**ptr != ')') {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }

    /* set 'ptr', 'value' and 'regid' */
    assert(**ptr == ')');
    ++*ptr;
    /* value and register already set */

    return PARSE_MEM;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
/**
 * @attention This function is deprecated since I never use it.
*/
parse_t parse_data(char** ptr, char** name, word_t* value) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (!IS_DIGIT(*ptr) && !IS_LETTER(*ptr)) {
        /**
         * @todo How to err_print?
         */
        err_print(WRONG);
        return PARSE_ERR;
    }

    /* if IS_DIGIT, then parse the digit */
    if (IS_DIGIT(*ptr))
        return parse_digit(ptr, value);

    /* if IS_LETTER, then parse the symbol */
    return parse_symbol(ptr, name);

    /* set 'ptr', 'name' and 'value' */
    /* already set */
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
parse_t parse_label(char** ptr, char** name) {
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    /**
     * From x86 assembly reference:
     * There are two types of labels: symbolic and numeric.
     * A symbolic label consists of an identifier* (or symbol)
     *        followed by a colon (:) (ASCII 0x3A).
     * A numeric label consists of a single digit
     *        in the range zero (0) through nine (9) followed by a colon (:).
     * identifier:
     *      An identifier is an arbitrarily-long sequence of letters and digits.
     *      The first character must be a letter; the underscore (_) (ASCII 0x5F)
     *      and the period (.) (ASCII 0x2E) are considered to be letters.
     *      Case is significant: uppercase and lowercase letters are different.
    */
    char* s_colon = strstr(*ptr, ":");
    if (s_colon == NULL)
        return PARSE_ERR;

    if (check_symbol(*ptr, s_colon) < 0)
        return PARSE_ERR;

    /* allocate name and copy to it */
    int label_len = (int)(s_colon - *ptr);
    *name = (char*)malloc(sizeof(char) * (label_len + 1));
    /**
     * FIXED: Use memcpy instead of strncpy to avoid warning
     */
    memcpy(*name, *ptr, label_len);
    /**
     * @attention Don't check duplicate label here, just parse label
     */

    /* set 'ptr' and 'name' */
    *ptr = s_colon + 1;
    /**
     * @attention I used to write `name[label_len]`, which
     *              caused a awful bug.
     */
    (*name)[label_len] = '\0'; /* null terminated */

    return PARSE_LABEL;
}

/**
 * @brief the tail of a line must be empty, or comment
 *
 * @return -1 error, 0 correct
 */
int check_tail(char* s) {
    /**
     * We don't need to change s.
     * Just make a check.
     */
    SKIP_BLANK(s);
    if (!IS_END(s) && !IS_COMMENT(s))
        return -1;
    return 0;
}

/**
 * @brief parse directive and do something
 *
 * @param line
 * @param s the pointer of the string
 * @param inst
 * @return -1 for error, 0 success
 */
int parse_directive(line_t* line, char** s, instr_t* inst) {
    assert(HIGH(inst->code) == I_DIRECTIVE);
    assert(line->y64bin.codes[0] == inst->code);
    dtv_t dtv = LOW(inst->code);
    word_t V = 0;
    int pc = 0;
    if (dtv == D_DATA) {
        SKIP_BLANK(*s);
        if (IS_DIGIT(*s)) {
            if (parse_digit(s, &V) == PARSE_ERR) {
                err_print("Invalid Immediate");
                return -1;
            }
            FILL_IMM(line->y64bin.codes, pc, V, inst->bytes);
        } else if (IS_LETTER(*s)) {
            char* name = NULL;
            if (parse_symbol(s, &name) == PARSE_ERR) {
                return -1;
            }
            assert(name);
            symbol_t* stmp = find_symbol(name);
            if (stmp) {
                V = stmp->addr;
                FILL_IMM(line->y64bin.codes, pc, V, inst->bytes);
            } else {
                /**
                 * @note Pack the itype and number of bytes to write.
                 *       Will be used in relocate process.
                 */
                line->y64bin.codes[0] = HPACK(HIGH(inst->code), inst->bytes);
                add_reloc(name, &line->y64bin);
            }
            free(name);
            name = NULL;
        } else {
            err_print("Invalid Immediate");
            return -1;
        }
    } else if (parse_digit(s, &V) == PARSE_ERR) {
        /* Otherwise parse an immediate for D_POS or D_ALIGN */
        err_print("Invalid Immediate");
        return -1;
    }

    switch (dtv) {
        case D_DATA:
            break;
        case D_POS:
            vmaddr = V;
            line->y64bin.addr = vmaddr;
            break;
        case D_ALIGN:
            /**
             * @note Integer must be a positive integer
             *       expression and must be a power of 2.
             */
            if (V == 0 || ((V - 1) & V)) {
                err_print("Invalid \'.align\' Instruction");
                return -1;
            }
            word_t mask = ~(V - 1);
            vmaddr = (vmaddr + V - 1) & mask;
            /**
             * @note the commented code below support any align number
             * vmaddr += V;
             * vmaddr -= (vmaddr - 1) % V + 1;
             */
            line->y64bin.addr = vmaddr;
            assert(vmaddr % V == 0);
            break;
        default:
            break;
    }
    return 0;
}
/*
 * parse_line: parse a line of y64 code (e.g., 'Loop: mrmovq (%rcx), %rsi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y64 assembly code
 *
 * return
 *     TYPE_XXX: success, fill line_t with assembled y64 code
 *     TYPE_ERR: error, try to print err information
 */
type_t parse_line(line_t* line) {

    /* when finish parse an instruction or lable, we still need to continue check
     * e.g.,
     *  Loop: mrmovl (%rbp), %rcx
     *           call SUM  #invoke SUM function */

    char* s = line->y64asm;
    SKIP_BLANK(s);

    if (IS_END(s)) {
        /* empty line */
        line->type = TYPE_COMM;
        return line->type;
    }

    /* is a comment ? */
    if (IS_COMMENT(s)) {
        line->type = TYPE_COMM;
        return line->type;
    }

    /* is a label ? */
    char* name = NULL;
    parse_t parse_status = PARSE_ERR;
    if ((parse_status = parse_label(&s, &name)) == PARSE_LABEL) {
        if (add_symbol(name) < 0) {
            err_print("Dup symbol:%s", name);
            line->type = TYPE_ERR;
            return line->type;
        }
    } else {
        /**
         * since ptr after parse_label return PARSE_ERR is undefined
         * we need to reset s
         */
        assert(name == NULL);
        s = line->y64asm;
    }
    free(name);
    name = NULL;

    /* is an instruction ? */
    instr_t* inst = NULL;
    if (parse_instr(&s, &inst) != PARSE_INSTR && parse_status == PARSE_ERR) {
        /* parsing a label or a instruction are failed simultanously */
        err_print(WRONG);
        line->type = TYPE_ERR;
        return line->type;
    }

    /* set type and y64bin */
    assert(parse_status == PARSE_LABEL || inst != NULL);
    if (!inst) {
        if (check_tail(s) < 0) {
            err_print(WRONG);
            line->type = TYPE_ERR;
            return TYPE_ERR;
        }
        /**
         * This line allows a label to be printed when
         *      generating a relocatable file (.yo).
         * @note A directive, in this context, is a instruction too.
         */
        line->type = TYPE_INS;
        line->y64bin.addr = vmaddr;
        line->y64bin.bytes = 0;
        return line->type;
    }
    assert(inst != NULL);
    line->y64bin.codes[0] = inst->code;
    line->y64bin.addr = vmaddr;
    line->y64bin.bytes = inst->bytes;

    /* update vmaddr */
    vmaddr += inst->bytes;

    /* parse the rest of instruction according to the itype */
    itype_t itype = HIGH(inst->code);
    regid_t rA = REG_NONE, rB = REG_NONE;
    word_t V = 0;

    switch (itype) {
        case I_NOP:
        case I_HALT:
        case I_RET:
            if (check_tail(s) < 0) {
                line->type = TYPE_ERR;
                return TYPE_ERR;
            }
            line->type = TYPE_INS;
            return line->type;
        default:
            break;
    }
    /* sparse first argument */
    switch (itype) {
        /* sparse rA if necessary */
        case I_RRMOVQ:
        case I_RMMOVQ:
        case I_ALU:
        case I_POPQ:
        case I_PUSHQ:
            if (parse_reg(&s, &rA) == PARSE_ERR) {
                line->type = TYPE_ERR;
                return TYPE_ERR;
            }
            break;
        default:
            break;
    }
    switch (itype) {
        /* check comma */
        case I_RRMOVQ:
        case I_RMMOVQ:
        case I_ALU:
            SKIP_BLANK(s);
            if (parse_delim(&s, ',') == PARSE_ERR) {
                line->type = TYPE_ERR;
                return TYPE_ERR;
            }
            break;
        default:
            break;
    }
    switch (itype) {
        /* parse imm if necessary */
        case I_IRMOVQ:
        case I_JMP:
        case I_CALL:
            parse_status = parse_imm(&s, &name, &V);
            if (parse_status == PARSE_ERR) {
                line->type = TYPE_ERR;
                return TYPE_ERR;
            }
            SKIP_BLANK(s);
            if (itype == I_IRMOVQ) {
                if (parse_delim(&s, ',') == PARSE_ERR) {
                    line->type = TYPE_ERR;
                    return TYPE_ERR;
                }
            }
            if (parse_status == PARSE_SYMBOL) {
                symbol_t* stmp = find_symbol(name);
                if (stmp) {
                    V = stmp->addr;
                    break;
                }
                /* V is undetermined yet */
                add_reloc(name, &line->y64bin);
            }
            break;
        default:
            break;
    }
    /**
     * NOTE: if (parse_status != PARSE_SYMBOL), name is NULL.
     *       since free does nothing for a null pointer, it's always safe.
     */
    free(name);
    name = NULL;
    /* name variable is useless now */
    switch (itype) {
        /* sparse rB if necessary */
        case I_RRMOVQ:
        case I_IRMOVQ:
        case I_ALU:
            if (parse_reg(&s, &rB) == PARSE_ERR) {
                line->type = TYPE_ERR;
                return TYPE_ERR;
            }
            /* no comma after rB */
            break;
        default:
            break;
    }
    switch (itype) {
        /* sparse memory if necessary */
        case I_RMMOVQ:
        case I_MRMOVQ:
            assert(rB == REG_NONE);
            /* another fact: V is undefined */
            if (parse_mem(&s, &V, &rB) == PARSE_ERR) {
                line->type = TYPE_ERR;
                return TYPE_ERR;
            }
            break;
        default:
            break;
    }
    if (itype == I_MRMOVQ) {
        /* parse rA */
        /* and check comma */
        assert(rA == REG_NONE);
        SKIP_BLANK(s);
        if (parse_delim(&s, ',') == PARSE_ERR) {
            line->type = TYPE_ERR;
            return TYPE_ERR;
        }
        if (parse_reg(&s, &rA)) {
            line->type = TYPE_ERR;
            return TYPE_ERR;
        }
    }

    /* check some facts */
#ifndef NDEBUG
    bool_t f1 = itype == I_IRMOVQ || itype == I_JMP || itype == I_CALL ||
                itype == I_NOP || itype == I_HALT || itype == I_RET ||
                itype == I_DIRECTIVE;
    bool_t f2 = rA == REG_NONE;
    assert(((f1 ^ f2) & 0x1) == 0);
    f1 = itype == I_JMP || itype == I_CALL || itype == I_POPQ || itype == I_PUSHQ ||
         itype == I_NOP || itype == I_HALT || itype == I_RET || itype == I_DIRECTIVE;
    f2 = rB == REG_NONE;
    assert(((f1 ^ f2) & 0x1) == 0);
    /* I have no idea how to check whether V is parsed */
#endif

    /* All parsing complete, now write the code */
    int pc = 1;
    switch (itype) {
        /* write register if necessary */
        case I_IRMOVQ:
        case I_RRMOVQ:
        case I_RMMOVQ:
        case I_MRMOVQ:
        case I_ALU:
        case I_POPQ:
        case I_PUSHQ:
            line->y64bin.codes[pc++] = HPACK(rA, rB);
            break;
        default:
            break;
    }
    switch (itype) {
        /* write imm if necessary */
        case I_RMMOVQ:
        case I_MRMOVQ:
        case I_JMP:
        case I_IRMOVQ:
        case I_CALL:
            FILL_IMM(line->y64bin.codes, pc, V, 8);
            break;
        default:
            break;
    }

    /* we didn't handle directives yet. */
    if (itype == I_DIRECTIVE) {
        parse_directive(line, &s, inst);
    }

    if (check_tail(s) < 0) {
        line->type = TYPE_ERR;
        return TYPE_ERR;
    }
    line->type = TYPE_INS;
    return line->type;
}

/*
 * assemble: assemble an y64 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y64 assembly file)
 *
 * return
 *     0: success, assmble the y64 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE* in) {
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t* line;
    int slen;
    char* y64asm;

    /* read y64 code line-by-line, and parse them to generate raw y64 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen = strlen(asm_buf);
        /* rstrip */
        while ((asm_buf[slen - 1] == '\n') || (asm_buf[slen - 1] == '\r')) {
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y64 assembly code */
        y64asm = (char*)malloc(sizeof(char) * (slen + 1));  // free in finit
        strcpy(y64asm, asm_buf);

        line = (line_t*)malloc(sizeof(line_t));  // free in finit
        memset(line, '\0', sizeof(line_t));

        line->type = TYPE_COMM;
        line->y64asm = y64asm;
        line->next = NULL;

        line_tail->next = line;
        line_tail = line;
        lineno++;

        if (parse_line(line) == TYPE_ERR) {
            return -1;
        }
    }

    lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y64 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void) {
    reloc_t* rtmp = reltab->next;

    while (rtmp) {
        /* find symbol */
        symbol_t* stmp = find_symbol(rtmp->name);
        if (stmp == NULL) {
            err_print("Unknown symbol:\'%s\'", rtmp->name);
            return -1;
        }

        /* relocate y64bin according itype */
        itype_t itype = HIGH(rtmp->y64bin->codes[0]);
        int pc;
        int bytes = 8;
        switch (itype) {
            case I_RMMOVQ:
            case I_MRMOVQ:
            case I_IRMOVQ:
                pc = 2;
                break;
            case I_JMP:
            case I_CALL:
                pc = 1;
                break;
            default:
                assert(itype == I_DIRECTIVE);
                pc = 0;
                /* preconfig */
                bytes = LOW(rtmp->y64bin->codes[0]);
                break;
        }
        word_t imm = stmp->addr;
        FILL_IMM(rtmp->y64bin->codes, pc, imm, bytes);
        /* next */
        rtmp = rtmp->next;
    }
    return 0;
}

/*
 * binfile: generate the y64 binary file
 * args
 *     out: point to output file (an y64 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE* out) {
    /* prepare image with y64 binary code */
    line_t* ltmp = line_head;
    word_t cur_addr = 0;
    byte_t place_holder = 0;

    /* binary write y64 code to output file (NOTE: see fwrite()) */
    while (ltmp) {
        /**
         * FIXED: 1. skip a space instruction to avoid write redundant
         *                  bytes (e.g. y64-app/prog7.ys)
         *        2. fill the blank
         */
        if (ltmp->y64bin.bytes == 0) {
            ltmp = ltmp->next;
            continue;
        }
        while (cur_addr < ltmp->y64bin.addr) {
            fwrite(&place_holder, sizeof place_holder, 1, out);
            ++cur_addr;
            if (ferror(out))
                return -1;
        }
        fwrite(ltmp->y64bin.codes, sizeof(byte_t), ltmp->y64bin.bytes, out);
        cur_addr += ltmp->y64bin.bytes;
        ltmp = ltmp->next;
    }

    return 0;
}

/* whether print the readable output to screen or not ? */
bool_t screen = FALSE;

static void hexstuff(char* dest, int value, int len) {
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4 * i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len - i - 1] = c;
    }
}

void print_line(line_t* line) {
    char buf[64];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t* y64bin = &line->y64bin;
        int i;

        strcpy(buf, "  0x000:                      | ");

        hexstuff(buf + 4, y64bin->addr, 3);
        if (y64bin->bytes > 0)
            for (i = 0; i < y64bin->bytes; i++)
                hexstuff(buf + 9 + 2 * i, y64bin->codes[i] & 0xFF, 2);
    } else {
        strcpy(buf, "                              | ");
    }

    printf("%s%s\n", buf, line->y64asm);
}

/*
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void) {
    line_t* tmp = line_head->next;
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void) {
    /**
     * ATTENTION: dump node (head)
    */
    reltab = (reloc_t*)malloc(sizeof(reloc_t));  // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t*)malloc(sizeof(symbol_t));  // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    line_head = (line_t*)malloc(sizeof(line_t));  // free in finit
    memset(line_head, 0, sizeof(line_t));
    line_tail = line_head;
    lineno = 0;
}

void finit(void) {
    reloc_t* rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name)
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);

    symbol_t* stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name)
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);

    line_t* ltmp = NULL;
    do {
        ltmp = line_head->next;
        if (line_head->y64asm)
            free(line_head->y64asm);
        free(line_head);
        line_head = ltmp;
    } while (line_head);
}

static void usage(char* pname) {
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;

    if (argc < 2)
        usage(argv[0]);

    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
            case 'v':
                screen = TRUE;
                nextarg++;
                break;
            default:
                usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg]) - 3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg] + rootlen, ".ys"))
        usage(argv[0]);

    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }

    /* init */
    init();

    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname + rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }

    if (assemble(in) < 0) {
        err_print("Assemble y64 code error");
        fclose(in);
        exit(1);
    }
    fclose(in);

    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }

    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname + rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }

    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);

    /* print to screen (.yo file) */
    if (screen)
        print_screen();

    /* finit */
    finit();
    return 0;
}
