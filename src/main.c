#include <gtk/gtk.h>
#include "utils.h"
#include "ui_builder.h"
#include <stdlib.h>

GtkWidget *main_file_container;
GtkWidget *directory_entry;
GtkWidget *notebook;

const char *default_directory = "/home"; // Default directory to start in
char *current_directory = NULL;

// Function declarations
void file_clicked(GtkGridView *view, guint position, gpointer user_data);

void directory_entry_changed(  GtkEntry* self, gpointer user_data);

void go_up_a_directory();

void on_close_tab_clicked(GtkButton *button, gpointer user_data);

void on_back_clicked(GtkButton *button, gpointer user_data);

void on_forward_clicked(GtkButton *button, gpointer user_data);

void add_tab_with_directory(const char* path);

typedef struct {
    GtkWidget *scrolled_window;
    GtkWidget *tab_label;
    char *current_path;

    GList *back_stack;
    GList *forward_stack;
} TabContext;

void populate_files_in_container(const char *directory, GtkWidget *container, TabContext *ctx);

void on_add_tab_clicked(GtkButton *button, gpointer user_data);

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

    //
    // Tabs
    //
    GtkWidget *back_button = gtk_button_new_from_icon_name("go-previous-symbolic");
    gtk_widget_set_tooltip_text(back_button, "Back");

    GtkWidget *forward_button = gtk_button_new_from_icon_name("go-next-symbolic");
    gtk_widget_set_tooltip_text(forward_button, "Forward");

    gtk_box_append(GTK_BOX(toolbar.toolbar), back_button);
    gtk_box_append(GTK_BOX(toolbar.toolbar), forward_button);

    // Connect signals
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_clicked), NULL);
    g_signal_connect(forward_button, "clicked", G_CALLBACK(on_forward_clicked), NULL);


    GtkWidget *add_tab_button = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(add_tab_button, "Open new tab");
    gtk_box_append(GTK_BOX(toolbar.toolbar), add_tab_button);

    // Connect signal
    g_signal_connect(add_tab_button, "clicked", G_CALLBACK(on_add_tab_clicked), NULL);


    // Connect signals
    g_signal_connect(toolbar.up_button, "clicked", G_CALLBACK(go_up_a_directory), NULL);

    g_signal_connect(directory_entry, "icon-press", G_CALLBACK(directory_entry_changed), NULL);
    g_signal_connect(directory_entry, "activate", G_CALLBACK(directory_entry_changed), NULL);

    //
    // Initial Tabs
    add_tab_with_directory("/home");


    //
    // File Container Area — using notebook now
    //
    notebook = gtk_notebook_new();
    gtk_widget_set_hexpand(notebook, TRUE);
    gtk_widget_set_vexpand(notebook, TRUE);

    gtk_box_append(GTK_BOX(right_box), toolbar.toolbar);
    gtk_box_append(GTK_BOX(right_box), notebook);

    add_tab_with_directory("/home");

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
 * Callback function for when a file is double-clicked in the grid view.
 * If the selected file is a directory, the tab's content is replaced with the new directory listing.
 *
 * This function works per-tab using the TabContext passed as user_data.
 *
 * @param view The GtkGridView that was activated
 * @param position The index of the item in the grid that was clicked
 * @param user_data Pointer to the TabContext of the current tab
 */
void file_clicked(GtkGridView *view, guint position, gpointer user_data) {
    TabContext *ctx = (TabContext *)user_data;

    GtkSelectionModel *model = gtk_grid_view_get_model(view);
    GListStore *files = G_LIST_STORE(gtk_multi_selection_get_model(GTK_MULTI_SELECTION(model)));

    GFile *file = g_list_model_get_item(G_LIST_MODEL(files), position);
    GFileInfo *info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    GFileType type = g_file_info_get_file_type(info);

    if (type == G_FILE_TYPE_DIRECTORY) {
        const char *path = g_file_get_path(file);
        populate_files_in_container(path, ctx->scrolled_window, ctx);
    } else {
        g_print("File %s clicked\n", g_file_get_basename(file));
    }

    g_object_unref(file);
    g_object_unref(info);
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

/**
 * Adds a new tab to the notebook, showing the contents of the specified directory.
 * Each tab holds its own GtkScrolledWindow and tracks its state with a TabContext.
 *
 * Automatically switches to the newly created tab and sets the tab label to the basename of the path.
 *
 * @param path The directory path to load in the new tab
 */
void add_tab_with_directory(const char* path) {
    // Create and allocate tab context
    TabContext *ctx = g_malloc0(sizeof(TabContext));
    ctx->current_path = g_strdup(path);

    // Create file container
    ctx->scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(ctx->scrolled_window, TRUE);
    gtk_widget_set_vexpand(ctx->scrolled_window, TRUE);

    // Create tab label
    const char* tab_name = g_path_get_basename(path);
    ctx->tab_label = gtk_label_new(tab_name);

    // Create custom tab header box (label + close button)
    GtkWidget *tab_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign(tab_header, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(tab_header, GTK_ALIGN_CENTER);

    GtkWidget *close_button = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_set_tooltip_text(close_button, "Close tab");
    gtk_widget_set_size_request(close_button, 20, 20);
    gtk_button_set_has_frame(GTK_BUTTON(close_button), FALSE);
    gtk_widget_set_focusable(close_button, FALSE);

    gtk_box_append(GTK_BOX(tab_header), ctx->tab_label);
    gtk_box_append(GTK_BOX(tab_header), close_button);

    // Populate files into this container
    populate_files_in_container(path, ctx->scrolled_window, ctx);

    // Add the new tab
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ctx->scrolled_window, tab_header);
    g_object_set_data(G_OBJECT(ctx->scrolled_window), "tab_ctx", ctx);

    // Connect close button handler
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_tab_clicked), notebook);

    // Switch to the newly added tab
    int total_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), total_pages - 1);
}

/**
 * Populates a given scrolled window container with file views from a directory,
 * using a given TabContext to track the view state and label title.
 *
 * This function sets up the file grid, handles file icons, and sets the activate signal.
 * It replaces the content inside the container with a new grid view of the files.
 *
 * @param directory The full path to the directory to load
 * @param container The GtkScrolledWindow to place the file grid inside
 * @param ctx Pointer to the TabContext for this tab (holds the label and current path)
 */

void populate_files_in_container(const char *directory, GtkWidget *container, TabContext *ctx) {
    if (ctx->current_path && strcmp(directory, ctx->current_path) != 0) {
        // Push current path to back stack
        ctx->back_stack = g_list_prepend(ctx->back_stack, ctx->current_path);
        ctx->forward_stack = NULL;  // Clear forward stack
    }

    ctx->current_path = g_strdup(directory);  // Update current path (replace strdup from before)

    size_t file_count = 0;
    GListStore *files = get_files_in_directory(directory, &file_count);
    if (!files) return;

    GtkMultiSelection *selection = gtk_multi_selection_new(G_LIST_MODEL(files));
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_file_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_file_item), NULL);

    GtkWidget *view = gtk_grid_view_new(GTK_SELECTION_MODEL(selection), factory);
    gtk_grid_view_set_single_click_activate(GTK_GRID_VIEW(view), FALSE);

    // Important: Set file click handler
    g_signal_connect(view, "activate", G_CALLBACK(file_clicked), ctx);

    // Swap in the new view
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(container), view);

    // Update context path + tab label
    g_free(ctx->current_path);
    ctx->current_path = g_strdup(directory);
    gtk_editable_set_text(GTK_EDITABLE(directory_entry), directory);
    gtk_label_set_text(GTK_LABEL(ctx->tab_label), g_path_get_basename(directory));
}

/**
 * Callback for the "+" button that opens a new tab.
 */
void on_add_tab_clicked(GtkButton *button, gpointer user_data) {
    // You can use a default or home directory here
    add_tab_with_directory("/home");
}

void on_close_tab_clicked(GtkButton *button, gpointer user_data) {
    GtkNotebook *notebook = GTK_NOTEBOOK(user_data);
    int n_pages = gtk_notebook_get_n_pages(notebook);

    for (int i = 0; i < n_pages; ++i) {
        GtkWidget *page = gtk_notebook_get_nth_page(notebook, i);
        GtkWidget *tab_header = gtk_notebook_get_tab_label(notebook, page);

        if (gtk_widget_is_ancestor(GTK_WIDGET(button), tab_header)) {
            gtk_notebook_remove_page(notebook, i);
            break;
        }
    }
}

void on_back_clicked(GtkButton *button, gpointer user_data) {
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (page < 0) return;

    GtkWidget *container = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
    GtkWidget *tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), container);
    TabContext *ctx = NULL;

    // Try to find the right TabContext
    for (int i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)); i++) {
        GtkWidget *pg = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
        if (pg == container) {
            // crude lookup: we assume ctx is associated with container memory-wise
            ctx = g_object_get_data(G_OBJECT(pg), "tab_ctx");
            break;
        }
    }

    if (ctx && ctx->back_stack) {
        char *prev_path = ctx->back_stack->data;
        ctx->back_stack = g_list_delete_link(ctx->back_stack, ctx->back_stack);
        ctx->forward_stack = g_list_prepend(ctx->forward_stack, g_strdup(ctx->current_path));
        populate_files_in_container(prev_path, ctx->scrolled_window, ctx);
    }
}

void on_forward_clicked(GtkButton *button, gpointer user_data) {
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (page < 0) return;

    GtkWidget *container = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
    GtkWidget *tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), container);
    TabContext *ctx = NULL;

    for (int i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)); i++) {
        GtkWidget *pg = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
        if (pg == container) {
            ctx = g_object_get_data(G_OBJECT(pg), "tab_ctx");
            break;
        }
    }

    if (ctx && ctx->forward_stack) {
        char *next_path = ctx->forward_stack->data;
        ctx->forward_stack = g_list_delete_link(ctx->forward_stack, ctx->forward_stack);
        ctx->back_stack = g_list_prepend(ctx->back_stack, g_strdup(ctx->current_path));
        populate_files_in_container(next_path, ctx->scrolled_window, ctx);
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
