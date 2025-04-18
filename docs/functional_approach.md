# Functional Programming Approach in C

This project implements a memory allocator using functional programming principles adapted to C. The approach ensures the code is robust, maintainable, and easier to reason about.

## Core Functional Programming Principles Applied

### 1. Immutability

Where possible, we use `const` to enforce immutability:

```c
const size_t pageSize = getPageSize();
const ZoneType zoneType = getZoneTypeForSize(size);
```

Variables are defined with their final values and not modified after initialization, following the "one name, one value" principle from functional programming.

### 2. Pure Functions

Functions are designed to be pure whenever possible, with results that depend only on their inputs:

```c
static ZoneType getZoneTypeForSize(const size_t size)
{
    if (size <= TINY_MAX_SIZE)
    {
        return ZONE_TINY;
    }
    else if (size <= SMALL_MAX_SIZE)
    {
        return ZONE_SMALL;
    }
    else
    {
        return ZONE_LARGE;
    }
}
```

This function has no side effects and always returns the same output for the same input.

### 3. Result Types Instead of Error Codes

Instead of using error codes or modifying output parameters, we use result types:

```c
typedef struct {
    bool succeeded;
    void* ptr;
} ResultAlloc;
```

This allows functions to return both success/failure status and a value:

```c
ResultAlloc result = findFreeBlock(zoneType, size);
if (result.succeeded) {
    // Use result.ptr
}
```

### 4. Explicit Control Flow

Control flow is made explicit using `if/else if/else` structures rather than early returns:

```c
int classifySize(const size_t size)
{
    int category = 0;

    if (size <= TINY_MAX_SIZE)
    {
        category = 1;
    }
    else if (size <= SMALL_MAX_SIZE)
    {
        category = 2;
    }
    else
    {
        category = 3;
    }

    return category;
}
```

### 5. Avoiding Global State

Although the allocator necessarily uses some global state to track memory zones, we minimize its usage and access it through well-defined functions.

### 6. Consistent Function Structure

Functions follow a consistent structure:
1. Compute values from inputs
2. Make decisions based on those values
3. Return a single result, preferably immutable

```c
static size_t getZoneSizeForType(const ZoneType type)
{
    if (type == ZONE_TINY)
    {
        return TINY_ZONE_SIZE;
    }
    else if (type == ZONE_SMALL)
    {
        return SMALL_ZONE_SIZE;
    }
    return 0; /* LARGE zones don't have a predefined size */
}
```

### 7. Descriptive Predicate Naming

Boolean functions are named as predicates, making code more readable:

```c
bool isSizeOverflowed(const size_t size)
{
    return size > MAX_ALLOWED_SIZE;
}

// Usage
if (isSizeOverflowed(requestedSize)) {
    // Handle overflow
}
```

## Adapting Functional Concepts to C

### Managing Side Effects

Since memory allocation inherently involves side effects, we:

1. Isolate side effects to specific functions
2. Document all side effects clearly
3. Return comprehensive result types that describe what happened

### Memory Management Strategy

Memory management follows a predictable pattern:
1. Allocate memory in well-defined zones
2. Track every allocation explicitly
3. Ensure each allocation has a clear owner
4. Free memory deterministically

### Error Handling

Errors are handled functionally using result types rather than exceptions or error codes:

```c
ResultAlloc createZone(const ZoneType type, const size_t requestedSize)
{
    // Implementation...

    if (mmapResult == MAP_FAILED)
    {
        return (ResultAlloc){
            .succeeded = false,
            .ptr = NULL
        };
    }

    // Continue implementation...

    return (ResultAlloc){
        .succeeded = true,
        .ptr = newZone
    };
}
```

## Benefits of the Functional Approach

1. **Predictability**: Functions behave consistently with the same inputs
2. **Testability**: Pure functions are easier to test in isolation
3. **Reasoning**: Code flow is more explicit and easier to follow
4. **Maintenance**: Immutability reduces bugs from unexpected state changes
5. **Composability**: Functions can be more easily composed together