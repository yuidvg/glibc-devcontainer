# Changelog

## [Unreleased] - Current Development

### Added
- Created documentation in the `docs/` directory
- Added detailed project summary
- Added technical documentation
- Added functional programming approach documentation

### Changed
- Updated Makefile to use `-Werror` flag to treat warnings as errors
- Fixed warnings in `src/my_malloc.c` by removing unused function `areBlocksAdjacent`
- Updated `test/test_my_malloc.c` to suppress the unused parameter warning in `segfaultHandler`
- Removed the function declaration from `include/my_malloc_internal.h`

### Fixed
- Fixed potential memory leaks in error handling paths
- Fixed compilation warnings that were being promoted to errors with `-Werror`
- Improved error handling in the memory coalescing function
- Fixed edge case in the block splitting algorithm

## [0.1.0] - Initial Implementation

### Added
- Implemented custom memory allocator with zone-based allocation
- Created public API in `my_malloc.h`
- Implemented internal details in `my_malloc_internal.h` and `my_malloc.c`
- Added comprehensive test suite in `test_my_malloc.c`
- Created a demo program in `main.c`
- Implemented memory leak detection
- Added best-fit allocation strategy
- Implemented block splitting for efficient memory use
- Added memory coalescing to reduce fragmentation
- Created Makefile build system with clean, test, and run targets