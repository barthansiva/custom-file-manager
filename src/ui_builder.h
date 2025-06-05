#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include <gtk/gtk.h>
#include "utils.h"

/**
 * Placeholder for a function that handles file clicks (it's passed as a parameter)
 */
typedef void (*FileClickHandler)(GtkWidget *widget, gpointer file_data);

/**
 * Creates GTK widgets for an array of files
 * @param files Array of file_t structures
 * @param file_count Number of files in the array
 * @param click_handler Function to call when a file is clicked
 * @return Array of GtkWidget pointers (must be freed by the caller)
 */
GtkWidget** create_file_widgets(file_t* files, size_t file_count, FileClickHandler click_handler);

#endif //UI_BUILDER_H
