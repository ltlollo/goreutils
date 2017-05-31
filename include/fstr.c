#include <immintrin.h>
#include <memory.h>
#include <assert.h>

/* Description: a string implementation build for fast equality comparisons.
 * The string is written to include a short variant, made to fit a cache line.
 * The string is considered short when the 31st byte is 0.
 * In order for the equality comparison to work the ramining bytes of the
 * string must be kept 0.
 * Motivation: in normal real world circumstances:
 * strings representing lines are < 32 bytes with p = 0.75,
 * strings representing words are < 32 bytes with p = 0.99,
 * strings are usually compared for equality.
 */

struct FStr {
	union {
		struct {
			size_t size;
			size_t alloc;
			char *lstr;
			char _[32 - sizeof(struct LongStr *) - 2 * sizeof(size_t)];
		};
		char sstr[32];
	};
};

static_assert(sizeof(struct FStr) == 32, "unsupported arch");

int 
str_equal(struct FStr *f, struct FStr *s) {
	__m256i ssf = _mm256_lddqu_si256((void *)f); 
	__m256i sss = _mm256_lddqu_si256((void *)s); 
	__m256i one = _mm256_set_epi64x(0xff00000000000000, 0, 0, 0);
	__m256i taintf = _mm256_add_epi8(ssf, one);
	__m256i diff = _mm256_cmpeq_epi8(taintf, sss);
	int res = _mm256_movemask_epi8(diff);

	if (res < 0x7fffffff) {
		/* most common case, different short strings */
		return 0;
	}
	if (f->sstr[31] == s->sstr[31]) {
		/* second most common case, equal short strings
		 * also valid for long strings pointing to the same buffer
		 */
		return 1;
	}
	if (f->size != s->size) {
		return 0;
	}
	return memcmp(f->lstr, s->lstr, f->size) == 0;
}

size_t
str_size(struct FStr *s) {
	__m256i sf = _mm256_lddqu_si256((void *)s);
	__m256i zero = _mm256_set1_epi64x(0);
	__m256i diff = _mm256_cmpeq_epi8(zero, sf);
	unsigned res = _mm256_movemask_epi8(diff);
	size_t size =  __builtin_ctz(res);

	if (__builtin_expect(res >= 0x80000000, 1)) {
		return size;
	}
	_mm_prefetch(s->lstr + s->size - 1, _MM_HINT_NTA);
	return s->size;
}

int
str_push(struct FStr *s, char c) {
	__m256i sf = _mm256_lddqu_si256((void *)s);
	size_t size = str_size(s);
	size_t alloc;
	char *buf;
	__m256i ls;

	if (__builtin_expect(size < 31, 1)) {
		s->sstr[size] = c;
		return 1;
	} else if (__builtin_expect(size > 31, 1)) {
		if (size + 1 < s->alloc) {
			s->lstr[size] = c;
			s->size++;
			return 1;
		}
		alloc = s->alloc + s->alloc / 2;
		buf = realloc(s->lstr, alloc * sizeof(*s->lstr));
		if (__builtin_expect(buf == NULL, 0)) {
			return 0;
		}
		buf[size] = c;
		buf[size + 1] = '\0';
		size++;
		ls = _mm256_set_epi64x(0xffffffffffffffff, (int64_t)buf, alloc, size);
		_mm256_storeu_si256((void *)s, ls);
		return 1;
	} else {
		buf = malloc(64 * sizeof(*s->lstr));
		if (__builtin_expect(buf == NULL, 0)) {
			return 0;
		}
		_mm256_storeu_si256((void *)buf, sf);
		buf[31] = c;
		buf[32] = '\0';
		ls = _mm256_set_epi64x(0xffffffffffffffff, (int64_t)buf, 64, 32);
		_mm256_storeu_si256((void *)s, ls);
		return 1;
	}
}

void
str_pop(struct FStr *s) {
	__m256i sf = _mm256_lddqu_si256((void *)s);
	size_t size = str_size(s);

	if (__builtin_expect(size < 32, 1)) {
		s->sstr[size] = '\0';
	} else if (__builtin_expect(size > 32, 1)) {
		s->lstr[size] = '\0';
		s->size--;
	} else {
		s->lstr[31] = '\0';
		sf = _mm256_lddqu_si256((void *)s->lstr);
		free(s->lstr);
		_mm256_storeu_si256((void *)s, sf);
	}
}

void
str_free(struct FStr *s) {
	size_t size = str_size(s);
	if (__builtin_expect(size < 32, 1)) {
		return;
	}
	free(s->lstr);
}

