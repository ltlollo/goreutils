// gpp self $cppflags

#include <dirent.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static void show(size_t, char *);

int
main(int argc, char *argv[]) {
    char *dn = argc - 1 ? argv[1] : (char *)".";
    show(0, dn);
    return 0;
}

static void
show(size_t depth, char *dn) {
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
    } while (en0 && en0->d_name[0] == '.');
    if (en0 == NULL) {
        (void)closedir(dir);
        return;
	}
	do {
        do {
			en1 = readdir(dir);
		} while (en1 && en1->d_name[0] == '.');
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
            show(depth + 1, subdn);
		}
    } while ((en0 = en1));
	(void)closedir(dir);
}

