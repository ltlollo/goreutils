// Scope: strstr for non-zero terminated *utf8* strings
// Suggested opt falgs: 
//  -s -O3 -ftree-vectorize -funroll-all-loops
//  -fprefetch-loop-arrays -fsched-pressure -fsched-spec-load
//  -fsched-spec-load-dangerous -fschedule-insns -fschedule-insns2
//  -minline-all-stringops -fsched-stalled-insns=8 -fsched2-use-superblocks
//  -ffunction-sections -fdata-sections
// Todo: .test, .utf32(maybe), .neon, .avx, .sse3
// See also: http://0x80.pl/articles/simd-strfind.html (uses different hashing)

#include <immintrin.h>
#include <string.h>
#include <stdint.h>

#ifndef DECL
#define DECL extern
#endif // DECL

#ifndef INTER
#define INTER static inline
#endif // INTER

#define align(p) (const char *)((uintptr_t)(p) & (~0x1f))
#define unlikely(e) (__builtin_expect((e), 0))

typedef __m256i m256i;
typedef __m128i m128i;

INTER int hash1(m256i , m256i);
INTER int hash2(m256i, m256i, m256i, m256i);
INTER int hash3(m256i, m256i, m256i, m256i, m256i);
INTER int hash4(m256i, m256i, m256i, m256i, m256i);
INTER int hash33(m256i, m256i, m256i, m256i, m256i, m256i, m256i);

INTER m256i load_small(const char *, short);

INTER const char *strstrnz1(const char *, size_t, const char *, size_t);
INTER const char *strstrnz2(const char *, size_t, const char *, size_t);
INTER const char *strstrnz3(const char *, size_t, const char *, size_t);
INTER const char *strstrnz4(const char *, size_t, const char *, size_t);
INTER const char *strstrnz33(const char *, size_t, const char *, size_t);

DECL const char *
strstrnz(const char *str, size_t n, const char *sstr, size_t sn) {
    if (unlikely(sn == 0)) {
        return str;
    } else if (sn == 1) {
        return strstrnz1(str, n, sstr, sn);
    } else if (sn == 2) {
        return strstrnz2(str, n, sstr, sn);
    } else if (sn == 3) {
        return strstrnz3(str, n, sstr, sn);
    } else if (__builtin_expect(sn > 3 && sn < 33, !!1)) {
        return strstrnz4(str, n, sstr, sn);
    } else {
        return strstrnz33(str, n, sstr, sn);
    }
}

INTER int
hash1(m256i fst, m256i c0) {
    return _mm256_movemask_epi8(_mm256_cmpeq_epi8(c0, fst));
}

INTER int
hash2(m256i fst, m256i snd, m256i c0, m256i c1) {
    m256i ffeq = _mm256_cmpeq_epi8(c0, fst);
    m256i fseq = _mm256_cmpeq_epi8(c1, fst);
    m256i sseq = _mm256_cmpeq_epi8(c1, snd);

    int ffm = _mm256_movemask_epi8(ffeq);
    int fsm = _mm256_movemask_epi8(fseq) >> 1;
    int ssm = _mm256_movemask_epi8(sseq) << 31;

    return ffm & (fsm | ssm);
}

INTER int
hash3(m256i fst, m256i snd, m256i c0, m256i c1, m256i c2) {
    m256i ffeq = _mm256_cmpeq_epi8(c0, fst);
    m256i fseq = _mm256_cmpeq_epi8(c1, fst);
    m256i sseq = _mm256_cmpeq_epi8(c1, snd);
    m256i fqeq = _mm256_cmpeq_epi8(c2, fst);
    m256i sqeq = _mm256_cmpeq_epi8(c2, snd);

    int ffm = _mm256_movemask_epi8(ffeq);
    int fsm = _mm256_movemask_epi8(fseq) >> 1;
    int ssm = _mm256_movemask_epi8(sseq) << 31;
    int ftm = _mm256_movemask_epi8(fqeq) >> 2;
    int stm = _mm256_movemask_epi8(sqeq) << 30;

    return ffm & (fsm | ssm) & (ftm | stm);
}

INTER int
hash4(m256i fst, m256i snd, m256i c0, m256i c1, m256i c3) {
    m256i ffeq = _mm256_cmpeq_epi8(c0, fst);
    m256i fseq = _mm256_cmpeq_epi8(c1, fst);
    m256i sseq = _mm256_cmpeq_epi8(c1, snd);
    m256i fqeq = _mm256_cmpeq_epi8(c3, fst);
    m256i sqeq = _mm256_cmpeq_epi8(c3, snd);

    int ffm = _mm256_movemask_epi8(ffeq);
    int fsm = _mm256_movemask_epi8(fseq) >> 1;
    int ssm = _mm256_movemask_epi8(sseq) << 31;
    int fqm = _mm256_movemask_epi8(fqeq) >> 3;
    int sqm = _mm256_movemask_epi8(sqeq) << 29;

    return ffm & (fsm | ssm) & (fqm | sqm);
}

// INTER int
// hash17(m256i fst, m256i snd, m256i c0, m256i c1, m256i c3, m256i c16) {
//     m256i mstr = _mm256_permute2f128_si256(fst, snd, 0x21);
//     m256i ffeq = _mm256_cmpeq_epi8(c0, fst);
//     m256i fseq = _mm256_cmpeq_epi8(c1, fst);
//     m256i sseq = _mm256_cmpeq_epi8(c1, snd);
//     m256i fqeq = _mm256_cmpeq_epi8(c3, fst);
//     m256i sqeq = _mm256_cmpeq_epi8(c3, snd);
//     m256i mmeq = _mm256_cmpeq_epi8(c16, mstr);
// 
//     int ffm = _mm256_movemask_epi8(ffeq);
//     int fsm = _mm256_movemask_epi8(fseq) >> 1;
//     int ssm = _mm256_movemask_epi8(sseq) << 31;
//     int fqm = _mm256_movemask_epi8(fqeq) >> 3;
//     int sqm = _mm256_movemask_epi8(sqeq) << 29;
//     int mfm = _mm256_movemask_epi8(mmeq);
// 
//     return ffm & (fsm | ssm) & (fqm | sqm) & mfm;
// }

INTER int
hash33(m256i fst,
       m256i snd,
       m256i c0,
       m256i c1,
       m256i c3,
       m256i c16,
       m256i c32) {
    m256i mstr = _mm256_permute2f128_si256(fst, snd, 0x21);
    m256i ffeq = _mm256_cmpeq_epi8(c0, fst);
    m256i fseq = _mm256_cmpeq_epi8(c1, fst);
    m256i sseq = _mm256_cmpeq_epi8(c1, snd);
    m256i fqeq = _mm256_cmpeq_epi8(c3, fst);
    m256i sqeq = _mm256_cmpeq_epi8(c3, snd);
    m256i mmeq = _mm256_cmpeq_epi8(c16, mstr);
    m256i sfeq = _mm256_cmpeq_epi8(c32, snd);

    int ffm = _mm256_movemask_epi8(ffeq);
    int fsm = _mm256_movemask_epi8(fseq) >> 1;
    int ssm = _mm256_movemask_epi8(sseq) << 31;
    int fqm = _mm256_movemask_epi8(fqeq) >> 3;
    int sqm = _mm256_movemask_epi8(sqeq) << 29;
    int mfm = _mm256_movemask_epi8(mmeq);
    int sfm = _mm256_movemask_epi8(sfeq);

    return ffm & (fsm | ssm) & (fqm | sqm) & mfm & sfm;
}

INTER m256i
load_small(const char *str, short size) {
    char s[32] = { 0 };
    for (short i = 0; i < size && i < 32; ++i) {
        s[i] = str[i];
    }
    return _mm256_set_epi8(s[31], s[30], s[29], s[28], s[27], s[26], s[25],
                           s[24], s[23], s[22], s[21], s[20], s[19], s[18],
                           s[17], s[16], s[15], s[14], s[13], s[12], s[11],
                           s[10], s[9], s[8], s[7], s[6], s[5], s[4], s[3],
                           s[2], s[1], s[0]);
}

INTER const char *
strstrnz3(const char *str, size_t n, const char *sstr, size_t sn) {
    if (unlikely(n < sn)) {
        return NULL;
    }
    m256i c0 = _mm256_set1_epi8(sstr[0]);
    m256i c1 = _mm256_set1_epi8(sstr[1]);
    m256i c3 = _mm256_set1_epi8(sstr[2]);
    m256i z = _mm256_set1_epi8(0);
    m256i f;
    m256i s;
    const char *b;
    const char *e;
    int hash;

    if (n < 32) {
        f = load_small(str, n);
        s = z;
        hash = hash3(f, s, c0, c1, c3);
        if (hash) {
            b = str;
            e = str + n - sn + 1;
            while (b != e) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
                b++;
            }
        }
        return NULL;
    }
    if (n < 64) {
        f = _mm256_lddqu_si256((__m256i *)str);
        s = load_small(str + 32, n - 32);
        hash = hash3(f, s, c0, c1, c3);
        if (hash) {
            for (b = str; b < str + 32; ++b) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
            }
        }
        f = s;
        s = z;
        hash = hash3(f, s, c0, c1, c3);
        if (hash) {
            b = str;
            e = str + n - sn + 1;
            while (b != e) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
                b++;
            }
        }
        return NULL;
    }
    f = _mm256_lddqu_si256((__m256i *)str);
    s = _mm256_lddqu_si256((__m256i *)(str + 32));
    hash = hash3(f, s, c0, c1, c3);
    if (hash) {
        for (b = str; b < str + 32; ++b) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
        }
    }
    b = align(str) + 0x20;
    e = align(str + n);
    s = _mm256_load_si256((__m256i *)b);
    while (b < e - 32) {
        b += 32;
        f = s;
        s = _mm256_load_si256((__m256i *)b);
        hash = hash3(f, s, c0, c1, c3);
        if (hash) {
            for (const char *bb = b - 32; bb < b; ++bb) {
                if (memcmp(bb, sstr, sn) == 0) {
                    return bb;
                }
            }
        }
    }
    f = s;
    s = load_small(b, e - b);
    hash = hash3(f, s, c0, c1, c3);
    if (hash) {
        while (b != e - sn + 1) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
            b++;
        }
    }
    return NULL;
}


INTER const char *
strstrnz4(const char *str, size_t n, const char *sstr, size_t sn) {
    if (unlikely(n < sn)) {
        return NULL;
    }
    m256i c0 = _mm256_set1_epi8(sstr[0]);
    m256i c1 = _mm256_set1_epi8(sstr[1]);
    m256i c3 = _mm256_set1_epi8(sstr[3]);
    m256i z = _mm256_set1_epi8(0);
    m256i f;
    m256i s;
    const char *b;
    const char *e;
    int hash;

    if (n < 32) {
        f = load_small(str, n);
        s = z;
        hash = hash4(f, s, c0, c1, c3);
        if (hash) {
            b = str;
            e = str + n - sn + 1;
            while (b != e) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
                b++;
            }
        }
        return NULL;
    }
    if (n < 64) {
        f = _mm256_lddqu_si256((__m256i *)str);
        s = load_small(str + 32, n - 32);
        hash = hash4(f, s, c0, c1, c3);
        if (hash) {
            for (b = str; b < str + 32; ++b) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
            }
        }
        f = s;
        s = z;
        hash = hash4(f, s, c0, c1, c3);
        if (hash) {
            b = str;
            e = str + n - sn + 1;
            while (b != e) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
                b++;
            }
        }
        return NULL;
    }
    f = _mm256_lddqu_si256((__m256i *)str);
    s = _mm256_lddqu_si256((__m256i *)(str + 32));
    hash = hash4(f, s, c0, c1, c3);
    if (hash) {
        for (b = str; b < str + 32; ++b) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
        }
    }
    b = align(str) + 0x20;
    e = align(str + n);
    s = _mm256_load_si256((__m256i *)b);
    while (b < e - 32) {
        b += 32;
        f = s;
        s = _mm256_load_si256((__m256i *)b);
        hash = hash4(f, s, c0, c1, c3);
        if (hash) {
            for (const char *bb = b - 32; bb < b; ++bb) {
                if (memcmp(bb, sstr, sn) == 0) {
                    return bb;
                }
            }
        }
    }
    f = s;
    s = load_small(b, e - b);
    hash = hash4(f, s, c0, c1, c3);
    if (hash) {
        while (b != e - sn + 1) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
            b++;
        }
    }
    return NULL;
}


INTER const char *
strstrnz2(const char *str, size_t n, const char *sstr, size_t sn) {
    if (unlikely(n < sn)) {
        return NULL;
    }
    m256i c0 = _mm256_set1_epi8(sstr[0]);
    m256i c1 = _mm256_set1_epi8(sstr[1]);
    m256i z = _mm256_set1_epi8(0);
    m256i f;
    m256i s;
    const char *b;
    const char *e;
    int hash;

    if (n < 32) {
        f = load_small(str, n);
        s = z;
        hash = hash2(f, s, c0, c1);
        if (hash) {
            b = str;
            e = str + n - sn + 1;
            while (b != e) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
                b++;
            }
        }
        return NULL;
    }
    if (n < 64) {
        f = _mm256_lddqu_si256((__m256i *)str);
        s = load_small(str + 32, n - 32);
        hash = hash2(f, s, c0, c1);
        if (hash) {
            for (b = str; b < str + 32; ++b) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
            }
        }
        f = s;
        s = z;
        hash = hash2(f, s, c0, c1);
        if (hash) {
            b = str;
            e = str + n - sn + 1;
            while (b != e) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
                b++;
            }
        }
        return NULL;
    }
    f = _mm256_lddqu_si256((__m256i *)str);
    s = _mm256_lddqu_si256((__m256i *)(str + 32));
    hash = hash2(f, s, c0, c1);
    if (hash) {
        for (b = str; b < str + 32; ++b) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
        }
    }
    b = align(str) + 0x20;
    e = align(str + n);
    s = _mm256_load_si256((__m256i *)b);
    while (b < e - 32) {
        b += 32;
        f = s;
        s = _mm256_load_si256((__m256i *)b);
        hash = hash2(f, s, c0, c1);
        if (hash) {
            for (const char *bb = b - 32; bb < b; ++bb) {
                if (memcmp(bb, sstr, sn) == 0) {
                    return bb;
                }
            }
        }
    }
    f = s;
    s = load_small(b, e - b);
    hash = hash2(f, s, c0, c1);
    if (hash) {
        while (b != e - sn + 1) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
            b++;
        }
    }
    return NULL;
}


INTER const char *
strstrnz1(const char *str, size_t n, const char *sstr, size_t sn) {
    if (unlikely(n < sn)) {
        return NULL;
    }
    m256i c0 = _mm256_set1_epi8(sstr[0]);
    m256i f;
    const char *b;
    const char *e;
    int hash;

    if (n < 32) {
        f = load_small(str, n);
        hash = hash1(f, c0);
        if (hash) {
            b = str;
            e = str + n;
            while (b != e) {
                if (*b == *sstr) {
                    return b;
                }
                b++;
            }
        }
        return NULL;
    }
    f = _mm256_lddqu_si256((__m256i *)str);
    hash = hash1(f, c0);
    if (hash) {
        for (b = str; b < str + 32; ++b) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
        }
    }
    b = align(str) + 0x20;
    e = align(str + n);
    while (b != e) {
        f = _mm256_load_si256((__m256i *)b);
        hash = hash1(f, c0);
        b += 32;
        if (hash) {
            for (const char *bb = b - 32; bb < b; ++bb) {
                if (*bb == *sstr) {
                    return bb;
                }
            }
        }
    }
    e = str + n;
    f = load_small(b, e - b);
    hash = hash1(f, c0);
    if (hash) {
        while (b != e) {
            if (*b == *sstr) {
                return b;
            }
            b++;
        }
    }
    return NULL;
}

INTER const char *
strstrnz33(const char *str, size_t n, const char *sstr, size_t sn) {
    if (unlikely(n < sn)) {
        return NULL;
    }
    m256i c0 = _mm256_set1_epi8(sstr[0]);
    m256i c1 = _mm256_set1_epi8(sstr[1]);
    m256i c3 = _mm256_set1_epi8(sstr[3]);
    m256i c16 = _mm256_set1_epi8(sstr[16]);
    m256i c32 = _mm256_set1_epi8(sstr[32]);
    m256i f;
    m256i s;
    const char *b;
    const char *e;
    int hash;

    if (n < 64) {
        f = _mm256_lddqu_si256((__m256i *)str);
        s = load_small(str + 32, n - 32);
        hash = hash33(f, s, c0, c1, c3, c16, c32);
        if (hash) {
            for (b = str; b < str + 32; ++b) {
                if (memcmp(b, sstr, sn) == 0) {
                    return b;
                }
            }
        }
        return NULL;
    }
    f = _mm256_lddqu_si256((__m256i *)str);
    s = _mm256_lddqu_si256((__m256i *)(str + 32));
    hash = hash33(f, s, c0, c1, c3, c16, c32);
    if (hash) {
        for (b = str; b < str + 32; ++b) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
        }
    }
    b = align(str) + 0x20;
    e = align(str + n);
    s = _mm256_load_si256((__m256i *)b);
    while (b < e - 32) {
        b += 32;
        f = s;
        s = _mm256_load_si256((__m256i *)b);
        hash = hash33(f, s, c0, c1, c3, c16, c32);
        if (hash) {
            for (const char *bb = b - 32; bb < b; ++bb) {
                if (memcmp(bb, sstr, sn) == 0) {
                    return bb;
                }
            }
        }
    }
    f = s;
    s = load_small(b, e - b);
    hash = hash33(f, s, c0, c1, c3, c16, c32);
    if (hash) {
        while (b < e - sn + 1) {
            if (memcmp(b, sstr, sn) == 0) {
                return b;
            }
            b++;
        }
    }
    return NULL;
}

