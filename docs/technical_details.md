# Memory Allocator Technical Documentation

## Design Philosophy

The memory allocator follows a functional programming approach wherever possible, emphasizing immutability and pure functions. The implementation aims to be robust, efficient, and easy to reason about.

## Core Algorithms

### Memory Zone Allocation

```
createZone(ZoneType type, size_t requestedSize) -> ResultAlloc
```

This function creates a new memory zone using `mmap`:

1. Determine zone size based on type (tiny, small) or requested size (large)
2. Allocate memory from OS using `mmap`
3. Initialize zone header and first block header
4. Add zone to appropriate list (tiny, small, or large)

### Block Allocation Strategy (Best-Fit)

```
findFreeBlock(ZoneType type, size_t size) -> ResultAlloc
```

The allocator uses a best-fit strategy to minimize fragmentation:

1. Walk through all zones of the appropriate type
2. For each zone, walk through all blocks
3. Identify free blocks large enough to accommodate the request
4. Select the smallest such block (best fit)
5. If a perfect fit is found, return immediately

### Block Splitting

```
splitBlockIfNeeded(BlockHeader* block, size_t requestedSize) -> void*
```

To avoid internal fragmentation, blocks are split when appropriate:

1. If the block is significantly larger than needed (requested size + header size + minimum practical size)
2. Create a new block header after the allocated portion
3. Update both block headers appropriately
4. Return pointer to the user data area

### Memory Coalescing

```
coalesceZone(Zone* zone) -> void
```

Reduces fragmentation by merging adjacent free blocks:

1. Walk through blocks in the zone
2. When a free block is found, check if the next block is also free
3. If adjacent free blocks exist, merge them by updating size information
4. Continue until no more adjacent free blocks can be merged

## Allocation Categories

### Tiny Allocations (≤ 128 bytes)

- Pre-allocated zones of 16 pages each
- High density of allocations
- Optimized for small, frequent allocations

### Small Allocations (129-1024 bytes)

- Pre-allocated zones of 128 pages each
- Balance between allocation density and memory overhead

### Large Allocations (> 1024 bytes)

- Directly mapped from the OS for each allocation
- Unmapped when freed to return memory to the OS
- Sized to exact multiples of page size

## Memory Layout

### Zone Structure

```
+----------------+----------------+----------------+-----
| Zone Header    | Block Header   | User Data      | ...
+----------------+----------------+----------------+-----
```

### Block Structure

```
+----------------+----------------+
| Block Header   | User Data      |
+----------------+----------------+
```

## Error Handling

The allocator handles various error conditions:

1. Out of memory: Returns NULL when allocation fails
2. Invalid free: Attempts to detect and safely handle
3. Double free: Implementation is designed to be safe against double-free bugs
4. Zero-size allocation: Returns NULL as per standard convention

## Memory Leak Detection

When DEBUG_MEMORY_LEAKS is enabled:

1. Every allocation is tracked in a linked list
2. Each free operation removes the corresponding entry
3. At program termination, remaining entries indicate leaks
4. A detailed leak report is generated

## Performance Considerations

1. **Allocation Speed**: O(n) where n is the number of existing blocks in relevant zones
2. **Memory Overhead**:
   - Tiny allocations: ~5% overhead
   - Small allocations: ~3% overhead
   - Large allocations: Negligible overhead (just header size)
3. **Fragmentation**: Minimized through best-fit and coalescing
4. **Locality**: Allocations within the same zone have good spatial locality

## Thread Safety

The current implementation is not thread-safe. To make it thread-safe:

1. Add mutex locks around critical sections
2. Consider per-thread allocation pools
3. Implement thread-local caching for small allocations