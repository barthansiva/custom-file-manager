#include <gtk/gtk.h>
#include "utils.h"
#include "ui_builder.h"
#include <stdlib.h>

GtkWidget *main_file_container;
char *current_directory = NULL;

// Function declarations
void file_clicked(GtkWidget *widget, gpointer file_data);

/**
 * Totally not AI-generated code documentation
 * Populates the file_container with widgets for files in the specified directory
 * @param directory Path to the directory to display
 * @return TRUE if successful, FALSE otherwise
 */
gboolean populate_files(const char* directory) {
    // Store current directory
    char* dir_copy = strdup(directory);
    if (!dir_copy) {
        g_warning("Failed to allocate memory for directory path");
        return FALSE;
    }

    // Clean up old directory path
    if (current_directory != NULL) {
        free(current_directory);
    }
    current_directory = dir_copy;

    // Clear any existing content first
    GtkWidget* old_child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(main_file_container));
    if (old_child) {
        // Remove old child (this will destroy the widget)
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(main_file_container), NULL);
    }

    // Read all files in the specified directory
    size_t file_count = 0;
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

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(main_file_container), view);

    // Release our reference to files since the model now holds one
    g_object_unref(files);
    // Release our reference to the factory
    g_object_unref(factory);

    return TRUE;
}

// Right now its just a box with a label and a flowbox (file_container)
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
 * Handles file/directory item clicks
 * @param widget The clicked widget (the button)
 * @param user_data Not used (NULL)
 */
void file_clicked(GtkWidget *widget, gpointer user_data) {
    g_print("File clicked\n");

    // Get the file path from the button's data
    const char *path = g_object_get_data(G_OBJECT(widget), "file-path");
    if (!path) {
        g_warning("No file path found in button data");
        return;
    }

    g_print("Path from button: %s\n", path);

    // Create a GFile from the path
    GFile *file = g_file_new_for_path(path);
    if (!file) {
        g_warning("Failed to create GFile from path");
        return;
    }

    GError *error = NULL;

    GFileInfo* info = g_file_query_info(file,
                      G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_SIZE "," G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                      G_FILE_QUERY_INFO_NONE,
                      NULL,
                      &error);

    if (error != NULL) {
        g_warning("Error getting file info: %s", error->message);
        g_error_free(error);
        g_object_unref(file);
        return;
    }

    if (!info) {
        g_warning("Failed to get file info");
        g_object_unref(file);
        return;
    }

    GFileType type = g_file_info_get_file_type(info);

    if (type == G_FILE_TYPE_DIRECTORY) {
        // It's a directory, populate the file container with its contents
        g_print("Directory clicked: %s\n", path);

        // Release resources before populating files
        g_object_unref(info);
        g_object_unref(file);

        populate_files(path);
    } else {
        // It's a regular file, just print info for now
        const char *name = g_file_info_get_display_name(info);
        goffset size = g_file_info_get_size(info);

        g_print("File clicked: %s (%.2f KB)\n", name ? name : "Unknown", size / 1024.0);

        // Release resources
        g_object_unref(info);
        g_object_unref(file);
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
