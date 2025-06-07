#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <gtk/gtk.h>

/**
 * Gets files from a directory
 * @param directory Path to the directory
 * @param file_count Pointer to store the number of files found
 * @return Array of file_t structures (must be freed by the caller)
 */
GListStore* get_files_in_directory(const char* directory, size_t* file_count);

#endif //UTILS_H
