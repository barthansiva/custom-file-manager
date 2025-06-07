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
 * Frees memory allocated for a generic data pointer
 * @param data Pointer to the data to free
 */
void free_data(const gpointer data);

/**
 * Connects a destroy signal to a widget to free the associated data
 * @param widget The widget to connect the signal to
 * @param data Pointer to the data to free when the widget is destroyed
 */
void free_on_destroy(GtkWidget* widget, gpointer data);

#endif //UTILS_H
