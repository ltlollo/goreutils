// musl-gcc self $cflags -o x

#include <err.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    tar,
    tgz,
    tbz,
    txz,
    zip,
    bz,
    bz2,
    sz,
    gz,
    xz,
    rar,
    z,
    deb,
    end
} exts;

typedef struct {
    char *cmd[4];
    const char *const ext[4];
} cmdmap;

static cmdmap map[] = {
    [tar] = {{"tar"    , "xf"   }, {".tar"                            }},
    [tgz] = {{"tar"    , "xvzf" }, {".tgz"    , ".tar.gz"             }},
    [tbz] = {{"tar"    , "xvjf" }, {".tar.bz2", ".tbz"   ,  ".tar.bz" }},
    [txz] = {{"tar"    , "xvJf" }, {".tar.xz"                         }},
    [gz ] = {{"gzip"   , "-d"   }, {".gz"                             }},
    [bz ] = {{"bunzip"          }, {".bz"                             }},
    [bz2] = {{"bunzip2"         }, {".bz2"                            }},
    [xz ] = {{"unlzma"          }, {".xz"                             }},
    [zip] = {{"unzip"           }, {".zip"                            }},
    [rar] = {{"unrar"  , "x"    }, {".rar"                            }},
    [sz ] = {{"7z"     , "x"    }, {".7z"                             }},
    [z  ] = {{"uncompress"      }, {".z"                              }},
    [deb] = {{"ar"     , "vx"   }, {".deb"                            }},
};

int
main(int argc, char *argv[]) {
    if (argc - 1 < 1) {
        errx(1, "not enough arguments");
    }
    char *fn = argv[1];
    size_t fnlen = strlen(fn);
    for (unsigned i = 0; i < end; ++i) {
        for (const char *const *ext = map[i].ext; *ext; ++ext) {
            size_t elen = strlen(*ext);
            if (elen > fnlen) {
                continue;
            }
            if (strcmp(fn + fnlen - elen, *ext) == 0) {
                char **cmd = map[i].cmd;
                while (*cmd) {
                    ++cmd;
                }
                *cmd = fn;
                execvp(*map[i].cmd,  map[i].cmd);
            }
        }
    }
    return 1;
}
