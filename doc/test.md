| #   | category       | title                        | action                                                 | expected result                                             |
| --- | -------------- | ---------------------------- | ------------------------------------------------------ | ----------------------------------------------------------- |
| 1   | malloc         | basic-allocation             | `p = malloc(42)`                                       | `p != NULL` ∧ read/write `p[0…41]` OK                       |
| 2   | malloc         | size-0-behaviour             | `p = malloc(0)`                                        | `(p==NULL)` ∨ `(unique pointer ∧ free(p) safe)`             |
| 3   | malloc         | alignment                    | `uintptr_t(p) % 16`                                    | == 0                                                        |
| 4   | malloc         | tiny-upper-bound             | `malloc(n)` where *n* = last TINY size                 | listed under TINY in `show_alloc_mem`                       |
| 5   | malloc         | small-lower-bound            | `malloc(n+1)`                                          | listed under SMALL                                          |
| 6   | malloc         | large-allocation             | `malloc(m+1)`                                          | individual `mmap`; listed under LARGE                       |
| 7   | malloc         | huge-but-legal               | `malloc(RLIMIT_AS/4)`                                  | success ∨ `(NULL ∧ errno==ENOMEM)`                          |
| 8   | free           | normal-free                  | `free(p)` (p from malloc)                              | no crash                                                    |
| 9   | free           | null-pointer                 | `free(NULL)`                                           | no crash                                                    |
| 10  | free           | double-free                  | `free(p); free(p)`                                     | program ≠ SEGFAULT; detects error or aborts                 |
| 11  | free           | foreign-pointer              | `free(ptr_from_glibc)`                                 | safe refusal (abort or error)                               |
| 12  | realloc        | grow-in-place                | `p = malloc(32); q = realloc(p,64)`                    | `q == p` ∧ bytes 0–31 preserved                             |
| 13  | realloc        | shrink                       | `p = malloc(128); q = realloc(p,32)`                   | bytes 0–31 preserved                                        |
| 14  | realloc        | ptr-null                     | `realloc(NULL,50)`                                     | behaves as `malloc(50)`                                     |
| 15  | realloc        | size-0                       | `realloc(p,0)`                                         | acts like `free(p)` **or** returns free-able unique pointer |
| 16  | realloc        | large-to-small-migrate       | `p = malloc(m+100); q = realloc(p,m)`                  | `q` in SMALL zone; former region `munmap`ed                 |
| 17  | zones          | at-least-100-allocs-per-zone | loop `malloc(n-1)` 100×                                | ≤ 1 `mmap` call (via strace)                                |
| 18  | zones          | page-multiple-mapping        | inspect mapped length                                  | length mod pagesize == 0                                    |
| 19  | zones          | full-large-free              | `free(bigptr)`                                         | exactly one `munmap`                                        |
| 20  | show-alloc-mem | header-order                 | mixed allocs → call viewer                             | headers TINY < SMALL < LARGE, addresses ascending           |
| 21  | show-alloc-mem | line-format                  | apply regex `0x[0-9A-F]+ - 0x[0-9A-F]+ : [0-9]+ bytes` | matches every data line                                     |
| 22  | show-alloc-mem | total-accurate               | sum bytes from lines                                   | equals final “Total : N bytes”                              |
| 23  | alignment-edge | one-to-sixtyfour-byte-sweep  | allocate sizes 1…64                                    | no overlaps ∧ each aligned                                  |
| 24  | canary-edge    | uaf-pattern                  | fill block 0xAA, free, malloc same size                | never crashes (behaviour documented)                        |
| 25  | error          | mmap-failure                 | lower `RLIMIT_AS`; `malloc(1 MiB)`                     | returns NULL ∧ `errno==ENOMEM`                              |
| 26  | error          | size-t-overflow              | `malloc(SIZE_MAX)`                                     | returns NULL                                                |
| 28  | stress         | random-10m-ops               | 10 M random malloc/free/realloc                        | no crash; invariants hold; end total == 0                   |
| 29  | stress         | fragmentation                | allocate/free pattern, then `malloc(7 000 000)`        | success w/o extra `mmap` (if defrag) else safe failure      |
| 30  | perf           | mmap-budget-tiny             | 10 000 TINY alloc/free                                 | (`mmap` + `munmap`) ≤ 2                                     |
| 31  | perf           | latency                      | benchmark 1 M malloc/free(16)                          | mean ≤ 3 × glibc                                            |
| 32  | analysis       | valgrind-clean               | run suite under Valgrind                               | 0 leaks / UMR                                               |
| 33  | analysis       | static-scan                  | `scan-build make`                                      | no high-severity warnings                                   |

**Notes**

* `n`, `m` = upper bounds you define for TINY and SMALL sizes.
* `pagesize` = `getpagesize()` or `sysconf(_SC_PAGESIZE)`.