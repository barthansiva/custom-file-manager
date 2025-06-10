#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>

// Operation history
GArray *operation_history = NULL;
// Forward history (for redoing undone operations)
GArray *forward_history = NULL;
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

    // // If directory empty
    // if (count == 0) {
    //     g_object_unref(files);
    //     return NULL;
    // }

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
    if (!forward_history) {
        forward_history = g_array_new(FALSE, FALSE, sizeof(operation_t));
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

    if (forward_history) {
        // Free any operation data
        for (guint i = 0; i < forward_history->len; i++) {
            operation_t *op = &g_array_index(forward_history, operation_t, i);
            if (op->data) {
                g_free(op->data);
            }
        }
        g_array_free(forward_history, TRUE);
        forward_history = NULL;
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
    gboolean success = g_file_trash(file, NULL, &error);
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
 * @param files Array of GFile objects
 * @param count Number of files in the array
 * @param separator String to use as separator between basenames
 * @param file_path Path to use as the first element in the returned string
 * @return A newly-allocated string containing the provided path and joined basenames (must be freed by caller)
 */
char* join_basenames(GFile** files, size_t count, const char* separator, const char* file_path) {
    g_return_val_if_fail(files != NULL && count > 0 && separator != NULL, NULL);

    // Initial string buffer
    GString *result = g_string_new(NULL);
    if (!result) {
        g_warning("Failed to allocate memory for joined basenames");
        return NULL;
    }

    // First, add the provided file path
    if (file_path) {
        g_string_append(result, file_path);

        // Add separator after the path
        g_string_append(result, separator);
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

/**
 * Undoes a delete operation by restoring the file from trash
 * @param operation The operation to undo (must be OPERATION_TYPE_DELETE)
 * @return TRUE if successful, FALSE otherwise
 */
gboolean undo_delete_file(operation_t operation) {
    if (operation.type != OPERATION_TYPE_DELETE) {
        g_warning("Attempted to undo non-delete operation");
        return FALSE;
    }

    // The operation data contains the original path of the deleted file
    const char *orig_path = (const char *)operation.data;
    if (!orig_path) {
        g_warning("Invalid operation data for undo delete");
        return FALSE;
    }

    // Get the trash directory
    g_autoptr(GFile) trash_dir = g_file_new_for_uri("trash:///");

    // Enumerate files in the trash to find our file
    g_autoptr(GFileEnumerator) enumerator =
        g_file_enumerate_children(trash_dir,
                                 G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_TRASH_ORIG_PATH,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL, NULL);

    if (!enumerator) {
        g_warning("Could not access trash directory");
        return FALSE;
    }

    // Find the file in trash that matches our original path
    g_autoptr(GFile) trashed_file = NULL;
    g_autoptr(GFileInfo) info;

    while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) {
        g_autofree char *trash_orig_path =
            g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);

        if (g_strcmp0(trash_orig_path, orig_path) == 0) {
            // Found our file
            const char *name = g_file_info_get_name(info);
            trashed_file = g_file_get_child(trash_dir, name);
            g_object_unref(info);
            break;
        }

        g_object_unref(info);
    }

    if (!trashed_file) {
        g_warning("Could not find the deleted file in trash: %s", orig_path);
        return FALSE;
    }

    // Create the target file
    g_autoptr(GFile) target = g_file_new_for_path(orig_path);

    // Move the file from trash back to original location
    g_autoptr(GError) error = NULL;
    gboolean success = g_file_move(trashed_file,
                                  target,
                                  G_FILE_COPY_NONE,
                                  NULL, NULL, NULL,
                                  &error);

    if (!success) {
        g_warning("Failed to restore file from trash: %s", error ? error->message : "Unknown error");
        return FALSE;
    }

    g_print("Successfully restored file: %s\n", orig_path);
    return TRUE;
}

/**
 * Undoes the given operation by calling the appropriate undo function
 * @param operation The operation to undo
 * @return TRUE if successful, FALSE otherwise
 */
gboolean undo_operation(operation_t operation) {
    // Handle operation based on its type
    switch (operation.type) {
        case OPERATION_TYPE_MOVE:
            return undo_move_file(operation);

        case OPERATION_TYPE_DELETE:
            return undo_delete_file(operation);

        case OPERATION_TYPE_COPY:
            g_print("Undo for copy operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_PASTE:
            g_print("Undo for paste operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_CREATE_DIRECTORY:
            g_print("Undo for create directory operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_CREATE_FILE:
            g_print("Undo for create file operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_NONE:
        default:
            g_warning("Cannot undo operation of unknown or invalid type: %d", operation.type);
            return FALSE;
    }
}

/**
 * Undoes the last operation in the history and stores it in forward history for possible redo
 * @return TRUE if successful, FALSE if no operations to undo
 */
gboolean undo_last_operation(void) {
    // Check if there are any operations to undo
    if (!operation_history || operation_history->len == 0) {
        g_print("No operations to undo\n");
        return FALSE;
    }

    // Get the last operation from the history
    operation_t *last_op = &g_array_index(operation_history, operation_t, operation_history->len - 1);

    // Make a copy of the operation for forward history
    operation_t forward_op;
    forward_op.type = last_op->type;

    // Deep copy of operation data based on type
    switch (last_op->type) {
        case OPERATION_TYPE_DELETE:
            // For delete operations, data is a string (path)
            forward_op.data = g_strdup((char *)last_op->data);
            break;

        case OPERATION_TYPE_MOVE:
            // For move operations, data is a move_paths_t struct
            {
                move_paths_t *orig_paths = (move_paths_t *)last_op->data;
                move_paths_t *new_paths = g_malloc(sizeof(move_paths_t));
                new_paths->source_path = g_strdup(orig_paths->source_path);
                new_paths->dest_path = g_strdup(orig_paths->dest_path);
                forward_op.data = new_paths;
            }
            break;

        case OPERATION_TYPE_COPY:
        case OPERATION_TYPE_PASTE:
        case OPERATION_TYPE_CREATE_DIRECTORY:
        case OPERATION_TYPE_CREATE_FILE:
        case OPERATION_TYPE_NONE:
            // For other operations, just copy the pointer (or NULL)
            forward_op.data = last_op->data;
            break;
    }

    // Try to undo the operation
    gboolean success = undo_operation(*last_op);

    if (success) {
        // Add the operation to forward history for possible redo
        if (!forward_history) {
            forward_history = g_array_new(FALSE, FALSE, sizeof(operation_t));
        }
        g_array_append_val(forward_history, forward_op);

        // Remove the operation from the history
        // Note: we don't free data because we moved it to forward_op
        g_array_remove_index(operation_history, operation_history->len - 1);

        g_print("Operation undone successfully\n");
    } else {
        // Free the copied data if undo failed
        if (forward_op.data) {
            switch (forward_op.type) {
                case OPERATION_TYPE_DELETE:
                    g_free(forward_op.data);
                    break;

                case OPERATION_TYPE_MOVE:
                    {
                        move_paths_t *paths = (move_paths_t *)forward_op.data;
                        g_free(paths->source_path);
                        g_free(paths->dest_path);
                        g_free(paths);
                    }
                    break;

                default:
                    break;
            }
        }

        g_print("Failed to undo operation\n");
    }

    return success;
}

/**
 * Redoes the last undone operation by applying it again
 * @return TRUE if successful, FALSE if no operations to redo
 */
gboolean redo_last_undo(void) {
    // Check if there are any operations to redo
    if (!forward_history || forward_history->len == 0) {
        g_print("No operations to redo\n");
        return FALSE;
    }

    // Get the last operation from the forward history
    operation_t *fwd_op = &g_array_index(forward_history, operation_t, forward_history->len - 1);

    // Create a new operation for the history
    operation_t history_op;
    history_op.type = fwd_op->type;

    // Deep copy of operation data based on type
    switch (fwd_op->type) {
        case OPERATION_TYPE_DELETE: {
            // For redo of delete, we need to delete the file again
            const char *path = (const char *)fwd_op->data;
            gboolean success = delete_file(path);

            if (!success) {
                g_warning("Failed to redo delete operation for file: %s", path);
                return FALSE;
            }

            // delete_file already adds this to the history, so return immediately
            // Also remove the forward operation
            g_array_remove_index(forward_history, forward_history->len - 1);
            g_print("Delete operation redone successfully\n");
            return TRUE;
        }

        case OPERATION_TYPE_MOVE: {
            // For redo of move, we need to move the file again
            move_paths_t *paths = (move_paths_t *)fwd_op->data;

            // Move the file (original source to dest)
            gboolean success = move_file(paths->source_path, paths->dest_path);

            if (!success) {
                g_warning("Failed to redo move operation from %s to %s",
                          paths->source_path, paths->dest_path);
                return FALSE;
            }

            // move_file already adds this to the history, so return immediately
            // Also remove the forward operation
            g_array_remove_index(forward_history, forward_history->len - 1);
            g_print("Move operation redone successfully\n");
            return TRUE;
        }

        case OPERATION_TYPE_COPY:
            g_print("Redo for copy operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_PASTE:
            g_print("Redo for paste operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_CREATE_DIRECTORY:
            g_print("Redo for create directory operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_CREATE_FILE:
            g_print("Redo for create file operation is not yet implemented\n");
            return FALSE;

        case OPERATION_TYPE_NONE:
        default:
            g_warning("Cannot redo operation of unknown or invalid type: %d", fwd_op->type);
            return FALSE;
    }
}

/**
 * Gets the directory path component of a file path
 * @param file_path Full path to a file
 * @return A newly-allocated string containing the directory path (must be freed by caller)
 */
char* get_directory(const char* file_path) {
    if (file_path == NULL) {
        return NULL;
    }

    // Create a GFile object for the provided path
    GFile* file = g_file_new_for_path(file_path);
    if (!file) {
        return g_strdup(".");
    }

    // Get the parent directory as a GFile
    GFile* parent = g_file_get_parent(file);

    // If there's no parent (e.g., for "/" or a relative path with no directory component)
    if (!parent) {
        g_object_unref(file);
        return g_strdup(".");
    }

    // Get the path of the parent directory
    char* directory = g_file_get_path(parent);

    // Clean up GFile objects
    g_object_unref(file);
    g_object_unref(parent);

    // If we couldn't get the path for some reason
    if (!directory) {
        return g_strdup(".");
    }

    return directory;
}

/**
 * Gets the basename (filename) component of a file path
 * @param file_path Full path to a file
 * @return A newly-allocated string containing the basename (must be freed by caller)
 */
char* get_basename(const char* file_path) {
    if (file_path == NULL) {
        return NULL;
    }

    // Find the last occurrence of '/'
    const char* last_slash = strrchr(file_path, '/');

    // If there's no slash, the entire path is the basename
    if (last_slash == NULL) {
        return g_strdup(file_path);
    }

    // Otherwise, return everything after the last slash
    return g_strdup(last_slash + 1);
}
