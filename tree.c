// gpp self $cppflags

#include <dirent.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static void show(size_t, char *, int);
static int filt(const char *, int);

int
main(int argc, char *argv[]) {
    char *dn = argc - 1 > 0 ? argv[1] : (char *)".";
    int hide = argc - 2 > 0 ? strcmp(argv[2], "-h") : 1;
    (void)printf("%s\n", dn);
    show(0, dn, hide);
    return 0;
}

static void
show(size_t depth, char *dn, int hide) {
    static char *dnbuf = NULL;
    struct dirent *en0, *en1;
    DIR *dir;
    char *subdn;
	size_t dnl, snl, i;

    if ((dir = opendir(dn)) == NULL) {
        warn("opendir %s", dn);
		return;
	}
    do {
        en0 = readdir(dir);
    } while (en0 && filt(en0->d_name, hide));
    if (en0 == NULL) {
        (void)closedir(dir);
        return;
	}
	do {
        do {
			en1 = readdir(dir);
		} while (en1 && filt(en1->d_name, hide));
        for (i = 0; i < depth; ++i) {
            (void)printf("│   ");
        }
        (void)printf("%s───%s\n", en1 ? "├" : "└", en0->d_name);
        if (en0->d_type == DT_DIR) {
			dnl = strlen(dn), snl = strlen(en0->d_name);
            if ((subdn = (char *)realloc(dnbuf, dnl + snl + 2)) == NULL) {
                err(1, "realloc");
			}
            (void)sprintf(subdn, "%s/%s", dn, en0->d_name);
            show(depth + 1, subdn, hide);
		}
    } while ((en0 = en1));
	(void)closedir(dir);
}

static int
filt(const char *s, int hide) {
	if (hide) {
		return *s == '.';
	}
    if (*s == '.' && *(s + 1) == '\0') {
        return 1;
	}
    if (*s == '.' && *(s + 1) == '.' && *(s + 2) == '\0') {
        return 1;
	}
	return 0;
}
