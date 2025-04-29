| #   | category       | title                        | action                                                 | expected result                                              |
| --- | -------------- | ---------------------------- | ------------------------------------------------------ | ------------------------------------------------------------ |
| 1   | build-linkage  | correct-library-name         | `make re ; stat libft_malloc_$HOSTTYPE.so`             | file exists ∧ size > 0                                       |
| 2   | build-linkage  | symlink-created              | `readlink libft_malloc.so`                             | path == `./libft_malloc_$HOSTTYPE.so`                        |
| 3   | build-linkage  | only-public-symbols          | `nm -gD libft_malloc_$HOSTTYPE.so`                     | symbols ⊆ {malloc, free, realloc, show_alloc_mem,(helpers?)} |
| 4   | build-linkage  | preload-smoke                | `LD_PRELOAD=./libft_malloc.so /bin/true`               | exit status == 0                                             |
| 5   | malloc         | basic-allocation             | `p = malloc(42)`                                       | `p != NULL` ∧ read/write `p[0…41]` OK                        |
| 6   | malloc         | size-0-behaviour             | `p = malloc(0)`                                        | `(p==NULL)` ∨ `(unique pointer ∧ free(p) safe)`              |
| 7   | malloc         | alignment                    | `uintptr_t(p) % alignof(max_align_t)`                  | == 0                                                         |
| 8   | malloc         | tiny-upper-bound             | `malloc(n)` where *n* = last TINY size                 | listed under TINY in `show_alloc_mem`                        |
| 9   | malloc         | small-lower-bound            | `malloc(n+1)`                                          | listed under SMALL                                           |
| 10  | malloc         | large-allocation             | `malloc(m+1)`                                          | individual `mmap`; listed under LARGE                        |
| 11  | malloc         | huge-but-legal               | `malloc(RLIMIT_AS/4)`                                  | success ∨ `(NULL ∧ errno==ENOMEM)`                           |
| 12  | free           | normal-free                  | `free(p)` (p from malloc)                              | no crash                                                     |
| 13  | free           | null-pointer                 | `free(NULL)`                                           | no crash                                                     |
| 14  | free           | double-free                  | `free(p); free(p)`                                     | program ≠ SEGFAULT; detects error or aborts                  |
| 15  | free           | foreign-pointer              | `free(ptr_from_glibc)`                                 | safe refusal (abort or error)                                |
| 16  | realloc        | grow-in-place                | `p=malloc(32); q=realloc(p,64)`                        | `q==p` ∧ bytes 0–31 preserved                                |
| 17  | realloc        | shrink                       | `p=malloc(128); q=realloc(p,32)`                       | bytes 0–31 preserved                                         |
| 18  | realloc        | ptr-null                     | `realloc(NULL,50)`                                     | behaves as `malloc(50)`                                      |
| 19  | realloc        | size-0                       | `realloc(p,0)`                                         | acts like `free(p)` **or** returns free-able unique pointer  |
| 20  | realloc        | large-to-small-migrate       | `p=malloc(m+100); q=realloc(p,m)`                      | `q` in SMALL zone; former region `munmap`ed                  |
| 21  | zones          | at-least-100-allocs-per-zone | loop `malloc(n-1)` 100×                                | ≤ 1 `mmap` call (via strace)                                 |
| 22  | zones          | page-multiple-mapping        | inspect mapped length                                  | length mod pagesize == 0                                     |
| 23  | zones          | full-large-free              | `free(bigptr)`                                         | exactly one `munmap`                                         |
| 24  | show-alloc-mem | header-order                 | mixed allocs → call viewer                             | headers TINY < SMALL < LARGE with ascending addresses        |
| 25  | show-alloc-mem | line-format                  | apply regex `0x[0-9A-F]+ - 0x[0-9A-F]+ : [0-9]+ bytes` | matches every data line                                      |
| 26  | show-alloc-mem | total-accurate               | sum bytes from lines                                   | equals final “Total : N bytes”                               |
| 27  | alignment-edge | one-to-sixtyfour-byte-sweep  | allocate sizes 1…64                                    | no overlaps ∧ each aligned                                   |
| 28  | canary-edge    | uaf-pattern                  | fill block 0xAA, free, malloc same size                | never crashes (behaviour documented)                         |
| 29  | error          | mmap-failure                 | lower `RLIMIT_AS`; `malloc(1 MiB)`                     | returns NULL ∧ `errno==ENOMEM`                               |
| 30  | error          | size-t-overflow              | `malloc(SIZE_MAX)`                                     | returns NULL                                                 |
| 31  | error          | corrupt-header-detect        | flip size metadata then `free`                         | terminates safely (no wild free)                             |
| 32  | stress         | random-10m-ops               | 10 M random malloc/free/realloc                        | no crash; invariants hold; end total == 0                    |
| 33  | stress         | fragmentation                | allocate/free pattern, then `malloc(7 000 000)`        | success w/o extra `mmap` (if defrag) else safe failure       |
| 34  | perf           | mmap-budget-tiny             | 10 000 TINY alloc/free                                 | (`mmap`+`munmap`) ≤ 2                                        |
| 35  | perf           | latency                      | benchmark 1 M malloc/free(16)                          | mean ≤ 3× glibc                                              |
| 36  | analysis       | valgrind-clean               | run suite under Valgrind                               | 0 leaks / UMR                                                |
| 37  | analysis       | static-scan                  | `scan-build make`                                      | no high-severity warnings                                    |

**Notes**

* `n`, `m` = upper bounds you defined for TINY and SMALL sizes.
* `pagesize` = `getpagesize()` or `sysconf(_SC_PAGESIZE)`.