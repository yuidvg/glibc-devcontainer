#pragma once
#include "types.h"

// syscall wrappers
AllocResult allocateMemory(const size_t size);
bool unallocateMemory(void *const ptr, const size_t size);

// utils


// variables
extern PreallocatedZones *preallocatedZones;
