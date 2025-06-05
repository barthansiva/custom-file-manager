#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Gets files from a directory
 * @param directory Path to the directory
 * @param file_count Pointer to store the number of files found
 * @return Array of file_t structures (must be freed by the caller)
 */
file_t* get_files_in_directory(const char* directory, size_t* file_count) {
    DIR* dir;
    struct dirent* entry;
    struct stat st;
    size_t count = 0;
    size_t capacity = 10;  // random ahh guess
    file_t* files = malloc(capacity * sizeof(file_t));

    // Open directory
    if ((dir = opendir(directory)) == NULL) {
        perror("Failed to open directory");
        return NULL;
    }

    char path[1024];

    // Loop over all of the files/directories in the directory
    while ((entry = readdir(dir)) != NULL) {

        //Skip "." and ".." directories, I have no clue what those are
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Get full path of the directory
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        // Get file statistics
        if (stat(path, &st) == -1) {
            perror("No file stats");
            continue;
        }

        // Resize when we reach capacity
        if (count >= capacity) {
            capacity *= 2;
            file_t* temp = realloc(files, capacity * sizeof(file_t));
            files = temp;
        }

        // Populate all of the stats

        files[count].name = strdup(entry->d_name);
        files[count].size_kb = (double)st.st_size / 1024.0;
        files[count].modified_time = st.st_mtime;
        files[count].is_dir = S_ISDIR(st.st_mode);

        count++;
    }

    closedir(dir);
    *file_count = count;

    // If directory empty
    if (count == 0) {
        free(files);
        return NULL;
    }

    // Maybe we overshot the capacity, so we can shrink it
    if (count < capacity) {
        file_t* temp = realloc(files, count * sizeof(file_t));
        if (temp) {
            files = temp;
        }
    }

    return files;
}

//
// These two function create copies and I still don't fully know why, but it doesn't work without it
// 100 % AI generated code
//

/**
 * Creates a deep copy of a file_t structure
 * @param src Source file_t to copy
 * @return Pointer to a newly allocated file_t structure (must be freed by caller)
 */
file_t* copy_file_data(const file_t* src) {
    if (src == NULL) {
        return NULL;
    }

    // Allocate memory for the new structure
    file_t* dst = (file_t*) malloc(sizeof(file_t));
    if (dst == NULL) {
        return NULL;
    }

    // Copy scalar values
    dst->size_kb = src->size_kb;
    dst->modified_time = src->modified_time;
    dst->is_dir = src->is_dir;

    // Deep copy the name string
    if (src->name != NULL) {
        dst->name = strdup(src->name);
        if (dst->name == NULL) {
            // Failed to allocate memory for name
            free(dst);
            return NULL;
        }
    } else {
        dst->name = NULL;
    }

    return dst;
}

/**
 * Frees memory allocated for a file_t structure created with copy_file_data
 * @param file Pointer to the file_t structure to free
 */
void free_file_data(file_t* file) {
    if (file == NULL) {
        return;
    }

    // Free the name string
    free(file->name);

    // Free the structure itself
    free(file);
}
