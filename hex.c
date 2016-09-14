// gcc self $cflags -lglut -lGL -lGLU -o hex


#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>
#include <GL/freeglut.h>

#ifndef __cplusplus
#define bool short
#define true 1
#define false 0
#define decltype(x) typeof(x)
#define constexpr const
#define atomic(x) _Alignas(64) x
#endif

#include <assert.h>

#define ADDR "0000000000000000"
#define COL " 00 00 00 00 00 00 00 00 |"
#define MODEL ADDR COL COL
#define MXLINES 160
#define MXCMD 63

#define KEY_ENTER 13
#define KEY_BACKSPACE 8
#define KEY_DELETE 127
#define KEY_ESCAPE 27

#define cxsize(x) (sizeof(x) / sizeof(*x))
#define cxlen(x) (cxsize(x) - 1)
#define min(x, y) ((x) > (y) ? (y) : (x))
#define access(x) (*(volatile decltype(x) *)&(x))
#define unused_attr __attribute__((unused))
#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x) __builtin_expect(!!(x), 1)

typedef struct { float x, y; } vec2;
typedef struct { float x, y, z; } vec3;

static const char* fname;
static void *font = GLUT_BITMAP_8_BY_13;
static atomic(int) winy, winx, nlines;
static unsigned char *beg, *end;
static atomic(unsigned char *) curs;
static size_t nvx;
static atomic(unsigned char) choff = 0;
static GLuint vs, fs, sp, vao, vbo[2];
static vec2 vrx[MXLINES * 96];
static vec3 col[MXLINES * 96];

static atomic(float) ix = cxlen(MODEL) + 16, iy;
static char cmdbuf[MXCMD + 1];
static char *cmdcurr = cmdbuf;
static atomic(bool) cmderr = 0;

static constexpr int xoff = 5, yoff = 13 * 2;
static constexpr int hfont = 13, unused_attr wfont = 8;
static constexpr float nch = cxlen(MODEL) + 2 * 16;
static constexpr float bg[3] = { 0.1, 0.1, 0.1 };
static const char *vsh = "\n#version 130"
                         "\nin vec2 inpos;"
                         "\nin vec3 incol;"
                         "\nout vec3 excol;"
                         "\nvoid main() {"
                         "\n    gl_Position = vec4(inpos, 0.0, 1.0);"
                         "\n    excol = incol;"
                         "\n}";
static const char *fsh = "\n#version 130"
                         "\nprecision highp float;"
                         "\nin vec3 excol;"
                         "\nout vec4 fragColor;"
                         "\nvoid main() {"
                         "\n    fragColor = vec4(excol, 1.0);"
                         "\n}";

static inline float nx(float);
static inline float ny(float);
static inline void set_vec2(vec2 *, float, float);
static inline void set_vec3(vec3 *, float, float, float);
static inline void *clamp(void *, void *, void *);
static inline unsigned char to_hex(unsigned char c);

static void resize(int, int);
static void setOrthographicProjection(void);
static void resetPerspectiveProjection(void);
static void draw_arr(float, float, unsigned char *, size_t);
static void draw_cstr(float, float, char *);
static unsigned char *format_buf(unsigned char *, unsigned char *);
static void check_shader(GLuint);
static void key_nav(int k, int, int);
static void key_ascii(unsigned char, int, int);
static void set_vrx_table(void);
static void set_col_table(unsigned char*);
static void display(void);
static void display_info(unsigned char charoff, unsigned);

static bool exec_cmd(unsigned char *);

static inline float
nx(float f) {
    return 2.0 * f - 1.0;
}
static inline float
ny(float f) {
    return 1.0 - f;
}

static inline void
set_vec2(vec2 *v, float x, float y) {
    v->x = x;
    v->y = y;
}

static inline void
set_vec3(vec3 *v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

static void
resize(int width, int height) {
    const float ar = (float)width / (float)height;
    winx = width;
    winy = height;
    nlines = winy / hfont - 1;
    iy = 2.0f / (float)(nlines - 3);
    if (nlines > MXLINES || nlines < 4) {
        errx(1, "too many lines");
    }
    nvx = nlines * 96;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void
setOrthographicProjection(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winx, 0, winy);
    glScalef(1, -1, 1);
    glTranslatef(0, -winy, 0);
    glMatrixMode(GL_MODELVIEW);
}

static void
resetPerspectiveProjection(void) {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void
draw_arr(float x, float y, unsigned char *str, size_t size) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < size; ++i) {
        glutBitmapCharacter(font, str[i]);
    }
}

static void
draw_cstr(float x, float y, char *str) {
    glRasterPos2f(x, y);
    while (*str) {
        glutBitmapCharacter(font, *str++);
    }
}

static inline unsigned char
to_hex(unsigned char c) {
    if (likely(c < 10)) {
        return c + '0';
    }
    return c - 10 + 'a';
}

static unsigned char *
format_buf(unsigned char *cur, unsigned char *buf) {
    size_t pos = cur - beg;
    for (int i = 0; i < 16; ++i) {
        *buf++ = to_hex((pos >> (16 - 1 - i) * 4) & 0xf);
    }
    *buf++ = ':';
    *buf++ = ' ';
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (cur < end) {
                *buf++ = to_hex(*cur >> 4);
                *buf++ = to_hex(*cur & 0xf);
            } else {
                *buf++ = ' ';
                *buf++ = ' ';
            }
            *buf++ = ' ';
            cur++;
        }
        *buf++ = '\x19';
        *buf++ = ' ';
    }
    return buf;
}

static void
check_shader(GLuint shader) {
    GLint len, res;
    char *log;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
    if (res == GL_FALSE) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        log = (char *)malloc(len);
        if (!log) {
            err(1, "malloc");
        }
        glGetShaderInfoLog(shader, len, &res, log);
        errx(1, "glErr: %s\n", log);
    }
}

static void
display_info(unsigned char coff, unsigned rpos) {
    static char buf[256] = { 0 };
    char err = unlikely(access(cmderr)) ? '!' : ' ';
    snprintf(buf, 256, "choff:%*d \x19 pos:%*d \x19 cmd:%c%14.*s \x19 "
                       "filename: %s",
             3, coff, 3, rpos, err, (int)(cmdcurr - cmdbuf), cmdbuf, fname);
    draw_cstr(xoff, yoff/2, buf);
}

static void
display(void) {
    static unsigned char buf[cxlen(MODEL) + 16] = { 0 };
    unsigned char *curr = access(curs), *slice = curr;
    unsigned char charoff = access(choff);
    unsigned rpos = 100 * (curr - beg) / (end - beg);

    set_col_table(slice);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * nvx, col, GL_STREAM_DRAW);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3d(0.7, 0.8, 0.6);
    setOrthographicProjection();
    glPushMatrix();
    glLoadIdentity();

    display_info(charoff, rpos);
    for (int i = 0, y = yoff; i < nlines && curr < end;
         ++i, y += hfont, curr += 16) {
        unsigned char *strend = format_buf(curr, buf);
        size_t rest = min(end - curr, 16);
        memcpy(strend, curr, rest);
        for (int j = 0; j < 16; ++j) {
            strend[j] += charoff;
        }
        for (int j = 0; j < 16; ++j) {
            if (unlikely(!strend[j])) {
                strend[j] = ' ';
            }
        }
        draw_arr(xoff, y, buf, cxlen(MODEL) + rest);
    }
    glPopMatrix();
    glDrawArrays(GL_TRIANGLES, 0, 96 * nch);
    resetPerspectiveProjection();
    glutSwapBuffers();
}

static void
key_nav(int k, int unused_attr f, int unused_attr s) {
    unsigned char *curr = access(curs);
    unsigned stp = nlines / 2 * 16;
    switch (k) {
    default:
        return;
    case GLUT_KEY_UP:
        access(curs) = clamp(curr - 16, beg, end);
        break;
    case GLUT_KEY_DOWN:
        access(curs) = clamp(curr + 16, beg, end);
        break;
    case GLUT_KEY_PAGE_UP:
        access(curs) = clamp(curr - stp, beg, end);
        break;
    case GLUT_KEY_PAGE_DOWN:
        access(curs) = clamp(curr + stp, beg, end);
        break;
    case GLUT_KEY_HOME:
        access(curs) = beg;
        break;
    case GLUT_KEY_END:
        access(curs) = end;
        break;
    }
    glutPostRedisplay();
}

static inline void *
clamp(void *ptr, void *llimit, void *rlimit) {
    if (unlikely(ptr < llimit)) {
        return llimit;
    } else if (unlikely(ptr > rlimit)) {
        return rlimit;
    }
    return ptr;
}

static bool exec_cmd(unsigned char * curr) {
    assert(cmdcurr < cmdbuf + sizeof(cmdbuf));
    *cmdcurr = '\0';
    char *ibeg;
    long long off = strtoll(cmdbuf, &ibeg, 16);
    bool valid_off = ibeg != cmdbuf;

    return false;

    switch (*ibeg++) {
    default: return false;
    case 'j':
        if (!valid_off) {
            if (*ibeg++ == 'e' && *ibeg++ == '\0') {
                access(curs) = end;
                return true;
            }
            return false;
        }
        switch (*ibeg++) {
        default:
            return false;
        case '\0':
            access(curs) = clamp(curr + off, beg, end);
            return true;
        case 'a':
            if (*ibeg != '\0') {
                return false;
            }
            access(curs) = clamp(beg + off, beg, end);
            return true;
        }
        break;
    case 'e':
        break;
    }
    return false;
}

static void
key_ascii(unsigned char k, int unused_attr f, int unused_attr s) {
    unsigned char *curr = access(curs);
    switch (k) {
    default:
        if (unlikely(cmdcurr == cmdbuf + MXCMD)) {
            return;
        }
        *cmdcurr++ = k;
        break;
    case 'q':
        exit(0);
    case 'o':
        access(choff) = choff + 1;
        break;
    case 'p':
        access(choff) = choff - 1;
        break;
    case '>':
        access(curs) = clamp(curr - 1, beg, end);
        break;
    case '<':
        access(curs) = clamp(curr + 1, beg, end);
        break;
    case KEY_ESCAPE:
        cmdcurr = cmdbuf;
        access(cmderr) = 0;
        break;
    case KEY_ENTER:
        access(cmderr) = !exec_cmd(curr);
        break;
    case KEY_BACKSPACE:
    case KEY_DELETE:
        if (unlikely(--cmdcurr < cmdbuf)) {
            cmdcurr = cmdbuf;
        }
        break;
    }
    glutPostRedisplay();
}

static void
set_col_table(unsigned char *slice) {
    int i = 0;
    for (; i < nlines && slice < end; ++i, slice += 16) {
        for (int j = 0; j < 16; ++j) {
            float rc = (slice[j] >> 6);
            float bc = (slice[j] >> 5) & 1;
            float gc = (slice[j] >> 0) & 31;
            size_t o = j * 6 + i * 96;
            set_vec3(&col[0 + o], rc / 3.f, gc / 31.f, bc / 2.f);
            set_vec3(&col[1 + o], rc / 3.f, gc / 31.f, bc / 2.f);
            set_vec3(&col[2 + o], rc / 3.f, gc / 31.f, bc / 2.f);
            set_vec3(&col[3 + o], rc / 3.f, gc / 31.f, bc / 2.f);
            set_vec3(&col[4 + o], rc / 3.f, gc / 31.f, bc / 2.f);
            set_vec3(&col[5 + o], rc / 3.f, gc / 31.f, bc / 2.f);
        }
    }
    for (; i < nlines; ++i) {
        for (int j = 0; j < 16; ++j) {
            size_t o = j * 6 + i * 96;
            set_vec3(&col[0 + o], bg[0], bg[1], bg[2]);
            set_vec3(&col[1 + o], bg[0], bg[1], bg[2]);
            set_vec3(&col[2 + o], bg[0], bg[1], bg[2]);
            set_vec3(&col[3 + o], bg[0], bg[1], bg[2]);
            set_vec3(&col[4 + o], bg[0], bg[1], bg[2]);
            set_vec3(&col[5 + o], bg[0], bg[1], bg[2]);
        }
    }

}

static void
set_vrx_table(void) {
    for (int j = 0; j < nlines; ++j) {
        for (int i = 0; i < 16; ++i) {
            size_t o = i * 6 + j * 96;
            set_vec2(&vrx[0 + o], nx((ix + i + 0) / nch), ny(iy * (j + 1)));
            set_vec2(&vrx[1 + o], nx((ix + i + 1) / nch), ny(iy * (j + 1)));
            set_vec2(&vrx[2 + o], nx((ix + i + 1) / nch), ny(iy * (j + 2)));
            set_vec2(&vrx[3 + o], nx((ix + i + 1) / nch), ny(iy * (j + 2)));
            set_vec2(&vrx[4 + o], nx((ix + i + 0) / nch), ny(iy * (j + 2)));
            set_vec2(&vrx[5 + o], nx((ix + i + 0) / nch), ny(iy * (j + 1)));
        }
    }
}

int
main(int argc, char *argv[]) {
    if (argc - 1 != 1) {
        errx(1, "not enough arguments");
    }
    int fd = open((fname = argv[1]), O_RDONLY);
    if (fd == -1) {
        err(1, "open");
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        err(1, "fstat");
    }
    size_t len = sb.st_size;
    if ((beg = mmap(NULL, len, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_POPULATE, fd, 0)) == MAP_FAILED) {
        err(1, "mmap");
    }
    curs = beg;
    end = beg + len;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow(*argv);
    winy = glutGet(GLUT_SCREEN_HEIGHT);
    winx = glutGet(GLUT_SCREEN_WIDTH);

    nlines = winy / hfont - 1;
    if (nlines > MXLINES || nlines < 4) {
        errx(1, "too many lines");
    }
    nvx = nlines * 96;
    iy = 2.0f / (float)(nlines - 3);

    set_vrx_table();
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * nvx, vrx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * nvx, col, GL_STREAM_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsh, NULL);
    glCompileShader(vs);
    check_shader(vs);

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsh, NULL);
    glCompileShader(fs);
    check_shader(fs);

    sp = glCreateProgram();
    glAttachShader(sp, vs);
    glAttachShader(sp, fs);
    glBindAttribLocation(sp, 0, "inpos");
    glBindAttribLocation(sp, 1, "incol");
    glLinkProgram(sp);
    check_shader(sp);
    glUseProgram(sp);

    glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glClearColor(bg[0], bg[1], bg[2], 1.0f);
    display();
    glutKeyboardFunc(key_ascii);
    glutSpecialFunc(key_nav);
    glutMainLoop();
    return 0;
}
