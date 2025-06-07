#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>

/**
 * Gets files from a directory
 * @param directory Path to the directory
 * @param file_count Pointer to store the number of files found
 * @return GListStore of GFile objects representing the files/directories
 */
GListStore* get_files_in_directory(const char* directory, size_t* file_count) {
    DIR* dir;
    struct dirent* entry;
    size_t count = 0;

    GListStore* files = g_list_store_new(G_TYPE_FILE);

    // Open directory
    if ((dir = opendir(directory)) == NULL) {
        perror("Failed to open directory");
        g_object_unref(files);  // Clean up if we can't open the directory
        return NULL;
    }

    // Loop over all of the files/directories in the directory
    while ((entry = readdir(dir)) != NULL) {

        //Skip "." and ".." directories (idk what those are)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Get full path of the file/directory
        char path[1024];
        if (directory[strlen(directory) - 1] == '/') {
            // Directory already ends with slash
            snprintf(path, sizeof(path), "%s%s", directory, entry->d_name);
        } else {
            // Need to add a slash
            snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);
        }

        // Create a GFile for this path
        GFile* file = g_file_new_for_path(path);
        if (file) {
            // Append it to our list store
            g_list_store_append(files, file);
            g_object_unref(file);  // The list store now owns a reference
            count++;
        }
    }

    closedir(dir);
    *file_count = count;

    // If directory empty
    if (count == 0) {
        g_object_unref(files);
        return NULL;
    }

    return files;
}
