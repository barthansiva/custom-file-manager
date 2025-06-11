#ifndef MAIN_H
#define MAIN_H

#include <gtk/gtk.h>

/**
 * @brief Holds state and widgets related to a single notebook tab.
 *
 * Each tab in the file manager has its own file view, preview pane, and
 * sorting model. This struct is used to store and retrieve per-tab data.
 */
typedef struct {
    GtkWidget *scrolled_window;
    GtkWidget *tab_label;
    char *current_directory;
    GtkWidget *preview_text_view;
    GtkWidget *preview_revealer;
    GtkGridView *file_grid_view;
    GtkSortListModel *sort_model;
    GListStore *file_store;
} TabContext;

/**
 * @brief Opens a file using the system's default application.
 *
 * @param file GFile pointing to the file to open.
 */
void open_file_with_default_app(GFile *file);

/**
 * @brief Retrieves the TabContext of the currently active tab.
 *
 * @return Pointer to TabContext or NULL if no tab is active.
 */
TabContext* get_current_tab_context();

/**
 * @brief Populates the given container with the contents of a directory.
 *
 * Sets up a new GtkGridView for the files and updates the associated tab state.
 *
 * @param directory Directory path to list files from.
 * @param container GtkScrolledWindow to host the file grid.
 * @param ctx Pointer to the TabContext associated with the container.
 */
void populate_files_in_container(const char *directory, GtkWidget *container, TabContext *ctx);

/**
 * @brief Handles right-clicks on individual file items.
 *
 * Displays a context menu for the selected file(s), positioned at the click location.
 *
 * @param gesture The GtkGestureClick event.
 * @param n_press Number of presses (unused).
 * @param x X coordinate of the click.
 * @param y Y coordinate of the click.
 * @param user_data The widget that was clicked (usually a GtkBox).
 */
void file_right_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

void reload_current_directory();

#endif //MAIN_H
