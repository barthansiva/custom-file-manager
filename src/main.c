#include <gtk/gtk.h>
#include "utils.h"
#include "ui_builder.h"
#include <stdlib.h>

GtkWidget *main_file_container;
char *current_directory = NULL;

// Function declarations
void file_clicked(GtkGridView* view, guint position, gpointer user_data);

/**
 * Totally not AI-generated code documentation
 * Populates the file_container with widgets for files in the specified directory
 * @param directory Path to the directory to display
 * @return TRUE if successful, FALSE otherwise
 */
gboolean populate_files(const char* directory) {

    if (current_directory != NULL) {
        free(current_directory);
    }

    current_directory = strdup(directory);

    // Clear the old views
    GtkWidget* old_child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(main_file_container));
    if (old_child) {
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(main_file_container), NULL);
    }

    // Read all files in the specified directory
    size_t file_count = 0; //The file count in case we need it later
    GListStore *files = get_files_in_directory(directory, &file_count);

    if (files == NULL) {
        g_print("No files found in directory: %s\n", directory);
        return FALSE; // No files found
    }

    GtkMultiSelection* selection = gtk_multi_selection_new(G_LIST_MODEL(files));
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_file_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_file_item), NULL);

    GtkWidget* view = gtk_grid_view_new(GTK_SELECTION_MODEL(selection), factory);

    gtk_grid_view_set_single_click_activate(GTK_GRID_VIEW(view),FALSE);
    g_signal_connect(view, "activate", G_CALLBACK(file_clicked), files);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(main_file_container), view);

    // Release our reference to the factory
    g_object_unref(factory);

    return TRUE;
}

/**
 * Creates the core widget structure for the app
 * @return GtkWidget* containing the side box
 */
static void init(GtkApplication *app, gpointer user_data) {

    GtkWidget *window;
    GtkWidget *hpaned;
    GtkWidget *side_box;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Manager"); //Really original title
    gtk_window_set_default_size(GTK_WINDOW(window), 1600, 900);

    // Main container
    hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    main_file_container = gtk_scrolled_window_new();

    side_box = create_side_box();

    gtk_paned_set_start_child (GTK_PANED (hpaned), side_box);
    gtk_paned_set_shrink_start_child (GTK_PANED (hpaned), FALSE);
    gtk_paned_set_resize_start_child (GTK_PANED (hpaned), FALSE);

    gtk_paned_set_end_child (GTK_PANED (hpaned), main_file_container);
    gtk_paned_set_shrink_end_child (GTK_PANED (hpaned), FALSE);

    gtk_window_set_child(GTK_WINDOW(window), hpaned);

    // Show the window
    gtk_window_present(GTK_WINDOW(window));

    populate_files("/home"); //Default directory, maybe make it configurable later
}


/**
 * Callback function for when a file is double-clicked in the grid view
 * @param view The GtkGridView that was clicked
 * @param position The position of the clicked item
 * @param user_data User data passed to the callback (in this case, the GListStore of files)
 */

void file_clicked(GtkGridView* view, guint position, gpointer user_data) {

    GListStore *files = G_LIST_STORE(user_data);
    GFile *file = g_list_model_get_item(G_LIST_MODEL(files), position);
    g_print("File %s clicked\n", g_file_get_basename(file));

    GFileInfo *info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    GFileType type = g_file_info_get_file_type(info);

    if (type == G_FILE_TYPE_DIRECTORY) {
        // If it's a directory, populate the file container with its contents
        populate_files(g_file_get_path(file));
    } else {
        // Print info for now
        g_print("File %s clicked\n", g_file_get_basename(file));
    }
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.fileman", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(init), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}


