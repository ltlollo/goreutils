#ifndef H
#define H

#include <errno.h>
#include <stdlib.h>

#define STR(...) "" #__VA_ARGS__
#define TOK(x) STR(x)
#define CXSIZE(x) (sizeof(x) / sizeof(*x))
#define CXSTRLEN(x) (CXSIZE(x) - 1)
#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)
#define ATTR(...) __attribute__((__VA_ARGS__))
#define EXPECT(x, v) __builtin_expect(x, v)
#define UNLIKELY(x) EXPECT(!!(x), 0)
#define LIKELY(x) EXPECT(!!(x), 1)
#define UNLIKELY_IF_(x) if (UNLIKELY(x))
#define LIKELY_IF_(x) if (LIKELY(x))

#define PERR(x) fwrite(x, 1, CXSTRLEN(x), stderr)
#define FAIL(x)                                                               \
	do {                                                                  \
		PERR(x);                                                      \
		exit(EXIT_FAILURE);                                           \
	} while (0)

#define RUNTIME_ASSERT(x)                                                     \
	do {                                                                  \
		UNLIKELY_IF_(!(x)) {                                          \
			FAIL("err: (" STR(x) ") failed at line " TOK(         \
			        __LINE__) " in " __FILE__ "\n");              \
		}                                                             \
	} while (0)

#ifndef NDEBUG
#define DEBUG_EXPR(x) (x)
#define DEBUG_ASSERT(x)                                                       \
	do {                                                                  \
		__asm__ __volatile__("" : : : "memory");                      \
		UNLIKELY_IF_(!(x)) {                                          \
			FAIL("err: (" STR(x) ") failed at line " TOK(         \
			        __LINE__) " in " __FILE__ "\n");              \
		}                                                             \
	} while (0)
#else
#define DEBUG_ASSERT(x) (void)(x)
#define DEBUG_EXPR(x)
#endif

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
	UNLIKELY_IF_(__builtin_umull_overflow(size, nmemb, &tot)) {
		errno = ENOMEM;
		return -1;
	}
	else UNLIKELY_IF_((res = realloc(*mem, tot)) == NULL) {
		return -1;
	}
	*mem = res;
	return 0;
}

DECL int
reallocarr_incr(void **mem, size_t size, size_t nmemb, size_t nincr) {
	size_t tot;
	void *res;
	UNLIKELY_IF_(__builtin_uaddl_overflow(nmemb, nincr, &tot)) {
		errno = ENOMEM;
		return -1;
	}
	else UNLIKELY_IF_(__builtin_umull_overflow(size, tot, &tot)) {
		errno = ENOMEM;
		return -1;
	}
	else UNLIKELY_IF_((res = realloc(*mem, tot)) == NULL) {
		return -1;
	}
	*mem = res;
	return 0;
}

#endif // IMPL

#endif // H

