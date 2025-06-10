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

/**
 * Gets an array of currently selected items from a GtkGridView
 * @param view The GtkGridView to get selection from
 * @param count Pointer to store the number of selected items
 * @return Array of GFile objects (must be freed by the caller along with each GFile)
 */
GFile** get_selection(GtkGridView* view, size_t* count);

/**
 * Joins basenames from an array of GFile objects into a single string
 * @param files Array of GFile objects
 * @param count Number of files in the array
 * @param separator String to use as separator between basenames
 * @param file_path Path to use as the first element in the returned string
 * @return A newly-allocated string containing the provided path and joined basenames (must be freed by caller)
 */
char* join_basenames(GFile** files, size_t count, const char* separator, const char* file_path);

/**
 * Splits a string of basenames into an array of individual basename strings
 * @param joined_string The string containing joined basenames
 * @param separator String used as separator between basenames
 * @param count Pointer to store the number of resulting basenames
 * @return Array of strings (must be freed by caller along with each string)
 */
char** split_basenames(const char* joined_string, const char* separator, size_t* count);

enum OPERATION_TYPE {

    OPERATION_TYPE_NONE, // No operation just in case
    OPERATION_TYPE_COPY, // The rest is self-explanatory
    OPERATION_TYPE_PASTE,
    OPERATION_TYPE_MOVE,
    OPERATION_TYPE_DELETE,
    OPERATION_TYPE_CREATE_DIRECTORY,
    OPERATION_TYPE_CREATE_FILE
};

// Operation structure for handling history
typedef struct {
    enum OPERATION_TYPE type; // Type of operation
    gpointer data; // Data associated with the operation, can be anything
} operation_t;

/**
 * Deletes a file at the given path and adds the operation to history
 * @param path Full path to the file to delete
 * @return TRUE if successful, FALSE otherwise
 */
gboolean delete_file(const char* path);

/**
 * Moves or renames a file from source path to destination path
 * If both paths are in the same directory, it acts as rename
 * If paths are in different directories, it moves the file
 *
 * @param source_path Full path to the source file
 * @param dest_path Full path to the destination (new name or location)
 * @return TRUE if successful, FALSE otherwise
 */
gboolean move_file(const char* source_path, const char* dest_path);

/**
 * Undoes a move operation by moving the file back to its original location
 * @param operation The operation to undo (must be OPERATION_TYPE_MOVE)
 * @return TRUE if successful, FALSE otherwise
 */
gboolean undo_move_file(operation_t operation);

/**
 * Undoes a delete operation by restoring the file from trash
 * @param operation The operation to undo (must be OPERATION_TYPE_DELETE)
 * @return TRUE if successful, FALSE otherwise
 */
gboolean undo_delete_file(operation_t operation);

/**
 * Undoes the given operation by calling the appropriate undo function
 * @param operation The operation to undo
 * @return TRUE if successful, FALSE otherwise
 */
gboolean undo_operation(operation_t operation);

/**
 * Undoes the last operation in the history and stores it in forward history for possible redo
 * @return TRUE if successful, FALSE if no operations to undo
 */
gboolean undo_last_operation(void);

/**
 * Redoes the last undone operation by applying it again
 * @return TRUE if successful, FALSE if no operations to redo
 */
gboolean redo_last_undo(void);

/**
 * Adds an operation to the operation history
 * @param operation The operation to add
 */
void add_operation_to_history(operation_t operation);

/**
 * Initialize the operation history system
 * Should be called once at program start
 */
void init_operation_history();

/**
 * Gets the directory path component of a file path
 * @param file_path Full path to a file
 * @return A newly-allocated string containing the directory path (must be freed by caller)
 */
char* get_directory(const char* file_path);

/**
 * Gets the basename (filename) component of a file path
 * @param file_path Full path to a file
 * @return A newly-allocated string containing the basename (must be freed by caller)
 */
char* get_basename(const char* file_path);

/**
 * Clean up operation history system resources
 * Should be called before program exit
 */
void cleanup_operation_history();

#endif //UTILS_H
