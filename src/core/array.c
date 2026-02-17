//------------------------------------------------------------------------------
// Dynamic array implementation
//
// Copyright (C)2026 Matt Davies, all rights reserved
//------------------------------------------------------------------------------

#include <core/core.h>

//------------------------------------------------------------------------------

void* array_maybe_grow(void* array,
                       usize element_size,
                       usize required_capacity,
                       cstr  file,
                       int   line)
{
    if (!array) {
        // Initial allocation
        usize initial_capacity = 4;
        if (required_capacity > initial_capacity) {
            initial_capacity = required_capacity;
        }

        ArrayHeader* header = (ArrayHeader*)mem_alloc(
            sizeof(ArrayHeader) + initial_capacity * element_size, file, line);
        header->count = 0; // No elements yet

        return (void*)(header + 1);
    }

    // Calculate current capacity from memory size
    ArrayHeader* header             = (ArrayHeader*)array - 1;
    usize current_capacity_bytes    = mem_size(header) - sizeof(ArrayHeader);
    usize current_capacity_elements = current_capacity_bytes / element_size;

    if (required_capacity <= current_capacity_elements) {
        return array; // No growth needed
    }

    // Need to grow the array
    usize new_capacity_elements = current_capacity_elements * 2;
    if (new_capacity_elements < required_capacity) {
        new_capacity_elements = required_capacity;
    }

    usize        new_capacity_bytes = new_capacity_elements * element_size;
    ArrayHeader* old_header         = header;
    ArrayHeader* new_header         = (ArrayHeader*)mem_realloc(
        old_header, sizeof(ArrayHeader) + new_capacity_bytes, file, line);

    // count is preserved by realloc

    return (void*)(new_header + 1);
}
