#ifndef H
#define H

#include <stdlib.h>
#include <errno.h>

#define STR(...) "" #__VA_ARGS__
#define CXSIZE(x) (sizeof(x) / sizeof(*x))
#define CXSTRLEN(x) (MCL_CXSIZE(x) - 1)
#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)
#define ATTR(...) __attribute__((__VA_ARGS__))

#define UNUSED ATTR(unused)
#ifdef STATIC
#define DECL static UNUSED
#else
#define DECL extern "C"
#endif

DECL int reallocarr(void **mem, size_t size, size_t nmemb);
DECL int reallocarr_incr(void **mem, size_t size, size_t nmemb, size_t nincr);

#ifdef IMPL

DECL int
reallocarr(void **mem, size_t size, size_t nmemb) {
	size_t tot;
	void *res;
	if (__builtin_umull_overflow(size, nmemb, &tot)) {
		errno = ENOMEM;
		return -1;
	} else if ((res = realloc(*mem, tot)) == NULL) {
		return -1;
	}
	*mem = res;
	return 0;
}

DECL int
reallocarr_incr(void **mem, size_t size, size_t nmemb, size_t nincr) {
	size_t tot;
	void *res;
	if (__builtin_uaddl_overflow(nmemb, nincr, &tot)) {
		errno = ENOMEM;
		return -1;
	} else if (__builtin_umull_overflow(size, tot, &tot)) {
		errno = ENOMEM;
		return -1;
	} else if ((res = realloc(*mem, tot)) == NULL) {
		return -1;
	}
	*mem = res;
	return 0;
}


#endif // IMPL

#endif // H

