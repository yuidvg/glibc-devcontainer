#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Starting malloc test program\n");

    // Allocate memory using malloc
    printf("Allocating 100 bytes with malloc\n");
    void *ptr = malloc(100);
    if (ptr == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    // Use the allocated memory
    printf("Memory allocated at address: %p\n", ptr);
    memset(ptr, 'A', 100);
    printf("Memory filled with 'A' characters\n");

    // Free the memory
    printf("Freeing allocated memory\n");
    free(ptr);

    // Allocate different sizes to see different malloc behavior
    printf("\nAllocating different sizes:\n");
    
    void *ptr1 = malloc(16);
    printf("16 bytes allocated at: %p\n", ptr1);
    
    void *ptr2 = malloc(1024);
    printf("1024 bytes allocated at: %p\n", ptr2);
    
    void *ptr3 = malloc(1024 * 1024);
    printf("1MB allocated at: %p\n", ptr3);

    // Free all allocations
    free(ptr1);
    free(ptr2);
    free(ptr3);
    
    printf("Test completed successfully\n");
    return 0;
} 