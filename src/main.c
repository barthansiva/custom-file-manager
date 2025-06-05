#include <gtk/gtk.h>
#include "utils.h"
#include "ui_builder.h"
#include <stdlib.h>

GtkWidget *file_container;
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

    //
    // Read all of the files in the specified directory
    //

    // Clear existing files
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(file_container)) != NULL) {
        gtk_flow_box_remove(GTK_FLOW_BOX(file_container), child);
    }

    // strdup() creates a copy of a string
    char* dir_copy = strdup(directory);

    if (current_directory != NULL) {
        free(current_directory); // I hate C
    }
    current_directory = dir_copy;

    size_t file_count = 0;
    file_t* files = get_files_in_directory(current_directory, &file_count);

    if (files == NULL) {
        g_printerr("Failed to read directory: %s\n", current_directory);
        return FALSE;
    }

    //
    // Create all widgets for the files and populate the file_container
    //

    GtkWidget** file_widgets = create_file_widgets(files, file_count, file_clicked);

    for (size_t i = 0; i < file_count; i++) {
        // position -1 means at the end
        gtk_flow_box_insert(GTK_FLOW_BOX(file_container), file_widgets[i], -1);
    }

    // more memory bs
    free(file_widgets);
    for (size_t i = 0; i < file_count; i++) {
        free(files[i].name);
    }
    free(files);

    return TRUE;
}

// Right now its just a box with a label and a flowbox (file_container)
static void init(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *scrolled_window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Manager"); //Really original title
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // Main container
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    label = gtk_label_new("Test");
    gtk_box_append(GTK_BOX(vbox), label); // <- Amazing, absolutely *NOT* object oriented code

    // Create the file container (flowbox)
    file_container = gtk_flow_box_new();

    //Selection kinda works with this but its weird, need to look into that
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(file_container), GTK_SELECTION_MULTIPLE);

    gtk_widget_set_hexpand(file_container, TRUE);
    gtk_widget_set_vexpand(file_container, TRUE);

    // Make file container child of a scrollable widget
    scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), file_container);

    // GTK is actually kinda nice with the defaults (unlike Qt)
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_widget_set_vexpand(scrolled_window, TRUE);

    gtk_box_append(GTK_BOX(vbox), scrolled_window);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // Show the window
    gtk_window_present(GTK_WINDOW(window));

    populate_files("/home"); //Default directory, maybe make it configurable later
}

/**
 * Handles file/directory item clicks
 * @param widget The clicked widget
 * @param file_data Pointer to the file_t data for the clicked item
 */
void file_clicked(GtkWidget *widget, gpointer file_data) {
    file_t *file = file_data;

    // Pretty self explanatory
    if (file->is_dir) {

        char path[1024];

        // Get full path of the clicked directory
        snprintf(path, sizeof(path), "%s/%s", current_directory, file->name);

        free(current_directory); //I once again hate C
        current_directory = strdup(path);

        populate_files(current_directory);
    } else {
        // It's a regular file, just print info for now
        g_print("File clicked: %s (%.2f KB)\n", file->name, file->size_kb);
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
