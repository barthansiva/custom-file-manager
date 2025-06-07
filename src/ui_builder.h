#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include <gtk/gtk.h>
#include "utils.h"

/**
 * Placeholder for a function that handles file clicks (it's passed as a parameter)
 */
typedef void (*FileClickHandler)(GtkWidget *widget, gpointer file_data);

/**
 * Sets up a list item factory for displaying files in a list
 * @param factory The GtkListItemFactory to set up
 * @param list_item The GtkListItem to set up
 */
void setup_file_item(GtkListItemFactory *factory, GtkListItem *list_item);

/**
 * Populates a list item factory for displaying files in a list
 * @param factory The GtkListItemFactory to set up
 * @param list_item The GtkListItem to set up
 * @param click_handler Function to call when a file button is clicked
 */
void bind_file_item(GtkListItemFactory *factory, GtkListItem *list_item);

/**
 * Creates the widget structure for the side panel
 * @return GtkWidget* containing the side panel
 */
GtkWidget* create_side_box(void);

#endif //UI_BUILDER_H
