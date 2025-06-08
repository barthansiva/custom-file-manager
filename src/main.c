#include <gtk/gtk.h>
#include "utils.h"
#include "ui_builder.h"
#include <stdlib.h>

GtkWidget *main_file_container;
GtkWidget *directory_entry;
const char *default_directory = "/home"; // Default directory to start in
char *current_directory = NULL;

// Function declarations
void file_clicked(GtkGridView *view, guint position, gpointer user_data);

void directory_entry_changed(  GtkEntry* self, gpointer user_data);

void go_up_a_directory();

/**
 * Totally not AI-generated code documentation
 * Populates the file_container with widgets for files in the specified directory
 * @param directory Path to the directory to display
 * @return TRUE if successful, FALSE otherwise
 */
gboolean populate_files(const char *directory) {
    if (current_directory != NULL) {
        free(current_directory);
    }

    current_directory = strdup(directory);

    // Read all files in the specified directory
    size_t file_count = 0; //The file count in case we need it later
    GListStore *files = get_files_in_directory(directory, &file_count);

    if (files == NULL) {
        g_print("No files found in directory: %s\n", directory);
        return FALSE; // No files found
    }

    GtkMultiSelection *selection = gtk_multi_selection_new(G_LIST_MODEL(files));
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_file_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_file_item), NULL);

    // Get rid of the old view if it exists
    GtkWidget *old_view = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(main_file_container));
    if (old_view) {
        // Disconnect the signal handler for the old view
        g_signal_handlers_disconnect_matched(old_view, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(file_clicked), NULL);

        g_object_ref(old_view); // Apparently we need to hold a reference to it before unparenting
        gtk_widget_unparent(old_view);
    }

    // Create new view
    GtkWidget *view = gtk_grid_view_new(GTK_SELECTION_MODEL(selection), factory);
    gtk_grid_view_set_single_click_activate(GTK_GRID_VIEW(view), FALSE);
    g_signal_connect(view, "activate", G_CALLBACK(file_clicked), files);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(main_file_container), view);

    // Now it's safe to release our reference to the old view if we had one
    if (old_view) {
        g_object_unref(old_view);
    }
    g_object_unref(factory);

    // Update the directory entry
    gtk_editable_set_text(GTK_EDITABLE(directory_entry), current_directory);

    return TRUE;
}

/**
 * Builds the core widget structure of the application
 * @param app The GtkApplication instance
 * @param user_data User data passed to the callback (not used here)
 */
static void init(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Manager"); //Really original title
    gtk_window_set_default_size(GTK_WINDOW(window), 1600, 900);

    //
    //  Left side panel
    //
    GtkWidget *left_box = create_left_box();

    //
    //  Main right container
    //
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING);
    gtk_widget_set_margin_top(right_box, SPACING);
    gtk_widget_set_margin_bottom(right_box, SPACING);
    gtk_widget_set_margin_start(right_box, SPACING);
    gtk_widget_set_margin_end(right_box, SPACING);

    //
    // Toolbar
    //
    Toolbar toolbar = create_toolbar(default_directory);

    // Store directory_entry in the global variable
    directory_entry = toolbar.directory_entry;

    // Connect signals
    g_signal_connect(toolbar.up_button, "clicked", G_CALLBACK(go_up_a_directory), NULL);

    g_signal_connect(directory_entry, "icon-press", G_CALLBACK(directory_entry_changed), NULL);
    g_signal_connect(directory_entry, "activate", G_CALLBACK(directory_entry_changed), NULL);

    //
    // File Container
    //
    main_file_container = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(main_file_container, TRUE);

    gtk_box_append(GTK_BOX(right_box), toolbar.toolbar);
    gtk_box_append(GTK_BOX(right_box), main_file_container);

    //
    //  Main container
    //
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    gtk_paned_set_start_child(GTK_PANED(hpaned), left_box);
    gtk_paned_set_shrink_start_child(GTK_PANED(hpaned), FALSE);
    gtk_paned_set_resize_start_child(GTK_PANED(hpaned), FALSE);

    gtk_paned_set_end_child(GTK_PANED(hpaned), right_box);
    gtk_paned_set_shrink_end_child(GTK_PANED(hpaned), FALSE);

    gtk_window_set_child(GTK_WINDOW(window), hpaned);

    // Show the window
    gtk_window_present(GTK_WINDOW(window));

    populate_files(default_directory);
}


/**
 * Callback function for when a file is double-clicked in the grid view
 * @param view The GtkGridView that was clicked
 * @param position The position of the clicked item
 * @param user_data User data passed to the callback (in this case, the GListStore of files)
 */
void file_clicked(GtkGridView *view, guint position, gpointer user_data) {
    GListStore *files = G_LIST_STORE(user_data);
    GFile *file = g_list_model_get_item(G_LIST_MODEL(files), position);

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

/**
 * Callback function for when the directory entry is changed
 * @param self The GtkEntry that was changed
 * @param user_data User data passed to the callback (not used here)
 */
void directory_entry_changed(  GtkEntry* self, gpointer user_data) {
    const char *new_directory = gtk_editable_get_text(GTK_EDITABLE(self));
    if (new_directory != NULL && strlen(new_directory) != 0 && strcmp(new_directory, current_directory) != 0) {
            const GFile *file = g_file_new_for_path(new_directory);
            if (file) {
                populate_files(new_directory);
            } else {
                gtk_editable_set_text(GTK_EDITABLE(directory_entry), current_directory);
            }
            g_object_unref(G_OBJECT(file));
            g_print("Directory entry changed to: %s\n", new_directory);
    }
}

/**
 * Function to go up one directory level
 */
void go_up_a_directory() {
    if (current_directory == NULL) {
        return;
    }

    // Create a GFile from the current directory
    GFile *current = g_file_new_for_path(current_directory);

    // Get the parent directory
    GFile *parent = g_file_get_parent(current);

    if (parent != NULL) {
        // Get the path of the parent directory
        const char *parent_path = g_file_get_path(parent);

        // Populate files with the parent directory
        populate_files(parent_path);

        // Free resources
        // g_free(parent_path);
        // g_object_unref(parent);
    }

    g_object_unref(current);
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
