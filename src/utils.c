#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>

// Operation history
GArray *operation_history = NULL;
const int MAX_HISTORY_SIZE = 50; // Maximum number of operations to store in history

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

/**
 * Initialize the operation history system
 * Should be called once at program start
 */
void init_operation_history(void) {
    if (!operation_history) {
        operation_history = g_array_new(FALSE, FALSE, sizeof(operation_t));
    }
}

/**
 * Clean up operation history system resources
 * Should be called before program exit
 */
void cleanup_operation_history(void) {
    if (operation_history) {
        // Free any operation data
        for (guint i = 0; i < operation_history->len; i++) {
            operation_t *op = &g_array_index(operation_history, operation_t, i);
            if (op->data) {
                g_free(op->data);
            }
        }
        g_array_free(operation_history, TRUE);
        operation_history = NULL;
    }
}

/**
 * Adds an operation to the operation history
 * @param operation The operation to add
 */
void add_operation_to_history(operation_t operation) {
    if (!operation_history) {
        // Ensure operation history is initialized
        init_operation_history();
    }

    // Add operation to history array
    g_array_append_val(operation_history, operation);

    // If we've exceeded the max size, remove the oldest item
    if (operation_history->len > MAX_HISTORY_SIZE) {
        // For the deleted operation, free any data if needed
        operation_t *oldest = &g_array_index(operation_history, operation_t, 0);
        if (oldest->data) {
            g_free(oldest->data);
        }

        g_array_remove_index(operation_history, 0);
    }
}

/**
 * Deletes a file at the given path and adds the operation to history
 * @param path Full path to the file to delete
 * @return TRUE if successful, FALSE otherwise
 */
gboolean delete_file(const char* path) {
    if (!path || strlen(path) == 0) {
        g_warning("Invalid path provided for deletion");
        return FALSE;
    }

    // Create a GFile for the path
    GFile *file = g_file_new_for_path(path);
    GError *error = NULL;

    // Check if the file exists
    if (!g_file_query_exists(file, NULL)) {
        g_warning("File does not exist: %s", path);
        g_object_unref(file);
        return FALSE;
    }

    // Make a copy of path for history data as the original path might be freed after deletion
    char *path_copy = g_strdup(path);

    g_print("Deleting file: %s\n", path_copy);

    // Attempt to delete the file
    gboolean success = g_file_delete(file, NULL, &error);
    if (!success) {
        g_warning("Failed to delete file: %s, error: %s", path, error ? error->message : "Unknown error");
        g_free(path_copy);
        if (error) g_error_free(error);
        g_object_unref(file);
        return FALSE;
    }

    // Free the GFile
    g_object_unref(file);

    // Create and add operation to history
    operation_t op = {
        .type = OPERATION_TYPE_DELETE,
        .data = path_copy  // Store the path for possible undo functionality
    };
    add_operation_to_history(op);

    return TRUE;
}

/**
 * Structure to hold both source and destination paths for move operations
 * This is stored in the operation history for possible undo functionality
 */
typedef struct {
    char *source_path;
    char *dest_path;
} move_paths_t;

/**
 * Moves or renames a file from source path to destination path
 * If both paths are in the same directory, it acts as rename
 * If paths are in different directories, it moves the file
 *
 * @param source_path Full path to the source file
 * @param dest_path Full path to the destination (new name or location)
 * @return TRUE if successful, FALSE otherwise
 */
gboolean move_file(const char* source_path, const char* dest_path) {
    if (!source_path || !dest_path || strlen(source_path) == 0 || strlen(dest_path) == 0) {
        g_warning("Invalid path provided for move operation");
        return FALSE;
    }

    // Create GFiles for source and destination
    GFile *source_file = g_file_new_for_path(source_path);
    GFile *dest_file = g_file_new_for_path(dest_path);
    GError *error = NULL;

    // Check if source file exists
    if (!g_file_query_exists(source_file, NULL)) {
        g_warning("Source file does not exist: %s", source_path);
        g_object_unref(source_file);
        g_object_unref(dest_file);
        return FALSE;
    }

    // Make copies of paths for history data
    move_paths_t *paths = g_malloc(sizeof(move_paths_t));
    paths->source_path = g_strdup(source_path);
    paths->dest_path = g_strdup(dest_path);

    // Perform the move operation
    // G_FILE_COPY_OVERWRITE flag will overwrite destination if it exists
    // G_FILE_COPY_ALL_METADATA ensures all metadata is preserved
    gboolean success = g_file_move(
        source_file,
        dest_file,
        G_FILE_COPY_OVERWRITE | G_FILE_COPY_ALL_METADATA,
        NULL, // Cancellable
        NULL, // Progress callback
        NULL, // Progress callback data
        &error
    );

    if (!success) {
        g_warning("Failed to move/rename file from %s to %s, error: %s",
                  source_path, dest_path, error ? error->message : "Unknown error");
        g_free(paths->source_path);
        g_free(paths->dest_path);
        g_free(paths);
        if (error) g_error_free(error);
        g_object_unref(source_file);
        g_object_unref(dest_file);
        return FALSE;
    }

    // Free GFiles
    g_object_unref(source_file);
    g_object_unref(dest_file);

    // Create and add operation to history
    operation_t op = {
        .type = OPERATION_TYPE_MOVE,
        .data = paths  // Store both paths for possible undo functionality
    };
    add_operation_to_history(op);

    return TRUE;
}

/**
 * Undoes a move operation by moving the file back to its original location
 * @param operation The operation to undo (must be OPERATION_TYPE_MOVE)
 * @return TRUE if successful, FALSE otherwise
 */
gboolean undo_move_file(operation_t operation) {
    // Verify this is a move operation
    if (operation.type != OPERATION_TYPE_MOVE) {
        g_warning("Cannot undo non-move operation with undo_move_file");
        return FALSE;
    }

    // Check if data is valid
    if (!operation.data) {
        g_warning("Invalid operation data for move undo");
        return FALSE;
    }

    // Extract the paths from operation data
    move_paths_t *paths = (move_paths_t *)operation.data;

    // For undo, we simply swap the source and destination
    // The original destination is now the source
    // The original source is now the destination
    gboolean success = move_file(paths->dest_path, paths->source_path);

    if (!success) {
        g_warning("Failed to undo move operation from %s to %s",
                  paths->dest_path, paths->source_path);
    }

    return success;
}

/**
 * Gets an array of currently selected items from a GtkGridView
 * @param view The GtkGridView to get selection from
 * @param count Pointer to store the number of selected items
 * @return Array of GFile objects (must be freed by the caller along with each GFile)
 */
GFile** get_selection(GtkGridView* view, size_t* count) {
    g_return_val_if_fail(view != NULL && count != NULL, NULL);

    // Initialize count to 0 in case of early return
    *count = 0;

    // Get the selection model
    GtkSelectionModel *selection_model = gtk_grid_view_get_model(view);
    if (!selection_model) {
        g_warning("Failed to get selection model from GtkGridView");
        return NULL;
    }

    // Get the bitset of selected items
    GtkBitset *selection = gtk_selection_model_get_selection(selection_model);
    if (!selection) {
        g_warning("Failed to get selection bitset from selection model");
        return NULL;
    }

    // Get the list model containing the actual items
    GListStore *files = G_LIST_STORE(gtk_multi_selection_get_model(GTK_MULTI_SELECTION(selection_model)));
    if (!files) {
        g_warning("Failed to get list store from selection model");
        gtk_bitset_unref(selection);
        return NULL;
    }

    // Count the number of selected items
    guint64 n_selected = gtk_bitset_get_size(selection);
    if (n_selected == 0) {
        gtk_bitset_unref(selection);
        return NULL;
    }

    // Allocate the array to hold the selected items
    GFile **selected_files = g_malloc_n(n_selected, sizeof(GFile *));
    if (!selected_files) {
        g_warning("Failed to allocate memory for selected files");
        gtk_bitset_unref(selection);
        return NULL;
    }

    // Iterate through selected positions and get corresponding files
    GtkBitsetIter iter;
    guint pos;
    size_t index = 0;

    if (gtk_bitset_iter_init_first(&iter, selection, &pos)) {
        do {
            // Get item at this position and add to our array
            GFile *file = g_list_model_get_item(G_LIST_MODEL(files), pos);
            if (file) {
                selected_files[index++] = file;
            }
        } while (gtk_bitset_iter_next(&iter, &pos));
    }

    // Store the actual count in case some items couldn't be retrieved
    *count = index;

    // Clean up
    gtk_bitset_unref(selection);

    return selected_files;
}

/**
 * Joins basenames from an array of GFile objects into a single string
 * The first element in the string is the parent directory path of the first file
 * @param files Array of GFile objects
 * @param count Number of files in the array
 * @param separator String to use as separator between path and basenames
 * @return A newly-allocated string containing parent path and joined basenames (must be freed by caller)
 */
char* join_basenames(GFile** files, size_t count, const char* separator) {
    g_return_val_if_fail(files != NULL && count > 0 && separator != NULL, NULL);

    // Initial string buffer
    GString *result = g_string_new(NULL);
    if (!result) {
        g_warning("Failed to allocate memory for joined basenames");
        return NULL;
    }

    // First, add the parent directory path of the first file
    if (files[0]) {
        GFile *parent = g_file_get_parent(files[0]);
        if (parent) {
            char *parent_path = g_file_get_path(parent);
            if (parent_path) {
                g_string_append(result, parent_path);
                g_free(parent_path);
            }
            g_object_unref(parent);

            // Add separator after the path
            g_string_append(result, separator);
        }
    }

    // Process each file
    for (size_t i = 0; i < count; i++) {
        if (files[i]) {
            // Get the basename of the file
            char *basename = g_file_get_basename(files[i]);
            if (basename) {
                // Add separator if not the first item
                if (i > 0) {
                    g_string_append(result, separator);
                }

                // Append basename
                g_string_append(result, basename);
                g_free(basename);
            }
        }
    }

    // Convert GString to a regular string and free GString
    char *joined = g_string_free(result, FALSE);
    return joined;
}

/**
 * Splits a string of basenames into an array of individual basename strings
 * @param joined_string The string containing joined basenames
 * @param separator String used as separator between basenames
 * @param count Pointer to store the number of resulting basenames
 * @return Array of strings (must be freed by caller along with each string)
 */
char** split_basenames(const char* joined_string, const char* separator, size_t* count) {
    g_return_val_if_fail(joined_string != NULL && separator != NULL && count != NULL, NULL);

    // Initialize count to 0 in case of early return
    *count = 0;

    // Empty string check
    if (strlen(joined_string) == 0) {
        return NULL;
    }

    // Split the string using GLib's g_strsplit function
    char **parts = g_strsplit(joined_string, separator, -1);
    if (!parts) {
        g_warning("Failed to split basename string");
        return NULL;
    }

    // Count the number of resulting strings
    size_t num_parts = 0;
    for (char **p = parts; *p != NULL; p++) {
        num_parts++;
    }

    *count = num_parts;
    return parts;
}
