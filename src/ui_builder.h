#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include <gtk/gtk.h>
#include "utils.h"

/**
 * Sets up a list item factory for displaying files in a list.
 * This function is called when the factory is created.
 * It sets up the structure of each file item in the list. (only the structure, not the data)
 *
 * @param factory The GtkListItemFactory to set up
 * @param list_item The GtkListItem to set up
 */
void setup_file_item(GtkListItemFactory *factory, GtkListItem *list_item);

/**
 * Binds data to a list item factory for displaying files in a list.
 * This function is called when the factory is used to create a list item.
 * It updates the contents of the file item with the actual file data. (that way the same structure can be reused for different files)
 *
 * @param factory The GtkListItemFactory to set up
 * @param list_item The GtkListItem to set up
 */
void bind_file_item(GtkListItemFactory *factory, GtkListItem *list_item);

/**
 * Creates the widget structure for the side panel
 * @return GtkWidget* containing the side panel
 */
GtkWidget* create_side_box(void);

#endif //UI_BUILDER_H
