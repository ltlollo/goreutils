// gcc self $cfalgs -lm $(pkg-config --libs ncursesw) -o scalc

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PACKED __attribute__((packed))

#define NUM_REG 16
#define REG_RNGBUF_SIZE 8
_Static_assert(NUM_REG, "register array too small");
_Static_assert(NUM_REG >= REG_RNGBUF_SIZE, "register ring buffer too big");

#define HIST_SIZE 2000
#define HIST_RNGBUF_SIZE 16
_Static_assert(HIST_SIZE, "history array too small");
_Static_assert(HIST_SIZE >= HIST_RNGBUF_SIZE, "history ring buffer too big");

#define USRIN_SIZE 80
#define CURS_RELOAD -1

#define KEY_CTRL_LEFT 545
#define KEY_CTRL_RIGHT 560

#define COL_LIGHT_GREEN 144
#define COL_PURE_WHITE 255
#define COL_GREEN 107
#define COL_ORANGE 173
#define COL_WHITE 252
#define COL_GRAY 102
#define COL_PURPLE 96
#define COL_DARK_GRAY 236
#define COL_XDARK_GRAY 234
#define COL_YELLOW 221
#define COL_PINK 161

#define HIST_FNAME ".scalc_history"

typedef enum COL {
    COL_BEG = 1,
    COL_FUCK_YOU_ONE_BASED_INDEX = COL_BEG,
    COL_0 = COL_BEG,
    COL_1,
    COL_2,
    COL_3,
    COL_4,
    COL_5,
    COL_6,
    COL_7,
    COL_8,
    COL_END,
    COL_ALT_BEG = COL_END,
    COL_ALT_0 = COL_ALT_BEG,
    COL_ALT_1,
    COL_ALT_2,
    COL_ALT_3,
    COL_ALT_4,
    COL_ALT_5,
    COL_ALT_6,
    COL_ALT_7,
    COL_ALT_8,
    COL_ALT_END,
    COL_BG_BEG = COL_ALT_END,
    COL_BG_0 = COL_BG_BEG,
    COL_BG_END,
    COL_BG_ALT_BEG = COL_BG_END,
    COL_BG_ALT_0 = COL_BG_ALT_BEG,
    COL_BG_ALT_END
} COL;

typedef enum OP {
    OP_INVALID = -1,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_SHL,
    OP_SHR,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_POW,
} OP;

typedef enum MODE {
    MODE_INT,
    MODE_UINT,
    MODE_F64,
    MODE_F32,
    MODE_TOT,
} MODE;

typedef union Reg {
    uint64_t u64;
    int64_t i64;
    double f64;
    void *ptr;
    struct PACKED {
        union {
            float f32;
            uint32_t u32;
            int32_t i32;
        };
        uint64_t rest : 32;
    } half;
    struct PACKED {
        union {
            uint16_t u16;
            int16_t i16;
        };
        uint64_t rest : 48;
    } hhalf;
    struct PACKED {
        union {
            uint8_t u8;
            int8_t i8;
        };
        uint64_t rest : 56;
    } hhhalf;
} Reg;
_Static_assert(sizeof(Reg) == 8, "unsupported architecture");

typedef struct Cell {
    enum TYPE { TYPE_OP, TYPE_NUM } type;
    union {
        OP op;
        Reg num;
    };
} Cell;

static const int color[] = {
            [COL_0] = COL_GRAY,      [COL_ALT_0] = COL_PURE_WHITE,
            [COL_1] = COL_GRAY,      [COL_ALT_1] = COL_ORANGE,
            [COL_2] = COL_GRAY,      [COL_ALT_2] = COL_YELLOW,
            [COL_3] = COL_GRAY,      [COL_ALT_3] = COL_LIGHT_GREEN,
            [COL_4] = COL_GRAY,      [COL_ALT_4] = COL_GREEN,
            [COL_6] = COL_DARK_GRAY, [COL_ALT_6] = COL_DARK_GRAY,
            [COL_5] = COL_WHITE,     [COL_ALT_5] = COL_PINK,
            [COL_7] = COL_GRAY,      [COL_ALT_7] = COL_PURE_WHITE,
            [COL_8] = COL_PURPLE,    [COL_ALT_8] = COL_GREEN,
};
static const int color_bg[] = {
            [COL_BG_0] = COL_XDARK_GRAY, [COL_BG_ALT_0] = COL_XDARK_GRAY,
};
static const char *shex = "0123456789abcdef";
static const char *smode[] = {
            [MODE_INT] = "int", [MODE_UINT] = "uint", [MODE_F32] = "f32",
            [MODE_F64] = "f64",
};
static unsigned currreg = 0;
static unsigned currcmd = 0;
static int cursor = 0;
static int mode_usecmd = 0;
static MODE mode = MODE_INT;
static union Reg reg[NUM_REG] = { 0 };
static char hist[HIST_SIZE][USRIN_SIZE + 1] = { 0 };
static char usrin[USRIN_SIZE + 1] = { 0 };
static char dbgmsg[USRIN_SIZE + 1] = { 0 };
static FILE *hist_file = NULL;

extern const char *__progname;

static void printc(int, const char *, ...);
static void render_regs(void);
static void render_hist(void);
static void render_cmd(void);
static void render_dbg(void);
static void renderscr(void);
static enum OP parse_op(const char *);
static int parse_num(const char *, Reg *);
static int parse_cell(const char *, struct Cell *);
static void sigerr(const char *, ...);
static union Reg eval_op(const OP, const Reg, const Reg);
static int eval_stmt(char *);
static void insert_spaces(const char *, char *);
static int eval(const char *);
static void initcalc(void);
static int find_prev_simil(const char *, int, int (*)(int));
static int find_next_simil(const char *, int, int (*)(int));

static void
printc(int col, const char *fmt, ...) {
    va_list vl;

    attron(COLOR_PAIR(col));
    va_start(vl, fmt);
    vwprintw(stdscr, fmt, vl);
}

static void
render_regs(void) {
    for (int i = 0; i < REG_RNGBUF_SIZE; ++i) {
        int color_off = i == 2 ? COL_ALT_BEG : COL_BEG;
        int pos = ((i + currreg - 2) % NUM_REG);
        union Reg *regp = reg + pos;
        Reg creg = *regp;
        uint64_t ull = creg.u64;

        printc(color_off + 8, "[");
        printc(color_off + 0, "%02x", pos);
        printc(color_off + 8, "]");
        printc(color_off + 4, "bin");
        printc(color_off + 0, " 0b");

        if (mode == MODE_INT || mode == MODE_UINT) {
            for (int i = 0; i < 64; ++i) {
                if (i % 16 == 0) {
                    attron(COLOR_PAIR(color_off + i / 16));
                }
                addch('0' + ((ull >> (63 - i)) & 0x1));
            }
        } else if (mode == MODE_F32) {
            int j = 0;

            attron(COLOR_PAIR(6 + color_off));
            for (; j < 32; ++j) {
                addch('0' + ((ull >> (63 - j)) & 0x1));
            }
            attron(COLOR_PAIR(0 + color_off));
            addch('0' + ((ull >> (63 - j++)) & 0x1));
            attron(COLOR_PAIR(1 + color_off));
            for (int k = j + 8; j < k; ++j) {
                addch('0' + ((ull >> (63 - j)) & 0x1));
            }
            attron(COLOR_PAIR(2 + color_off));
            for (int k = j + 23; j < k; ++j) {
                addch('0' + ((ull >> (63 - j)) & 0x1));
            }
        } else if (mode == MODE_F64) {
            int j = 0;

            attron(COLOR_PAIR(0 + color_off));
            addch('0' + ((ull >> (63 - j++)) & 0x1));
            attron(COLOR_PAIR(1 + color_off));
            for (int k = j + 11; j < k; ++j) {
                addch('0' + ((ull >> (63 - j)) & 0x1));
            }
            attron(COLOR_PAIR(2 + color_off));
            for (int k = j + 52; j < k; ++j) {
                addch('0' + ((ull >> (63 - j)) & 0x1));
            }
        }
        printc(color_off + 0, "\n ├──");
        printc(color_off + 4, "hex");
        printc(color_off + 0, " 0x");
        for (int i = 0; i < 16; ++i) {
            if (i % 4 == 0) {
                attron(COLOR_PAIR(color_off + i / 4));
            }
            addch(shex[(ull >> (4 * (15 - i))) & 0xf]);
        }
        if (mode == MODE_INT) {
            printc(color_off + 0, "\n └──");
            printc(color_off + 4, "i8 ");
            printc(color_off + 0, "%s%4hhd  ", creg.hhhalf.rest ? ".." : "  ",
                   creg.hhhalf.i8);
            printc(color_off + 4, "i16 ");
            printc(color_off + 0, "%s%6hd  ", creg.hhalf.rest ? ".." : "  ",
                   creg.hhalf.i16);
            printc(color_off + 4, "i32 ");
            printc(color_off + 0, "%s%11hd  ", creg.half.rest ? ".." : "  ",
                   creg.half.i32);
            printc(color_off + 4, "i64 ");
            printc(color_off + 5, "%20lld\n", creg.i64);
        } else if (mode == MODE_UINT) {
            printc(color_off + 0, "\n └──");
            printc(color_off + 4, "u8 ");
            printc(color_off + 0, "%s%4hhu  ", creg.hhhalf.rest ? ".." : "  ",
                   creg.hhhalf.u8);
            printc(color_off + 4, "u16 ");
            printc(color_off + 0, "%s%6hu  ", creg.hhalf.rest ? ".." : "  ",
                   creg.hhalf.u16);
            printc(color_off + 4, "u32 ");
            printc(color_off + 0, "%s%11hu  ", creg.half.rest ? ".." : "  ",
                   creg.half.u32);
            printc(color_off + 4, "u64 ");
            printc(color_off + 5, "%20llu\n", creg.u64);
        } else if (mode == MODE_F64) {
            printc(color_off + 0, "\n └──");
            printc(color_off + 4, "f64 ");
            printc(color_off + 5, "%.40f\n", creg.f64);
        } else if (mode == MODE_F32) {
            printc(color_off + 0, "\n └──");
            printc(color_off + 4, "f32 ");
            printc(color_off + 5, "%.20f\n", creg.half.f32);
        }
    }
}

static void
render_hist(void) {
    for (int i = 0; i < HIST_RNGBUF_SIZE; ++i) {
        int pos = (HIST_RNGBUF_SIZE + currcmd - i) % HIST_SIZE;
        printc(COL_BG_ALT_BEG + 0, " ");
        printc(COL_BEG + 8, "%s\n", hist[pos]);
    }
}

static void
render_cmd(void) {
    int len, x, y;

    if (mode_usecmd != 0 && cursor < 0) {
        memcpy(usrin, hist[currcmd % HIST_SIZE], USRIN_SIZE);
    }
    printc(COL_BG_BEG + 0, " ");
    printc(COL_ALT_BEG + 7, "%s", usrin);
    if (cursor > (len = (int)strlen(usrin)) || cursor < 0) {
        cursor = len;
    }
    getyx(stdscr, y, x);
    move(y, cursor + 1);
    (void)x;
}

static void
render_dbg(void) {
    int x, y, ymax, xmax;

    getyx(stdscr, y, x);
    getmaxyx(stdscr, ymax, xmax);

    move(ymax - 1, 0);

    printc(COL_ALT_BEG + 7, "%s", dbgmsg);

    move(y, x);
    (void)xmax;
}

static void
render_title(void) {
    printc(COL_ALT_BEG + 8, "[");
    printc(COL_ALT_BEG + 0, "%s", __progname);
    printc(COL_ALT_BEG + 8, "]");
    printc(COL_ALT_BEG + 8, " register mode: ");
    printc(COL_ALT_BEG + 2, "%s", smode[mode]);
    printc(COL_ALT_BEG + 8, " history mode: ");
    printc(COL_ALT_BEG + 2, "%s\n", mode_usecmd ? "clobber" : "noclobber");
}

static void
renderscr(void) {
    clear();
    render_title();
    render_regs();
    render_hist();
    render_cmd();
    render_dbg();
}

static enum OP
parse_op(const char *str) {
    long long len = strlen(str);
    if (len == 0) {
        return OP_INVALID;
    }
    if (len == 1) {
        switch (*str) {
        default:
            return OP_INVALID;
        case '+':
            return OP_ADD;
        case '-':
            return OP_SUB;
        case '*':
            return OP_MUL;
        case '%':
            return OP_MOD;
        case '/':
            return OP_DIV;
        case '&':
            return OP_AND;
        case '^':
            return OP_XOR;
        case '|':
            return OP_OR;
        }
    }
    if (strcmp(str, ">>") == 0) {
        return OP_SHR;
    } else if (strcmp(str, "<<") == 0) {
        return OP_SHL;
    } else if (strcmp(str, "**") == 0) {
        return OP_POW;
    }
    return OP_INVALID;
}

static int
parse_num(const char *str, Reg *res) {
    long len = strlen(str);
    int neg = 0;
    Reg tmp;
    char *end;

    if (len == 0) {
        return -1;
    }
    if (str[len] == '-') {
        neg = 1;
        ++str;
        if (--len == 0) {
            return -1;
        }
    }
    if (mode == MODE_INT || mode == MODE_UINT) {
        tmp.u64 = strtoll(str, &end, 0);
        if (end - str != len || errno == ERANGE) {
            return -1;
        }
        if (neg == 0) {
            *res = tmp;
        } else {
            res->i64 = -tmp.i64;
        }
    } else if (mode == MODE_F32 || mode == MODE_F64) {
        tmp.f64 = strtod(str, &end);
        if (end - str != len) {
            sigerr("error: junk in token '%s'", str);
            return -1;
        }
        if (errno == ERANGE) {
            sigerr("error: number out of range in token '%s'", str);
            return -1;
        }
        if (mode == MODE_F64) {
            if (neg == 0) {
                *res = tmp;
            } else {
                res->f64 = -tmp.f64;
            }
        } else {
            if (neg == 0) {
                res->half.f32 = tmp.f64;
            } else {
                res->half.f32 = -tmp.f64;
            }
        }
    }
    return 0;
}

static int
parse_cell(const char *str, struct Cell *res) {

    if ((res->op = parse_op(str)) != OP_INVALID) {
        res->type = TYPE_OP;
        return 0;
    }
    if (parse_num(str, &res->num) != -1) {
        res->type = TYPE_NUM;
        return 0;
    }
    return -1;
}

static void
sigerr(const char *fmt, ...) {
    va_list vl;

    va_start(vl, fmt);
    vsnprintf(dbgmsg, USRIN_SIZE, fmt, vl);
}

static union Reg
eval_op(const OP op, const Reg f, const Reg s) {
    Reg res;

    switch (op) {
    default:
        sigerr("error: unimplemented operation");
    case OP_ADD:
        if (mode == MODE_INT || mode == MODE_UINT) {
            res.u64 = f.u64 + s.u64;
        } else if (mode == MODE_F64) {
            res.f64 = f.f64 + s.f64;
        } else {
            res.half.f32 = f.half.f32 + s.half.f32;
        }
        break;
    case OP_SUB:
        if (mode == MODE_INT || mode == MODE_UINT) {
            res.u64 = f.u64 - s.u64;
        } else if (mode == MODE_F64) {
            res.f64 = f.f64 - s.f64;
        } else {
            res.half.f32 = f.half.f32 - s.half.f32;
        }
        break;
    case OP_MOD:
        if (mode == MODE_INT || mode == MODE_UINT) {
            res.u64 = f.u64 % s.u64;
        } else {
            sigerr("error: mod operation on non integer type");
            break;
        }
        break;
    case OP_AND:
        if (mode == MODE_INT || mode == MODE_UINT) {
            res.u64 = f.u64 & s.u64;
        } else {
            sigerr("error: mod operation on non integer type");
            break;
        }
        break;
    case OP_OR:
        if (mode == MODE_INT || mode == MODE_UINT) {
            res.u64 = f.u64 | s.u64;
        } else {
            sigerr("error: mod operation on non integer type");
            break;
        }
        break;
    case OP_XOR:
        if (mode == MODE_INT || mode == MODE_UINT) {
            res.u64 = f.u64 ^ s.u64;
        } else {
            sigerr("error: mod operation on non integer type");
            break;
        }
        break;
    case OP_MUL:
        if (mode == MODE_UINT) {
            res.u64 = f.u64 * s.u64;
        } else if (mode == MODE_INT) {
            res.i64 = f.i64 * s.i64;
        } else if (mode == MODE_F64) {
            res.f64 = f.f64 * s.f64;
        } else {
            res.half.f32 = f.half.f32 * s.half.f32;
        }
        break;
    case OP_DIV:
        if (mode == MODE_UINT) {
            res.u64 = f.u64 / s.u64;
        } else if (mode == MODE_INT) {
            res.i64 = f.i64 / s.i64;
        } else if (mode == MODE_F64) {
            res.f64 = f.f64 / s.f64;
        } else {
            res.half.f32 = f.half.f32 / s.half.f32;
        }
        break;
    case OP_SHR:
        if (mode == MODE_UINT) {
            res.u64 = f.u64 >> s.u64;
        } else if (mode == MODE_INT) {
            res.i64 = f.i64 >> s.i64;
        } else {
            sigerr("error: shift operation on non integer type");
            break;
        }
        break;
    case OP_SHL:
        if (mode == MODE_UINT) {
            res.u64 = f.u64 << s.u64;
        } else if (mode == MODE_INT) {
            res.i64 = f.i64 << s.i64;
        } else {
            sigerr("error: shift operation on non integer type");
            break;
        }
        break;
    case OP_POW:
        if (mode == MODE_UINT) {
            res.u64 = 1;
            for (uint64_t i = 0; i < s.u64; ++i) {
                res.u64 *= f.u64;
            }
        } else if (mode == MODE_INT) {
            if (res.i64 < 0) {
                sigerr("error: right operand cannot be negative");
                break;
            }
            res.i64 = 1;
            for (uint64_t i = 0; i < s.u64; ++i) {
                res.i64 *= f.i64;
            }
        } else if (mode == MODE_F64) {
            res.f64 = pow(f.f64, s.f64);
        } else if (mode == MODE_F32) {
            res.half.f32 = powf(f.half.f32, s.half.f32);
        }
        break;
    }

    return res;
}

static int
eval_stmt(char *cmd) {
    const char *cell[3];
    struct Cell res[3];
    int nargs;

    if ((cell[0] = strtok(cmd, " ")) == NULL) {
        return 0;
    }
    cell[1] = strtok(NULL, " ");
    cell[2] = strtok(NULL, " ");

    if (strtok(NULL, " ") != NULL) {
        sigerr("error: too many tokens in statement");
        return -1;
    }

    if (cell[2]) {
        nargs = 3;
    } else if (cell[1]) {
        nargs = 2;
    } else {
        nargs = 1;
    }
    for (int i = 0; i < nargs; ++i) {
        if (parse_cell(cell[i], res + i) == -1) {
            sigerr("error: could not parse argument %d in mode '%s': '%s'",
                   i + 1, smode[mode], cell[i]);
            return -1;
        }
    }
    if (nargs == 3 && res[2].type == TYPE_OP) {
        sigerr("error: argument 3 is an operator");
        return -1;
    }
    if (nargs > 1 && res[0].type == TYPE_OP && res[1].type == TYPE_OP) {
        sigerr("error: multiple operators per statement");
        return -1;
    }
    if (nargs == 2 && res[0].type != TYPE_OP && res[1].type != TYPE_OP) {
        sigerr("error: multiple constant and no operator");
        return -1;
    }

    Reg operands[2];
    OP op;

    if (nargs == 1) {
        if (res[0].type == TYPE_NUM) {
            reg[(currreg + 1) % NUM_REG] = res[0].num;
            return 0;
        }
        operands[0] = reg[(currreg - 1) % NUM_REG];
        operands[1] = reg[(currreg - 0) % NUM_REG];
        op = res[0].op;
    } else if (nargs == 2) {
        if (res[0].type == TYPE_NUM) {
            operands[0] = res[0].num;
            operands[1] = reg[currreg % NUM_REG];
            op = res[1].op;
        } else {
            operands[0] = reg[currreg % NUM_REG];
            operands[1] = res[1].num;
            op = res[0].op;
        }
    } else {
        operands[0] = res[0].num;
        operands[1] = res[2].num;
        op = res[1].op;
    }
    reg[(currreg + 1) % NUM_REG] = eval_op(op, operands[0], operands[1]);
    return 0;
}

static void
insert_spaces(const char *from, char *to) {
    long long len = strlen(from);
    const char *end = from + len;

    while (from < end) {
        if (ispunct(*from)) {
            if (*from == '-') {
                // '-' ambiguous syntax
                *to++ = *from++;
            } else {
                while (*from != '-' && ispunct(*from)) {
                    *to++ = *from++;
                }
                *to++ = ' ';
            }
        } else {
            while (*from && !ispunct(*from)) {
                *to++ = *from++;
            }
            *to++ = ' ';
        }
    }
    *to = '\0';
}

static int
eval(const char *usrin) {
    static char cmd[2 * USRIN_SIZE + 1];
    char *tok, *save;
    long long len = strlen(usrin);

    if (len == 0) {
        return -1;
    } else if (len == 1 && *usrin == ';') {
        ++currreg;
        return -1;
    }

    insert_spaces(usrin, cmd);

    tok = strtok_r(cmd, ";", &save);

    while (tok) {
        if (eval_stmt(tok) == -1) {
            return -1;
        }
        currreg++;
        tok = strtok_r(NULL, ";", &save);
    }
    return 0;
}

static void
save_hist(void) {
    if ((hist_file = freopen(HIST_FNAME, "w+", hist_file)) == NULL) {
        return;
    }
    for (int i = 0; i < HIST_SIZE; ++i) {
        fprintf(hist_file, "%s\n", hist[(currcmd + i + 1) % HIST_SIZE]);
    }
}

static void
load_hist(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;

    if ((hist_file = fopen(HIST_FNAME, "r")) == NULL) {
        if ((hist_file = fopen(HIST_FNAME, "w+")) == NULL) {
            sigerr("error: loading '%s'", HIST_FNAME);
            return;
        }
    }
    while (i < HIST_SIZE && (read = getline(&line, &len, hist_file)) != -1) {
        if (read > USRIN_SIZE) {
            ++i;
            continue;
        } else {
            memcpy(hist[i % HIST_SIZE], line, read - 1);
            ++i;
        }
    }
    free(line);
}

static void
initcalc(void) {
    atexit(save_hist);
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    intrflush(stdscr, FALSE);
    start_color();
    use_default_colors();

    for (int i = COL_BEG; i < COL_ALT_END; ++i) {
        init_pair(i, color[i], -1);
    }
    for (int i = COL_BG_BEG; i < COL_BG_ALT_END; ++i) {
        init_pair(i, -1, color_bg[i]);
    }
    load_hist();
    --currcmd;
}

static int
find_prev_simil(const char *usrin, int cursor, int (*f)(int)) {
    while (cursor && f(usrin[cursor - 1])) {
        --cursor;
    }
    return cursor;
}

static int
find_next_simil(const char *usrin, int cursor, int (*f)(int)) {
    while (usrin[cursor] != '\0' && f(usrin[cursor])) {
        cursor++;
    }
    return cursor;
}

int
main(void) {
    int run = 1;
    int c, tmp;

    initcalc();

    while (run) {
        renderscr();
        switch (c = getch()) {
        case '?':
            sigerr("?: 'm' to change register mode, TAB to change history "
                   "mode");
            break;
        case 'Q':
        case 'q':
            run = 0;
            break;
        case KEY_UP:
            if (mode_usecmd) {
                cursor = CURS_RELOAD;
            }
            currcmd++;
            break;
        case '\n':
            dbgmsg[0] = '\0';
            if (eval(usrin) == -1) {
                break;
            }
            memcpy(hist[currcmd % HIST_SIZE], usrin, USRIN_SIZE);
            currcmd--;
            if (mode_usecmd) {
                cursor = CURS_RELOAD;
            } else {
                *usrin = 0;
                cursor = 0;
            }
            break;
        case KEY_DOWN:
            if (mode_usecmd) {
                cursor = CURS_RELOAD;
            }
            currcmd--;
            break;
        case KEY_LEFT:
            if (cursor > 0) {
                cursor--;
            }
            break;
        case KEY_RIGHT:
            if (cursor < (int)strlen(usrin)) {
                cursor++;
            }
            break;
        case '\t':
            cursor = CURS_RELOAD;
            mode_usecmd = !mode_usecmd;
            if (mode_usecmd == 0) {
                usrin[0] = '\0';
            }
            break;
        case 'm':
            mode = (mode + 1) % MODE_TOT;
            break;
        case KEY_DC:
            if (cursor == 0) {
                break;
            }
            memmove(usrin + cursor, usrin + cursor + 1, USRIN_SIZE - cursor);
            break;
        case KEY_BACKSPACE:
            if (cursor == 0) {
                break;
            }
            cursor--;
            memmove(usrin + cursor, usrin + cursor + 1, USRIN_SIZE - cursor);
            break;
        case 23:
            if (strcmp(keyname(c), "^W") == 0) {
                if (cursor == 0) {
                    break;
                }
                tmp = cursor;
                if (isspace(usrin[cursor - 1])) {
                    // del sapces
                    cursor = find_prev_simil(usrin, cursor, isspace);
                    memmove(usrin + cursor, usrin + tmp, USRIN_SIZE - tmp);
                } else if (ispunct(usrin[cursor - 1])) {
                    // del symbols
                    memmove(usrin + --cursor, usrin + tmp, USRIN_SIZE - tmp);
                } else if (isalnum(usrin[cursor - 1])) {
                    // del vars
                    cursor = find_prev_simil(usrin, cursor, isalnum);
                    memmove(usrin + cursor, usrin + tmp, USRIN_SIZE - tmp);
                }
            }
            break;
        case KEY_CTRL_LEFT:
            if (cursor == 0) {
                break;
            }
            if (isspace(usrin[cursor - 1])) {
                cursor = find_prev_simil(usrin, cursor, isspace);
            } else if (ispunct(usrin[cursor - 1])) {
                cursor--;
            } else if (isalnum(usrin[cursor - 1])) {
                cursor = find_prev_simil(usrin, cursor, isalnum);
            }
            break;
        case KEY_CTRL_RIGHT:
            if (usrin[cursor] == '\0') {
                break;
            }
            if (isspace(usrin[cursor])) {
                cursor = find_next_simil(usrin, cursor, isspace);
            } else if (ispunct(usrin[cursor])) {
                cursor++;
            } else if (isalnum(usrin[cursor])) {
                cursor = find_next_simil(usrin, cursor, isalnum);
            }
            break;
        default:
            if (isprint(c) == 0) {
                break;
            }
            if (cursor >= USRIN_SIZE) {
                break;
            }
            memmove(usrin + cursor + 1, usrin + cursor, USRIN_SIZE - cursor);
            usrin[cursor++] = (char)c;
        }
    }
    endwin();
    return 0;
}

