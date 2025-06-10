#ifndef UI_BUILDER_H
#define UI_BUILDER_H
#define SPACING 7

#include <gtk/gtk.h>
#include "main.h"

/**
 * Struct to hold the toolbar widgets
 */
typedef struct {
    GtkWidget* toolbar;
    GtkWidget* up_button;
    GtkWidget* directory_entry;
    GtkWidget* search_entry;
} toolbar_t;

/**
 * Struct to hold the left box widgets
 * This contains the side panel with undo/redo buttons
 */
typedef struct {
    GtkWidget* side_panel;
    GtkButton* undo_button;
    GtkButton* redo_button;
} left_box_t;

/**
 * Struct to hold the dialog widgets
 * This contains the dialog window and entry for user input
 */
typedef struct {
    GtkWindow* dialog;
    GtkEntry* entry;
} dialog_t;

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
left_box_t create_left_box();


// Logic for the collapsable directory structures
static GtkWidget* create_directory_row(GFile *file);
static void on_directory_row_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

void set_context(TabContext* ctx);

/**
 * Creates the toolbar widget structure with up button, directory entry and search entry
 * @param default_directory The default directory to display in the entry
 * @return A struct containing the toolbar widgets
 */
toolbar_t create_toolbar(const char* default_directory);

GtkPopoverMenu* create_file_context_menu(const char* params);

GtkPopoverMenu* create_directory_context_menu(const char* params);

dialog_t create_dialog(const char* title, const char* message);

GtkWindow* create_properties_window(const char* file_path);

#endif //UI_BUILDER_H
