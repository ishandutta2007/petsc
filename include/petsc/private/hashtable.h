#pragma once

#include <petsc/private/petscimpl.h>

#define kh_inline   inline
#define klib_unused PETSC_UNUSED
#if !defined(kh_foreach_value)
  #define undef_kh_foreach_value
#endif
#include <petsc/private/khash/khash.h>
#if defined(undef_kh_foreach_value)
  #undef kh_foreach_value
  #undef undef_kh_foreach_value
#endif

/* Required for khash <= 0.2.5 */
#if !defined(kcalloc)
  #define kcalloc(N, Z) calloc(N, Z)
#endif
#if !defined(kmalloc)
  #define kmalloc(Z) malloc(Z)
#endif
#if !defined(krealloc)
  #define krealloc(P, Z) realloc(P, Z)
#endif
#if !defined(kfree)
  #define kfree(P) free(P)
#endif

/* --- Useful extensions to khash --- */

#if !defined(kh_reset)
  /*! @function
  @abstract     Reset a hash table to initial state.
  @param  name  Name of the hash table [symbol]
  @param  h     Pointer to the hash table [khash_t(name)*]
 */
  #define kh_reset(name, h) \
    { \
      if (h) { \
        kfree((h)->keys); \
        kfree((h)->flags); \
        kfree((h)->vals); \
        memset((h), 0x00, sizeof(*(h))); \
      } \
    }
#endif /*kh_reset*/

#if !defined(kh_foreach)
  /*! @function
  @abstract     Iterate over the entries in the hash table
  @param  h     Pointer to the hash table [khash_t(name)*]
  @param  kvar  Variable to which key will be assigned
  @param  vvar  Variable to which value will be assigned
  @param  code  Block of code to execute
 */
  #define kh_foreach(h, kvar, vvar, code) \
    { \
      khint_t __i; \
      for (__i = kh_begin(h); __i != kh_end(h); ++__i) { \
        if (!kh_exist(h, __i)) continue; \
        (kvar) = kh_key(h, __i); \
        (vvar) = kh_val(h, __i); \
        code; \
      } \
    }
#endif /*kh_foreach*/

#if !defined(kh_foreach_key)
  /*! @function
  @abstract     Iterate over the keys in the hash table
  @param  h     Pointer to the hash table [khash_t(name)*]
  @param  kvar  Variable to which key will be assigned
  @param  code  Block of code to execute
 */
  #define kh_foreach_key(h, kvar, code) \
    { \
      khint_t __i; \
      for (__i = kh_begin(h); __i != kh_end(h); ++__i) { \
        if (!kh_exist(h, __i)) continue; \
        (kvar) = kh_key(h, __i); \
        code; \
      } \
    }
#endif /*kh_foreach_key*/

#if !defined(kh_foreach_value)
  /*! @function
  @abstract     Iterate over the values in the hash table
  @param  h     Pointer to the hash table [khash_t(name)*]
  @param  vvar  Variable to which value will be assigned
  @param  code  Block of code to execute
 */
  #define kh_foreach_value(h, vvar, code) \
    do { \
      khint_t __i; \
      for (__i = kh_begin(h); __i != kh_end(h); ++__i) { \
        if (!kh_exist(h, __i)) continue; \
        (vvar) = kh_val(h, __i); \
        code; \
      } \
    } while (0)
#endif /*kh_foreach_value*/

/* --- Helper macro for error checking --- */

#if defined(PETSC_USE_DEBUG)
  #define PetscHashAssert(expr) PetscCheck(expr, PETSC_COMM_SELF, PETSC_ERR_LIB, "[khash] Assertion: `%s' failed.", PetscStringize(expr))
#else
  #define PetscHashAssert(expr) ((void)(expr))
#endif

/* --- Low level iterator API --- */

typedef khiter_t PetscHashIter;

#define PetscHashIterAtEnd(ht, i) ((i) == kh_end(ht))

#define PetscHashIterAtBegin(ht, i) ((i) == kh_begin(ht))

#define PetscHashIterIncContinue(ht, it) (!PetscHashIterAtEnd((ht), (it)) && !kh_exist((ht), (it)))

#define PetscHashIterBegin(ht, i) \
  do { \
    PetscHashIter phib_it_ = kh_begin(ht); \
    if (PetscHashIterIncContinue((ht), phib_it_)) PetscHashIterNext((ht), phib_it_); \
    (i) = phib_it_; \
  } while (0)

#define PetscHashIterNext(ht, i) \
  do { \
    ++(i); \
  } while (PetscHashIterIncContinue((ht), (i)))

#define PetscHashIterEnd(ht, i) ((i) = kh_end(ht))

#define PetscHashIterDecContinue(ht, it) (PetscHashIterAtEnd((ht), (it)) || (!PetscHashIterAtBegin((ht), (it)) && !kh_exist((ht), (it))))

#define PetscHashIterPrevious(ht, i) \
  do { \
    PetscHashIter phip_it_ = (i); \
    PetscAssertAbort(!PetscHashIterAtBegin((ht), phip_it_), PETSC_COMM_SELF, PETSC_ERR_PLIB, "Trying to decrement iterator past begin"); \
    do { \
      --phip_it_; \
    } while (PetscHashIterDecContinue((ht), phip_it_)); \
    (i) = phip_it_; \
  } while (0)

#define PetscHashIterGetKey(ht, i, k) ((k) = kh_key((ht), (i)))

#define PetscHashIterGetVal(ht, i, v) ((v) = kh_val((ht), (i)))

#define PetscHashIterSetVal(ht, i, v) (kh_val((ht), (i)) = (v))

/* --- Thomas Wang integer hash functions --- */

typedef khint32_t PetscHash32_t;
typedef khint64_t PetscHash64_t;
typedef khint_t   PetscHash_t;

/* Thomas Wang's first version for 32-bit integers */
static inline PetscHash_t PetscHash_UInt32_v0(PetscHash32_t key)
{
  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);
  return key;
}

/* Thomas Wang's second version for 32-bit integers */
static inline PetscHash_t PetscHash_UInt32_v1(PetscHash32_t key)
{
  key = ~key + (key << 15); /* key = (key << 15) - key - 1; */
  key = key ^ (key >> 12);
  key = key + (key << 2);
  key = key ^ (key >> 4);
  key = key * 2057; /* key = (key + (key << 3)) + (key << 11); */
  key = key ^ (key >> 16);
  return key;
}

static inline PetscHash_t PetscHash_UInt32(PetscHash32_t key)
{
  return PetscHash_UInt32_v1(key);
}

/* Thomas Wang's version for 64-bit integer -> 32-bit hash */
static inline PetscHash32_t PetscHash_UInt64_32(PetscHash64_t key)
{
  key = ~key + (key << 18); /* key = (key << 18) - key - 1; */
  key = key ^ (key >> 31);
  key = key * 21; /* key = (key + (key << 2)) + (key << 4);  */
  key = key ^ (key >> 11);
  key = key + (key << 6);
  key = key ^ (key >> 22);
  return (PetscHash32_t)key;
}

/* Thomas Wang's version for 64-bit integer -> 64-bit hash */
static inline PetscHash64_t PetscHash_UInt64_64(PetscHash64_t key)
{
  key = ~key + (key << 21); /* key = (key << 21) - key - 1; */
  key = key ^ (key >> 24);
  key = key * 265; /* key = (key + (key << 3)) + (key << 8);  */
  key = key ^ (key >> 14);
  key = key * 21; /* key = (key + (key << 2)) + (key << 4); */
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}

static inline PetscHash_t PetscHash_UInt64(PetscHash64_t key)
{
  return sizeof(PetscHash_t) < sizeof(PetscHash64_t) ? (PetscHash_t)PetscHash_UInt64_32(key) : (PetscHash_t)PetscHash_UInt64_64(key);
}

static inline PetscHash_t PetscHashInt(PetscInt key)
{
#if defined(PETSC_USE_64BIT_INDICES)
  return PetscHash_UInt64((PetscHash64_t)key);
#else
  return PetscHash_UInt32((PetscHash32_t)key);
#endif
}

static inline PetscHash_t PetscHashInt64(PetscInt64 key)
{
#if defined(PETSC_USE_64BIT_INDICES)
  return PetscHash_UInt64((PetscHash64_t)key);
#else
  return PetscHash_UInt32((PetscHash32_t)key);
#endif
}

static inline PetscHash_t PetscHashPointer(const void *key)
{
#if PETSC_SIZEOF_VOID_P == 8
  return PetscHash_UInt64((PetscHash64_t)key);
#else
  return PetscHash_UInt32((PetscHash32_t)key);
#endif
}

static inline PetscHash_t PetscHashCombine(PetscHash_t seed, PetscHash_t hash)
{
  /* https://doi.org/10.1002/asi.10170 */
  /* https://dl.acm.org/citation.cfm?id=759509 */
  return seed ^ (hash + (seed << 6) + (seed >> 2));
}

#define PetscHashEqual(a, b) ((a) == (b))
