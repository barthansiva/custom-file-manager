#include <gtk/gtk.h>
#include "utils.h"
#include "ui_builder.h"
#include "main.h"
#include <stdlib.h>
#include "snake.h"
#include <glib.h>
#include <sys/stat.h>
#include <gio/gio.h>
#include <glib/gdatetime.h>

GtkWidget *window;
GtkWidget *main_file_container;
GtkWidget *directory_entry;
GtkWidget *notebook;
GtkWidget *search_entry;

const char *default_directory = "/home"; // Default directory to start in

// Function declarations
void file_clicked(GtkGridView *view, guint position, gpointer user_data);

void directory_entry_changed(  GtkEntry* self, gpointer user_data);

void on_up_clicked();

void on_close_tab_clicked(GtkButton *button, gpointer user_data);

void add_tab_with_directory(const char* path);

void update_preview_text(TabContext *ctx, GFile *file);

void on_add_tab_clicked(GtkButton *button, gpointer user_data);

void search_entry_changed(GtkEditable *editable, gpointer user_data);

void tab_changed(GtkNotebook* self, GtkWidget* page, guint page_num, gpointer user_data);

static void menu_delete_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void menu_rename_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

void undo_button_clicked(GtkButton *button, gpointer user_data);

void redo_button_clicked(GtkButton *button, gpointer user_data);

void on_settings_button_clicked(GtkButton *button, gpointer user_data);

void on_new_folder_clicked(GtkButton *button, gpointer user_data);

void on_open_in_tab_clicked(GtkButton *button, gpointer user_data);

void on_properties_clicked(GtkButton *button, gpointer user_data);

void on_sort_by_clicked(GtkButton *button, gpointer user_data);

void on_open_terminal_clicked(GtkButton *button, gpointer user_data);

void file_rename_confirm(GtkEntry* self, gpointer data1, gpointer data2);

void close_dialog_window(GtkEntry* self, gpointer data1, gpointer data2);

static const GActionEntry win_actions[] = {
    { "delete", menu_delete_clicked, "s", NULL, NULL },
    { "rename", menu_rename_clicked, "s", NULL, NULL }
};

/**
 * Builds the core widget structure of the application
 * @param app The GtkApplication instance
 * @param user_data User data passed to the callback (not used here)
 */
static void init(GtkApplication *app, gpointer user_data) {
    // Initialize the operation history array
    init_operation_history();

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Manager"); //Really original title
    gtk_window_set_default_size(GTK_WINDOW(window), 1600, 900);

    //Add actions to our window (stupid ass system, I have no idea why it exists)
    g_action_map_add_action_entries(G_ACTION_MAP(window), win_actions, G_N_ELEMENTS(win_actions), window);

    //
    //  Left side panel
    //
    left_box_t left_box = create_left_box();

    g_signal_connect(left_box.undo_button,"clicked",G_CALLBACK(undo_button_clicked),NULL);
    g_signal_connect(left_box.redo_button,"clicked",G_CALLBACK(redo_button_clicked),NULL);

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
    toolbar_t toolbar = create_toolbar(default_directory);

    //
    // Search
    //
    search_entry = toolbar.search_entry;
    g_signal_connect(search_entry, "changed", G_CALLBACK(search_entry_changed), NULL);

    // Store directory_entry in the global variable
    directory_entry = toolbar.directory_entry;

    //
    // Tabs
    //

    GtkWidget *add_tab_button = gtk_button_new_from_icon_name("tab-new-symbolic");
    gtk_widget_set_tooltip_text(add_tab_button, "Open new tab");
    gtk_box_append(GTK_BOX(toolbar.toolbar), add_tab_button);

    // Connect signal
    g_signal_connect(add_tab_button, "clicked", G_CALLBACK(on_add_tab_clicked), NULL);

    // Connect signals
    g_signal_connect(toolbar.up_button, "clicked", G_CALLBACK(on_up_clicked), NULL);

    g_signal_connect(directory_entry, "icon-press", G_CALLBACK(directory_entry_changed), NULL);
    g_signal_connect(directory_entry, "activate", G_CALLBACK(directory_entry_changed), NULL);

    //
    // Settings
    //
    GtkWidget *settings_button = gtk_button_new_from_icon_name("open-menu-symbolic");
    gtk_widget_set_tooltip_text(settings_button, "Settings");
    g_signal_connect(settings_button, "clicked", G_CALLBACK(on_settings_button_clicked), NULL);
    gtk_box_append(GTK_BOX(toolbar.toolbar), settings_button);

    //
    // File Container Area — using notebook now
    //
    notebook = gtk_notebook_new();
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    g_signal_connect(notebook, "switch-page", G_CALLBACK(tab_changed), NULL);
    gtk_widget_set_hexpand(notebook, TRUE);
    gtk_widget_set_vexpand(notebook, TRUE);

    gtk_box_append(GTK_BOX(right_box), toolbar.toolbar);
    gtk_box_append(GTK_BOX(right_box), notebook);

    add_tab_with_directory(default_directory);

    //
    //  Main container
    //
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    gtk_paned_set_start_child(GTK_PANED(hpaned), left_box.side_panel);
    gtk_paned_set_shrink_start_child(GTK_PANED(hpaned), FALSE);
    gtk_paned_set_resize_start_child(GTK_PANED(hpaned), FALSE);

    gtk_paned_set_end_child(GTK_PANED(hpaned), right_box);
    gtk_paned_set_shrink_end_child(GTK_PANED(hpaned), FALSE);

    gtk_window_set_child(GTK_WINDOW(window), hpaned);

    // Show the window
    gtk_window_present(GTK_WINDOW(window));

   // populate_files(default_directory);
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
void file_clicked(GtkGridView *view, const guint position, const gpointer user_data) {
    TabContext *ctx = (TabContext *)user_data;

    GtkSelectionModel *model = gtk_grid_view_get_model(view);
    GListStore *files = G_LIST_STORE(gtk_multi_selection_get_model(GTK_MULTI_SELECTION(model)));

    GFile *file = g_list_model_get_item(G_LIST_MODEL(files), position);
    GFileInfo *info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    GFileType type = g_file_info_get_file_type(info);

    if (type == G_FILE_TYPE_DIRECTORY) {
        gtk_revealer_set_reveal_child(GTK_REVEALER(ctx->preview_revealer), FALSE);
        const char *path = g_file_get_path(file);
        populate_files_in_container(path, ctx->scrolled_window, ctx);
    } else {
        open_file_with_default_app(file);
        GFileInfo *full_info = g_file_query_info(file,
            G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
            G_FILE_QUERY_INFO_NONE, NULL, NULL);

        if (full_info) {
            const char *mime = g_file_info_get_content_type(full_info);
            if (g_str_has_prefix(mime, "text/")) {
                update_preview_text(ctx, file);
            } else {
                gtk_revealer_set_reveal_child(GTK_REVEALER(ctx->preview_revealer), FALSE);
            }
            g_object_unref(full_info);
        }
    }

    g_object_unref(file);
    g_object_unref(info);
}

/**
 * Callback function for when the directory entry is changed
 * @param self The GtkEntry that was changed
 * @param user_data User data passed to the callback (not used here)
 */
void directory_entry_changed(GtkEntry* self, gpointer user_data) {
    const char *new_directory = gtk_editable_get_text(GTK_EDITABLE(self));
    TabContext* ctx = get_current_tab_context();
    if (strcmp(new_directory,"play/snake") == 0) {
        launch_snake();
        gtk_editable_set_text(GTK_EDITABLE(directory_entry), ctx->current_directory);
        return;
    }
    if (new_directory != NULL && strlen(new_directory) != 0 && strcmp(new_directory, ctx->current_directory) != 0) {
            if (g_file_test(new_directory, G_FILE_TEST_EXISTS) && g_file_test(new_directory, G_FILE_TEST_IS_DIR)) {
                populate_files_in_container(new_directory, ctx->scrolled_window, ctx);
            } else {
                gtk_editable_set_text(GTK_EDITABLE(directory_entry), ctx->current_directory);
            }
    }
}

/**
 * Function to go up one directory level
 */
void on_up_clicked() {

    TabContext* ctx = get_current_tab_context();
    if (!ctx || !ctx->current_directory) return;

    if (ctx->current_directory == NULL) {
        return;
    }

    // Create a GFile from the current directory
    GFile *current = g_file_new_for_path(ctx->current_directory);

    // Get the parent directory
    GFile *parent = g_file_get_parent(current);

    if (parent != NULL) {
        // Get the path of the parent directory
        const char *parent_path = g_file_get_path(parent);
        if (parent_path) {
            populate_files_in_container(parent_path, ctx->scrolled_window, ctx);
        }
        g_object_unref(parent);
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
    TabContext *ctx = g_malloc0(sizeof(TabContext));
    ctx->current_directory = g_strdup(path);

    // File list container
    ctx->scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(ctx->scrolled_window, TRUE);
    gtk_widget_set_vexpand(ctx->scrolled_window, TRUE);

    // Preview setup
    ctx->preview_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(ctx->preview_text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ctx->preview_text_view), GTK_WRAP_WORD);

    GtkWidget *preview_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(preview_scroll), ctx->preview_text_view);

    ctx->preview_revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(ctx->preview_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
    gtk_widget_set_hexpand(ctx->preview_revealer, TRUE);
    gtk_widget_set_vexpand(ctx->preview_revealer, TRUE);
    gtk_revealer_set_child(GTK_REVEALER(ctx->preview_revealer), preview_scroll);
    gtk_revealer_set_reveal_child(GTK_REVEALER(ctx->preview_revealer), FALSE);  // initially hidden

    // Split pane with file list and preview
    GtkWidget *split = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(split, TRUE);
    gtk_widget_set_vexpand(split, TRUE);
    gtk_paned_set_start_child(GTK_PANED(split), ctx->scrolled_window);
    gtk_paned_set_end_child(GTK_PANED(split), ctx->preview_revealer);

    // Save context to lookup later
    g_object_set_data(G_OBJECT(split), "tab_ctx", ctx);

    // Tab label setup
    const char* tab_name = g_path_get_basename(path);
    ctx->tab_label = gtk_label_new(tab_name);

    GtkWidget *tab_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING);
    gtk_widget_set_halign(tab_header, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(tab_header, GTK_ALIGN_CENTER);

    GtkWidget *close_button = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_set_tooltip_text(close_button, "Close tab");
    gtk_widget_set_size_request(close_button, 20, 20);
    gtk_button_set_has_frame(GTK_BUTTON(close_button), FALSE);
    gtk_widget_set_focusable(close_button, FALSE);

    gtk_box_append(GTK_BOX(tab_header), ctx->tab_label);
    gtk_box_append(GTK_BOX(tab_header), close_button);

    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_tab_clicked), notebook);

    // Load file contents
    populate_files_in_container(path, ctx->scrolled_window, ctx);

    // Add tab
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), split, tab_header);
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
    ctx->current_directory = g_strdup(directory);  // Update current path (replace strdup from before)

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
    g_free(ctx->current_directory);
    ctx->current_directory = g_strdup(directory);
    gtk_editable_set_text(GTK_EDITABLE(directory_entry), ctx->current_directory);
    gtk_label_set_text(GTK_LABEL(ctx->tab_label), g_path_get_basename(directory));
}

/**
 * Callback for the "Add Tab" button.
 * It creates a new tab with the default directory.
 *
 * @param button The GtkButton that was clicked
 * @param user_data Not used here, but can be used to pass additional data
 */
void on_add_tab_clicked(GtkButton *button, gpointer user_data) {
    // You can use a default or home directory here
    add_tab_with_directory(default_directory);
}

/**
 * Callback for the close button on a tab.
 * It finds the tab that contains the button and removes it from the notebook.
 *
 * @param button The GtkButton that was clicked
 * @param user_data The GtkNotebook containing the tabs
 */
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

/**
 * uses GTK standard function to open files with default program,
 * complains if there is no default program
 * @param file
 */
void open_file_with_default_app(GFile *file) {
    GError *error = NULL;
    GFileInfo *info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                        G_FILE_QUERY_INFO_NONE, NULL, &error);

    // catch for if no file info is found
    if (!info) {
        g_warning("Could not get file info: %s", error->message);
        g_error_free(error);
        return;
    }

    // this gets the identifying type of the file
    const char *content_type = g_file_info_get_content_type(info);
    GAppInfo *app_info = g_app_info_get_default_for_type(content_type, FALSE);
    if (!app_info) {
        g_warning("wdym: %s", content_type);
        g_object_unref(info);
        return;
    }

    // this stops the program from crashing when using an invalid file (I tried this with an .fbx file bc it's
    // the most obscure one I had on hand)
    GList *files = g_list_prepend(NULL, file);
    if (!g_app_info_launch(app_info, files, NULL, &error)) {
        g_warning("Failed to launch file: %s", error->message);
        g_error_free(error);
    }

    // Return used memory
    g_list_free(files);
    g_object_unref(app_info);
    g_object_unref(info);
}


/**
 * Gets the current TabContext based on the currently active tab in the notebook.
 * This function assumes that each tab's content is a GtkScrolledWindow with a TabContext stored in its user data.
 *
 * @return Pointer to the current TabContext, or NULL if no tab is active.
 */
TabContext* get_current_tab_context() {
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (page < 0) return NULL;

    GtkWidget *split = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
    if (!split) return NULL;

    return g_object_get_data(G_OBJECT(split), "tab_ctx");
}

void populate_files_with_filter(const char *filter) {
    TabContext *ctx = get_current_tab_context();
    if (!ctx || !ctx->current_directory) return;

    size_t file_count = 0;
    GListStore *raw_files = get_files_in_directory(ctx->current_directory, &file_count);
    if (!raw_files) return;

    // Prepare filtered list
    GListStore *filtered_files = g_list_store_new(G_TYPE_FILE);
    for (guint i = 0; i < g_list_model_get_n_items(G_LIST_MODEL(raw_files)); i++) {
        GFile *file = g_list_model_get_item(G_LIST_MODEL(raw_files), i);
        const char *basename = g_file_get_basename(file);

        if (!filter || strlen(filter) == 0 || g_strrstr(basename, filter)) {
            g_list_store_append(filtered_files, file);
        }

        g_object_unref(file);
    }
    g_object_unref(raw_files);

    // If no matches found, avoid updating the view
    if (g_list_model_get_n_items(G_LIST_MODEL(filtered_files)) == 0) {
        g_object_unref(filtered_files);
        return;
    }

    GtkWidget *view = gtk_grid_view_new(
        GTK_SELECTION_MODEL(gtk_multi_selection_new(G_LIST_MODEL(filtered_files))),
        gtk_signal_list_item_factory_new()
    );

    g_signal_connect(view, "activate", G_CALLBACK(file_clicked), ctx);

    // Setup factory
    GtkListItemFactory *factory = gtk_grid_view_get_factory(GTK_GRID_VIEW(view));
    g_signal_connect(factory, "setup", G_CALLBACK(setup_file_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_file_item), NULL);

    // Replace old view safely
    GtkWidget *old_view = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctx->scrolled_window));
    if (GTK_IS_WIDGET(old_view)) {
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(ctx->scrolled_window), NULL);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(ctx->scrolled_window), view);
}


void search_entry_changed(GtkEditable *editable, gpointer user_data) {
    const char *query = gtk_editable_get_text(editable);
    populate_files_with_filter(query);
}

void tab_changed(GtkNotebook* self, GtkWidget* page, const guint page_num, gpointer user_data) {
    const TabContext* ctx = g_object_get_data(G_OBJECT(page), "tab_ctx");
    if (ctx) {
        gtk_editable_set_text(GTK_EDITABLE(directory_entry), ctx->current_directory);
    } else {
        gtk_editable_set_text(GTK_EDITABLE(directory_entry), default_directory);
    }
}

void update_preview_text(TabContext *ctx, GFile *file) {
    if (!ctx || !ctx->preview_text_view || !ctx->preview_revealer) return;

    char *path = g_file_get_path(file);
    if (!path) return;

    gchar *content = NULL;
    gsize length;
    GError *error = NULL;

    if (!g_file_get_contents(path, &content, &length, &error)) {
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(ctx->preview_text_view)),
                                 "Could not read file.", -1);
        if (error) g_error_free(error);
    } else {
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(ctx->preview_text_view)),
                                 content, -1);
        g_free(content);
    }

    gtk_revealer_set_reveal_child(GTK_REVEALER(ctx->preview_revealer), TRUE);
    g_free(path);
}

void reload_current_directory() {
    TabContext *ctx = get_current_tab_context();
    if (ctx && ctx->current_directory) {
        populate_files_in_container(ctx->current_directory, ctx->scrolled_window, ctx);
    }
}

// New function to handle right-click on file items
void file_right_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {

    GtkWidget *box = GTK_WIDGET(user_data);

    size_t count;
    GtkGridView* view = GTK_GRID_VIEW(gtk_widget_get_first_child(get_current_tab_context()->scrolled_window));
    GFile** selected_files = get_selection(view, &count);

    GtkListItem *list_item = GTK_LIST_ITEM(g_object_get_data(G_OBJECT(box), "list-item"));
    GFile *file = G_FILE(gtk_list_item_get_item(list_item));

    char* params;
    if (count == 0) {

        params = g_file_get_path(file);

        g_object_unref(file);
    }else {
        params = join_basenames(selected_files, count, " ", g_file_get_path(file));
        g_free(selected_files);
    }

    GtkPopoverMenu* popover = create_file_context_menu(params);
    gtk_widget_set_parent(GTK_WIDGET(popover), box);
    const GdkRectangle rect = { gtk_widget_get_width(box)/2, gtk_widget_get_height(box), 1, 1 };
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
    gtk_popover_popup(GTK_POPOVER(popover));

}

static void menu_delete_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const char *params = g_variant_get_string(parameter, NULL);
    size_t count;
    char** files = split_basenames(params," ", &count);
    if (count == 1) {
        delete_file(files[0]);
    } else {
        // Extract directory path from the first element
        char *dir_path = strdup(get_directory(files[0]));

        // Loop through remaining files (starting from index 1)
        for (size_t i = 1; i < count; i++) {
            // Construct absolute path by combining directory path and filename
            char *abs_path = g_build_filename(dir_path, files[i], NULL);
            // Delete the file
            delete_file(abs_path);

            // Free the constructed path
            g_free(abs_path);
        }

        // Free the directory path
        g_free(dir_path);
    }

    // Free the split basenames array
    g_strfreev(files);
    reload_current_directory();
}

static void menu_rename_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {

    const char *params = g_variant_get_string(parameter, NULL);
    size_t count;
    char** files = split_basenames(params," ", &count);
    dialog_t dialog = create_dialog("Rename File", "Enter new name for the file:");

    gtk_window_set_transient_for(GTK_WINDOW(dialog.dialog), GTK_WINDOW(window));
    gtk_window_set_modal(GTK_WINDOW(dialog.dialog), TRUE);

    char* path = strdup(files[0]);

    g_print(path);
    gtk_editable_set_text(GTK_EDITABLE(dialog.entry), get_basename(path));
    g_signal_connect(GTK_WIDGET(dialog.entry), "icon-press", G_CALLBACK(file_rename_confirm), path);
    g_signal_connect(GTK_WIDGET(dialog.entry), "activate", G_CALLBACK(file_rename_confirm), path);

    g_signal_connect(GTK_WIDGET(dialog.entry), "icon-press", G_CALLBACK(close_dialog_window), dialog.dialog);
    g_signal_connect(GTK_WIDGET(dialog.entry), "activate", G_CALLBACK(close_dialog_window), dialog.dialog);

    gtk_window_present(GTK_WINDOW(dialog.dialog));
}

void close_dialog_window(GtkEntry* self, gpointer data1, gpointer data2) {
    GtkWindow* dialog_window = data2 ? data2 : data1;

    gtk_window_close(dialog_window);
    gtk_window_destroy(dialog_window);
}

void file_rename_confirm(GtkEntry* self, gpointer data1, gpointer data2) {
    char* src = data2 ? data2 : data1;
    const char *new_name = gtk_editable_get_text(GTK_EDITABLE(self));
    if (new_name == NULL || strlen(new_name) == 0) {
        g_free(src);
        return; // No new name provided
    }
    // Get the directory of the source file
    char *dir = get_directory(strdup(src));
    // Construct the new file path
    char *new_path = g_build_filename(dir, new_name, NULL);
    // Free the directory path
    g_free(dir);
    // Move the file to the new path
    move_file(src, new_path);

    reload_current_directory();
}


void undo_button_clicked(GtkButton *button, gpointer user_data) {
    undo_last_operation();
    reload_current_directory();
}

void redo_button_clicked(GtkButton *button, gpointer user_data) {
    redo_last_undo();
    reload_current_directory();
}

void on_settings_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *popover = gtk_popover_new();
    gtk_widget_set_parent(GTK_WIDGET(popover), GTK_WIDGET(button));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *new_folder = gtk_button_new_with_label("New Folder");
    GtkWidget *open_tab = gtk_button_new_with_label("Open in New Tab");
    GtkWidget *properties = gtk_button_new_with_label("Properties");
    GtkWidget *sort_by = gtk_button_new_with_label("Sort By");
    GtkWidget *open_terminal = gtk_button_new_with_label("Open in Terminal");

    gtk_box_append(GTK_BOX(box), new_folder);
    gtk_box_append(GTK_BOX(box), open_tab);
    gtk_box_append(GTK_BOX(box), properties);
    gtk_box_append(GTK_BOX(box), sort_by);
    gtk_box_append(GTK_BOX(box), open_terminal);

    gtk_popover_set_child(GTK_POPOVER(popover), box);
    // gtk_widget_show(popover); // Removed this deprecated line
    gtk_popover_popup(GTK_POPOVER(popover)); // Use this to show the popover
    // might also want to connect a signal to position the popover correctly
    // or use gtk_popover_set_pointing_to if i have a specific widget/area to point to.


    // Connect signals
    g_signal_connect(new_folder, "clicked", G_CALLBACK(on_new_folder_clicked), NULL);
    g_signal_connect(open_tab, "clicked", G_CALLBACK(on_open_in_tab_clicked), NULL);
    g_signal_connect(properties, "clicked", G_CALLBACK(on_properties_clicked), NULL);
    g_signal_connect(sort_by, "clicked", G_CALLBACK(on_sort_by_clicked), NULL);
    g_signal_connect(open_terminal, "clicked", G_CALLBACK(on_open_terminal_clicked), NULL);
}

void on_new_folder_clicked(GtkButton *button, gpointer user_data) {
    TabContext *ctx = get_current_tab_context();
    if (!ctx || !ctx->current_directory) return;

    // char path[PATH_MAX];
    // snprintf(path, sizeof(path), "%s/New Folder", ctx->current_directory);
    // g_create
    // if (g_mkdir(path, 0755) != 0) {
    //     g_warning("Could not create folder: %s", path);
    // } else {
    //     populate_files_in_container(ctx->current_directory, main_file_container, ctx);
    // }
}

void on_open_in_tab_clicked(GtkButton *button, gpointer user_data) {
    TabContext *ctx = get_current_tab_context();
    if (ctx && ctx->current_directory) {
        add_tab_with_directory(ctx->current_directory);
    }
}

// Helper function for adding rows to the properties dialog grid (converted from lambda)
static void add_property_row(GtkWidget *grid_widget, const char *label_text, const char *value_text, int row) {
    GtkWidget *label = gtk_label_new(label_text);
    gtk_widget_set_halign(label, GTK_ALIGN_END); // Align labels to the right
    gtk_grid_attach(GTK_GRID(grid_widget), label, 0, row, 1, 1);

    GtkWidget *value_label = gtk_label_new(value_text);
    gtk_widget_set_halign(value_label, GTK_ALIGN_START); // Align values to the left
    gtk_label_set_selectable(GTK_LABEL(value_label), TRUE); // Allow selecting text
    gtk_label_set_wrap(GTK_LABEL(value_label), TRUE); // Wrap long text
    gtk_grid_attach(GTK_GRID(grid_widget), value_label, 1, row, 1, 1);
}

void on_properties_clicked(GtkButton *button, gpointer user_data) {
    GtkPopover *popover = GTK_POPOVER(user_data); // Cast user_data to GtkPopover
    TabContext *ctx = get_current_tab_context();
    if (!ctx || !ctx->current_directory) {
        gtk_popover_popdown(popover);
        return;
    }

    GFile *file = g_file_new_for_path(ctx->current_directory);
    if (!file) {
        g_warning("Could not create GFile for path: %s", ctx->current_directory);
        gtk_popover_popdown(popover);
        return;
    }

    GError *error = NULL;
    GFileInfo *info = g_file_query_info(
        file,
        G_FILE_ATTRIBUTE_STANDARD_TYPE ","
        G_FILE_ATTRIBUTE_STANDARD_SIZE ","
        G_FILE_ATTRIBUTE_TIME_MODIFIED ","
        G_FILE_ATTRIBUTE_TIME_CREATED ","
        G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        G_FILE_QUERY_INFO_NONE,
        NULL,
        &error
    );

    if (error) {
        g_warning("Error querying file info for %s: %s", ctx->current_directory, error->message);
        g_error_free(error);
        g_object_unref(file);
        gtk_popover_popdown(popover);
        return;
    }

    char *kind_str = NULL;
    char *size_str = NULL;
    char *created_str = NULL;
    char *modified_str = NULL;

    GFileType file_type = g_file_info_get_file_type(info);
    switch (file_type) {
        case G_FILE_TYPE_REGULAR: {
            const char *content_type = g_file_info_get_content_type(info);
            kind_str = g_strdup_printf("File (%s)", content_type ? content_type : "unknown");
            break;
        }
        case G_FILE_TYPE_DIRECTORY:
            kind_str = g_strdup("Directory");
            break;
        case G_FILE_TYPE_SYMBOLIC_LINK:
            kind_str = g_strdup("Symbolic Link");
            break;
        case G_FILE_TYPE_SPECIAL:
            kind_str = g_strdup("Special File");
            break;
        default:
            kind_str = g_strdup("Unknown");
            break;
    }

    goffset file_size = g_file_info_get_size(info);
    size_str = g_strdup_printf("%lld bytes", (long long)file_size);
    if (file_size >= (1LL << 30)) {
        g_free(size_str);
        size_str = g_strdup_printf("%.2f GB", (double)file_size / (1LL << 30));
    } else if (file_size >= (1LL << 20)) {
        g_free(size_str);
        size_str = g_strdup_printf("%.2f MB", (double)file_size / (1LL << 20));
    } else if (file_size >= (1LL << 10)) {
        g_free(size_str);
        size_str = g_strdup_printf("%.2f KB", (double)file_size / (1LL << 10));
    }

    guint64 created_unix = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_CREATED);
    if (created_unix != 0) {
        GDateTime *dt_created = g_date_time_new_from_unix_local(created_unix);
        created_str = g_date_time_format(dt_created, "%Y-%m-%d %H:%M:%S");
        g_date_time_unref(dt_created);
    } else {
        created_str = g_strdup("N/A");
    }

    guint64 modified_unix = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
    if (modified_unix != 0) {
        GDateTime *dt_modified = g_date_time_new_from_unix_local(modified_unix);
        modified_str = g_date_time_format(dt_modified, "%Y-%m-%d %H:%M:%S");
        g_date_time_unref(dt_modified);
    } else {
        modified_str = g_strdup("N/A");
    }

    // Modern GTK4 dialog creation and display
    GtkWidget *dialog = gtk_dialog_new(); // Create dialog without buttons initially
    gtk_window_set_title(GTK_WINDOW(dialog), "Properties");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window)); // Set main window as parent
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE); // Make the dialog modal
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 250);

    // Create a GtkBox for the dialog content area
    GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(content_area, TRUE);
    gtk_widget_set_hexpand(content_area, TRUE);
    gtk_window_set_child(GTK_WINDOW(dialog), content_area); // Set this box as the main child of the dialog window

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_box_append(GTK_BOX(content_area), grid); // Add the grid to the dialog's content area

    // Use the helper function to add properties to the grid
    add_property_row(grid, "Path:", ctx->current_directory, 0);
    add_property_row(grid, "Kind:", kind_str, 1);
    add_property_row(grid, "Size:", size_str, 2);
    add_property_row(grid, "Created:", created_str, 3);
    add_property_row(grid, "Modified:", modified_str, 4);

    gtk_widget_set_visible(grid, TRUE); // Ensure the grid content is visible

    // Add an "OK" button to the dialog's action area
    GtkWidget *ok_button = gtk_button_new_with_label("OK");
    gtk_widget_set_halign(ok_button, GTK_ALIGN_END); // Align the button to the end (right)
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), ok_button, GTK_RESPONSE_OK); // Add button with a response ID

    // Connect to the "response" signal to destroy the dialog when a button is clicked
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);

    gtk_widget_set_visible(dialog, TRUE); // Show the dialog

    g_free(kind_str);
    g_free(size_str);
    g_free(created_str);
    g_free(modified_str);
    g_object_unref(info);
    g_object_unref(file);

    gtk_popover_popdown(popover); // Close the settings popover
}

void on_sort_by_clicked(GtkButton *button, gpointer user_data) {
    // Placeholder, integrate later with actual sorting mechanism
    g_print("Sort by clicked.\n");
}

void on_open_terminal_clicked(GtkButton *button, gpointer user_data) {
    TabContext *ctx = get_current_tab_context();
    if (!ctx || !ctx->current_directory) return;

    char command[PATH_MAX + 50];
    snprintf(command, sizeof(command), "gnome-terminal --working-directory='%s'", ctx->current_directory);
    system(command);
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
