#ifndef STR_SET_H
#define STR_SET_H

#include <stddef.h> // size_t, ptrdiff_t

typedef struct {
	size_t raw_size, raw_alloc;
	char *raw;
	size_t arr_size, arr_alloc;
	char **arr;
} StrSet;

#define UNUSED __attribute__((unused))
#ifdef STATIC
#define DECL static UNUSED
#else
#define DECL extern "C"
#endif

#define IFAIL ((char *)NULL - 1)

DECL int init_strset_size_avg(StrSet *, size_t, size_t);
DECL int init_strset_size(StrSet *, size_t);
DECL int init_strset(StrSet *);
DECL void free_strset(StrSet *);
DECL char *insert_strset(StrSet *, const char *);
DECL void offset_strset_tokv(StrSet *, char **, size_t);
DECL void freeze_strset(StrSet *);

#endif // STR_SET_H

#ifdef STR_SET_IMPL

#include <assert.h> // assert
#include <stdlib.h> // malloc, realloc, free
#include <string.h> // strcmp, strlen, memcpy, memmove

static inline int addarr_strset(StrSet *, char *, size_t);
static inline char *addraw_strset(StrSet *, const char *);
static inline int addarr_strset(StrSet *, char *, size_t);
static inline char **lower_bound(char **, char **, const char *, ptrdiff_t);

DECL int
init_strset_size_avg(StrSet *set, size_t size, size_t avg) {
	if ((set->raw = (char *)malloc(size * avg)) == NULL) {
		return -1;
	}
	if ((set->arr = (char **)malloc(size * sizeof(char *))) == NULL) {
		free(set->raw);
		return -1;
	}
	set->raw_size = 0;
	set->raw_alloc = size * avg;
	set->arr_size = 0;
	set->arr_alloc = size;
	return 0;
}

DECL int
init_strset_size(StrSet *set, size_t size) {
	return init_strset_size_avg(set, size, 8);
}

DECL int
init_strset(StrSet *set) {
	return init_strset_size(set, 4096);
}

DECL void
free_strset(StrSet *set) {
	set->raw_size = 0;
	set->raw_alloc = 0;
	set->arr_size = 0;
	set->arr_alloc = 0;
	assert(set->arr);
	assert(set->raw);
	free(set->arr);
	free(set->raw);
}

DECL char *
insert_strset(StrSet *set, const char *str) {
	ptrdiff_t off = (ptrdiff_t)set->raw;
	char **it = lower_bound(set->arr, set->arr + set->arr_size, str, off);
	size_t pos = it - set->arr;
	if (pos == set->arr_size || strcmp(str, *it + (ptrdiff_t)set->raw)) {
		char *nstr = addraw_strset(set, str);
		if (nstr == IFAIL) {
			return IFAIL;
		}
		if (addarr_strset(set, nstr, pos)) {
			return IFAIL;
		}
	}
	return *it;
}

DECL void
freeze_strset(StrSet *set) {
	set->raw = (char *)realloc(set->raw, set->raw_size);
	set->raw_alloc = set->raw_size;
	set->arr = (char **)realloc(set->arr, set->arr_size * sizeof(char *));
	set->arr_alloc = set->arr_size;
	offset_strset_tokv(set, set->arr, set->arr_size);
}

DECL void
offset_strset_tokv(StrSet *set, char **beg, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		beg[i] += (ptrdiff_t)set->raw;
	}
}

static inline char **
lower_bound(char **beg, char **end, const char *val, ptrdiff_t off) {
	char **it;
	size_t count = end - beg, step;
	while (count > 0) {
		it = beg;
		step = count / 2;
		it += step;
		if (strcmp(*it + off, val) < 0) {
			beg = ++it;
			count -= step + 1;
		} else {
			count = step;
		}
	}
	return beg;
}

static inline char *
addraw_strset(StrSet *set, const char *str) {
	size_t sstr = strlen(str) + 1;
	size_t res = set->raw_size;
	if (set->raw_size + sstr > set->raw_alloc) {
		size_t incr = set->raw_alloc / 2 + sstr;
		char *nraw = (char *)realloc(set->raw, set->raw_alloc + incr);
		if (nraw == NULL) {
			return IFAIL;
		}
		set->raw_alloc += incr;
		set->raw = nraw;
	}
	memcpy(set->raw + set->raw_size, str, sstr);
	set->raw_size += sstr;
	return (char *)NULL + res;
}

static inline int
addarr_strset(StrSet *set, char *str, size_t pos) {
	if (set->arr_size == set->arr_alloc) {
		size_t incr = set->arr_size / 2 + 1;
		char **narr = (char **)realloc(
		        set->arr, (set->arr_alloc + incr) * sizeof(char *));
		if (narr == NULL) {
			return -1;
		}
		set->arr_alloc += incr;
		set->arr = narr;
	}
	memmove(set->arr + pos + 1, set->arr + pos,
	        (set->arr_size - pos) * sizeof(char *));
	set->arr[pos] = str;
	set->arr_size += 1;
	return 0;
}

#endif // STR_SET_IMPL
