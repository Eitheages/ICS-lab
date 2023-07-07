/**
 * @file csim.c
 * @author Bojun Ren
 * @Id 521021910788
 *
 * @date 2023-05-07
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
 */
#define NDEBUG
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

/** Number of physical (main memory) address bits */
#define m 64
/** where to print the error message */
#define ERROR_OUT stderr
#define AVAIL(block, bit) ((block >> bit) & 01)
/** max length of instruction */
#define MAX_INSLEN 30

typedef uint64_t add_t;
typedef enum { LOAD, STORE, MODIFY } op_t;
enum { FALSE, TRUE };

/**
 * A line with valid bit and tag.
 *      In this lab, there is no need to store block bytes.
 */
typedef struct line_type {
    int valid;
    /** t bits */
    int tag;
    struct line_type* next;
} line_t;

/**
 * A set is combined with multiple lines, where
 *      the lines are stored in a linked list.
 */
typedef line_t* set_t;

/**
 * A cache is combined with multiple sets.
 */
typedef set_t* cache_t;

/**
 * @brief parse an instruction line, e.g. ` L 10,1`.
 *              A line with a leading space is ignored.
 *
 * @param line the instruction line, null-terminated
 * @return 0 on success, -1 on invalid
 */
int parse_inst(char* line, op_t* op, add_t* addr, int* size);

void init_cache();
void destory_cache();
/** Basic operations */
void load(add_t addr, int size);
void modify(add_t addr, int size);
void store(add_t addr, int size);

/**
 * fucntion utils
 */
void unix_error(const char*, ...);
void usage(FILE*, const char*);

/** [begin] global variables */
extern char* optarg;
extern int opterr;
/** s: set index bits, E: number of lines per set, b: block offset bits */
int s, E, b;
/** S: 2^s number of sets, t: number of tag bits */
int S, t;
int verbose = 0;
char inst_line[MAX_INSLEN + 1];
cache_t cache;
int hits, misses, evictions;
/** [end] global variables */

int main(int argc, char* argv[]) {
    int opt;
    char test_file[30];

    s = E = b = test_file[0] = 0;
    S = 1;

    while ((opt = getopt(argc, argv, "vhs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                usage(stdout, argv[0]);
                printf("Examples:\n");
                printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n",
                       argv[0]);
                printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n",
                       argv[0]);
                exit(0);
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
            case 't':
                strcpy(test_file, optarg);
                break;
            default:
                break;
        }
    }
    S = 1 << s;
    t = m - (s + b);
    // printf("s = %d, E = %d, b = %d, v = %d, t = %s\n", s, E, b, verbose, test_file);
    if (s == 0 || E == 0 || b == 0 || test_file[0] == 0) {
        unix_error("%s: Missing required command line argument\n", argv[0]);
        usage(ERROR_OUT, argv[0]);
        exit(0);
    }

    FILE* fp = fopen(test_file, "r");
    if (!fp) {
        perror(test_file);
        exit(0);
    }

    init_cache();

    op_t op;
    add_t addr;
    int size;
    while (fgets(inst_line, MAX_INSLEN, fp) != NULL) {
        if (parse_inst(inst_line, &op, &addr, &size) >= 0) {
            // printf("%d, %lx, %d\n", op, addr, size);
            switch (op) {
                case LOAD:
                    load(addr, size);
                    break;
                case STORE:
                    store(addr, size);
                    break;
                case MODIFY:
                    modify(addr, size);
                    break;
            }
        }
    }
    printSummary(hits, misses, evictions);

    destory_cache();
    return 0;
}

void usage(FILE* fp, const char* pname) {
    fprintf(fp, "Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n",
            pname);
    fprintf(fp, "Options:\n");
    fprintf(fp, "  -h         Print this help message.\n");
    fprintf(fp, "  -v         Optional verbose flag.\n");
    fprintf(fp, "  -s <num>   Number of set index bits.\n");
    fprintf(fp, "  -E <num>   Number of lines per set.\n");
    fprintf(fp, "  -b <num>   Number of block offset bits.\n");
    fprintf(fp, "  -t <file>  Trace file.\n");
}
/** Unix-style error */
void unix_error(const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(ERROR_OUT, msg, args);
    va_end(args);
}

int parse_inst(char* ptr, op_t* op, add_t* addr, int* size) {
    if (ptr[0] != '\x20') {
        return -1;
    }
    ++ptr;
    switch (*ptr) {
        case 'L':
            *op = LOAD;
            break;
        case 'S':
            *op = STORE;
            break;
        case 'M':
            *op = MODIFY;
            break;
        default:
            break;
    }
    ++ptr;
    while (isspace(*ptr)) {
        ++ptr;
    }
    char* p_tmp = ptr;
    while (!(*p_tmp == ',')) {
        if ((*p_tmp >= 'a' && *p_tmp <= 'f') ||
            (*p_tmp >= 'A' && *p_tmp <= 'F') || isdigit(*p_tmp)) {
            ++p_tmp;
            continue;
        }
        /** invalid input address */
        return -1;
    }
    *p_tmp = '\0';
    *addr = strtol(ptr, &ptr, 16);
    assert(ptr == p_tmp);
    ++ptr;
    *size = atoi(ptr);
    return 0;
}

/**
 * @brief parse an address. The function use global variables `s`, `b`.
 *
 * @param addr the address to parse
 */
void parse_addr(add_t addr, int* tag, int* set_idx, int* offset) {
    *offset = addr & ~((~0u) << b);
    addr >>= b;
    *set_idx = addr & ~((~0u) << s);
    addr >>= s;
    *tag = addr;
}

void init_cache() {
    hits = misses = evictions = 0;
    cache = (set_t*)malloc(sizeof(set_t) * S);
    for (int i = 0; i < S; ++i) {
        /** cache[i] acts as a dump head */
        cache[i] = (line_t*)malloc(sizeof(line_t));
        line_t* t = cache[i];
        for (int k = 0; k < E; ++k) {
            t->next = (line_t*)malloc(sizeof(line_t));
            t = t->next;
            memset(t, 0, sizeof(line_t));
        }
    }
}

void destory_cache() {
    for (int i = 0; i < S; ++i) {
        line_t* t = cache[i];
        for (int k = 0; k < E; ++k) {
            line_t* tmp = t;
            t = t->next;
            free(tmp);
        }
        free(t);
    }
    free(cache);
}

/**
 * @brief Seek the x-th line in a set.
 *
 * @param t the (dump) head of the linked list
 * @param x index
 */
#define SEEK_LINE(t, x)          \
    for (int i = 0; i <= x; ++i) \
    t = t->next

/** Drag the idx-th line to the front */
void drag_line(line_t* head, int idx) {
    if (idx == 0) {
        return;
    }
    line_t* t = head;
    SEEK_LINE(t, idx - 1);
    line_t* tmp = head->next;
    head->next = t->next;
    t->next = head->next->next;
    head->next->next = tmp;
}

/**
 * @brief The basic unit function to simulate all operations.
 *
 * @param addr
 * @param size
 * @return the message to print.
 */
void load_unit(add_t addr, char** s) {
    int tag, set_idx, offset;
    int evicted = 0;
    parse_addr(addr, &tag, &set_idx, &offset);

    line_t* t = cache[set_idx]->next;
    int i = 0;
    while (t && !(t->valid && t->tag == tag)) {
        t = t->next;
        ++i;
    }
    if (t != NULL) {
        *s += sprintf(*s, "hit");
        hits++;
        drag_line(cache[set_idx], i);
        return;
    }
    /** Find the first invalid line */
    line_t* to_evict = cache[set_idx]->next;
    i = 0;
    while (to_evict && to_evict->valid) {
        to_evict = to_evict->next;
        ++i;
    }
    if (to_evict == NULL) {
        /** No invalid line, evict the last */
        to_evict = cache[set_idx];
        SEEK_LINE(to_evict, E - 1);
        i = E - 1;
        evicted = TRUE;
    }
    assert(to_evict);

    to_evict->valid = TRUE;
    to_evict->tag = tag;

    *s += sprintf(*s, "miss");
    misses++;
    if (evicted) {
        *s += sprintf(*s, " eviction");
        evictions++;
    }
    drag_line(cache[set_idx], i);
}

char buffer[30];

char* init_buffer(char op, add_t addr, int size) {
    int x = sprintf(buffer, "%c %lx,%d ", op, addr, size);
    return buffer + x;
}

void load(add_t addr, int size) {
    char* s = init_buffer('L', addr, size);
    load_unit(addr, &s);
    if (verbose)
        printf("%s\n", buffer);
}
void modify(add_t addr, int size) {
    char* s = init_buffer('M', addr, size);
    load_unit(addr, &s);
    *s = '\x20';
    s++;
    load_unit(addr, &s);
    if (verbose)
        printf("%s\n", buffer);
}
void store(add_t addr, int size) {
    char* s = init_buffer('S', addr, size);
    load_unit(addr, &s);
    if (verbose)
        printf("%s\n", buffer);
}