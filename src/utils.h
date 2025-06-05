#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <time.h>
#include <stdbool.h>

typedef struct {
    char* name;
    double size_kb;
    time_t modified_time;
    bool is_dir;
} file_t; // apparently when you define a struct you should add _t at the end of the name

/**
 * Gets files from a directory
 * @param directory Path to the directory
 * @param file_count Pointer to store the number of files found
 * @return Array of file_t structures (must be freed by the caller)
 */
file_t* get_files_in_directory(const char* directory, size_t* file_count);

//
// These two function create copies and I still don't fully know why, but it doesn't work without it
//

/**
 * Creates a deep copy of a file_t structure
 * @param src Source file_t to copy
 * @return Pointer to a newly allocated file_t structure (must be freed by caller)
 */
file_t* copy_file_data(const file_t* src);

/**
 * Frees memory allocated for a file_t structure created with copy_file_data
 * @param file Pointer to the file_t structure to free
 */
void free_file_data(file_t* file);

#endif //UTILS_H
