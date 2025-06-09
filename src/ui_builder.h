#ifndef UI_BUILDER_H
#define UI_BUILDER_H
#define SPACING 7

#include <gtk/gtk.h>
#include "main.h"

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
GtkWidget* create_left_box();

/**
 * Struct to hold the toolbar widgets
 */
typedef struct {
    GtkWidget* toolbar;
    GtkWidget* up_button;
    GtkWidget* directory_entry;
    GtkWidget* search_entry;
} Toolbar;

// Logic for the collapsable directory structures
static GtkWidget* create_directory_row(GFile *file);
static void on_directory_row_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

void set_context(TabContext* ctx);

/**
 * Creates the toolbar widget structure with up button, directory entry and search entry
 * @param default_directory The default directory to display in the entry
 * @return A struct containing the toolbar widgets
 */
Toolbar create_toolbar(const char* default_directory);

#endif //UI_BUILDER_H
