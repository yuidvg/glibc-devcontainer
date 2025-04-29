Below is a **check-list-style test plan** you can take straight to your favourite harness (Python + ctypes, Criterion, GoogleTest, etc.).
It covers every behaviour explicitly required in the *ft-malloc* specification and adds practical edge-, stress- and error-cases that evaluators usually look for.

---

## 0 — Build & linkage smoke tests
| Goal | What to do | Expected |
|------|------------|----------|
| Correct library name | `make re && ls libft_malloc_$HOSTTYPE.so` | File exists (size > 0) ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG) |
| Symlink | `readlink libft_malloc.so` | Points to the file above ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG) |
| Public symbols only | `nm -gD libft_malloc_$HOSTTYPE.so` | **Only** `malloc`, `free`, `realloc`, `show_alloc_mem` +
optional helper(s); no stray `printf`, `new`, etc. |
| Can be preloaded | `LD_PRELOAD=$PWD/libft_malloc.so ls >/dev/null` | Program runs & exits 0 |

---

## 1 — Functional API tests

### 1.1 `malloc`
| Case | Action | Check |
|------|--------|-------|
| Basic | `p = malloc(42)` | `p != NULL`; subsequent read/write ok |
| `size == 0` | `p = malloc(0)` | **Either** `NULL` **or** unique non-NULL pointer that can be freed (both allowed by POSIX, document choice) |
| First/last byte | Write to `p[0]` & `p[41]` | Value round-trips |
| Alignment | `uintptr_t(p) % alignof(max_align_t) == 0` |
| Category boundaries | Request `n`, `n+1`, `m`, `m+1` bytes → verify each is placed in expected TINY/SMALL/LARGE zone via `show_alloc_mem` ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG) |
| Huge but legal | `malloc(RLIMIT_AS/4)` | Either success or `NULL` with `errno == ENOMEM` |

### 1.2 `free`
| Case | Action | Check |
|------|--------|-------|
| Normal | `free(p)` | No crash; subsequent `malloc(42)` may reuse same addr (optional) |
| `ptr == NULL` | `free(NULL)` | No crash ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG) |
| Double free (error) | `free(p); free(p)` | Library *must not* seg-fault; detect & ignore or abort with diagnostic |
| Pointer not ours | Pass heap pointer from glibc `malloc` | Should refuse safely (usual answer: abort) |

### 1.3 `realloc`
| Scenario | Steps | Expected |
|----------|-------|----------|
| Grow in-place | `p = malloc(32)` → `q = realloc(p,64)` | `q==p`; data preserved |
| Shrink | `p = malloc(128)` → `q = realloc(p,32)` | `q` may move or not, but bytes 0-31 unchanged |
| `ptr==NULL` | `realloc(NULL, 50)` | Behaves like `malloc(50)` |
| `size==0` | `realloc(p, 0)` | Behaves like `free(p)` **OR** returns unique pointer immediately free-able (document choice) |
| Realloc of LARGE that fits SMALL | `malloc(m+100)` then `realloc(..., m)` | Should migrate into SMALL zone & munmap former region |

---

## 2 — Zone-management tests

| Focus | Method | Expected |
|-------|--------|----------|
| ≥ 100 allocs/zone | Loop `malloc(n-1)` 100× | Only **one** `mmap` for the whole loop (strace) ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG) |
| Page multiple size | Check size of `mmap`ed area modulo `getpagesize()` | == 0 |
| mmap reduction | After freeing all blocks in a LARGE alloc, ensure a `munmap` occurs (strace count) |

---

## 3 — `show_alloc_mem` output tests

1. Invoke after a known allocation pattern.
2. Parse stdout:
   * Headers appear in ascending address order: `TINY :`, `SMALL :`, `LARGE :` ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG)
   * Each line: `0xADDR - 0xADDR : N bytes`
   * Final total matches sum.
3. Ensure the list is **sorted** by start address.

Tip: Use regex in the test harness, ignore colour codes if you add them.

---

## 4 — Alignment & padding edge cases

* Allocate every size **1 .. 64 B**; verify each returned pointer respects alignment and the next allocation does not overlap.
* Canary test: fill each block with `0xAA`, `free`, `malloc(same size)` → memory should be zeroed **only if** you implement such feature; otherwise data may remain (allowed). Document behaviour.

---

## 5 — Error-handling tests

| Error | Injection technique | Expectation |
|-------|--------------------|-------------|
| mmap fails | Temporarily reduce `RLIMIT_AS` with `setrlimit` | `malloc`/`realloc` return `NULL` and `errno==ENOMEM` |
| Invalid size_t overflow | `malloc(SIZE_MAX)` | Return `NULL` (cannot succeed) |
| Corrupted header | Manually flip size field then `free` | Library detects & aborts cleanly (no seg-fault) |

---

## 6 — Stress / fuzz tests

### 6.1 Randomised allocator torture
* Seeded PRNG produces 10 million operations:
  * 40 % `malloc` random [1 … 2 MiB]
  * 40 % `free` of random live pointer
  * 20 % `realloc` random live pointer to random new size
* Invariants checked continuously:
  * No overlapping live regions (interval tree).
  * `malloc` never returns NULL while free memory exists (optional).
  * At end, free everything → check `show_alloc_mem` reports **0 B** total.

### 6.2 Fragmentation stress
```
for i in {1..200000}; do
    ptr[i] = malloc( (i%3 ? 32 : 8000) );
done
free every second pointer
malloc 7 000 000 bytes   # should succeed without a fresh mmap if defragment works
```
*(The last expectation becomes a soft check if you did **not** implement defragmentation; it must at least not crash.)*

---

## 7 — Performance & syscall budget

| Metric | Tool | Threshold |
|--------|------|-----------|
| mmap+munmap calls per 10 000 TINY allocs | `strace -c` | **≤ 2** (one initial mmap, no munmap) |
| Latency | Benchmark `for(i=0;i<1e6) malloc(16); free` | Within 3× glibc on same machine |

---

## 8 — Static & dynamic analysis

* **Valgrind / ASan**: run whole suite, expect **zero** leaks, errors or UMRs.
* **`scan-build` / `cppcheck`**: no high-severity warnings.
These are not spec-mandated but raise confidence.

---

### Quick reference of spec points covered
* Function prototypes & NULL / error rules ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG)
* Use of `mmap` / `munmap` only ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG)
* TINY / SMALL / LARGE zones, ≥ 100 allocs each, page-multiple size ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG)
* `show_alloc_mem` format ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG)
* Alignment requirement ‡ft-malloc.pdf](file-service://file-RFbTis1Nhh6769aEJUhFkG)

---

### How to run everything automatically
1. **Harness skeleton** (Python):
   * Use `ctypes.CDLL("./libft_malloc.so")` to call your functions directly.
   * Wrap allocations in tiny objects so Python’s GC frees them deterministically.
2. **Syscall counting**: launch tests under `strace -f -e mmap,munmap`.
3. **Leak check**: tail-run under `valgrind --leak-check=full`.
4. Make the suite itself run under **`LD_PRELOAD`** to catch interactions with real programs (e.g., `grep`, `tar`, `python` itself).

With this table of tests in place you can be confident your *ft-malloc* meets *all* mandatory requirements and resists the classic evaluation traps. Happy hacking!