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
gboolean show_hidden_files = FALSE;

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

static void menu_file_properties_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void menu_new_folder_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void menu_open_terminal_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void menu_open_tab_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void menu_dir_properties_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void menu_sort_dir_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

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

void directory_right_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

void sort_files_by(gboolean ascending, const char *criteria);

void on_sort_name_asc(GSimpleAction *action, GVariant *param, gpointer user_data);

void on_sort_name_desc(GSimpleAction *action, GVariant *param, gpointer user_data);

void on_sort_date_asc(GSimpleAction *action, GVariant *param, gpointer user_data);

void on_sort_date_desc(GSimpleAction *action, GVariant *param, gpointer user_data);

void on_sort_size_asc(GSimpleAction *action, GVariant *param, gpointer user_data);

void on_sort_size_desc(GSimpleAction *action, GVariant *param, gpointer user_data);

static void toggle_hidden_action_handler(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void menu_preview_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data);

typedef struct {
    gboolean ascending;
} SortContext;



static const GActionEntry win_actions[] = {
    { "delete", menu_delete_clicked, "s", NULL, NULL },
    { "rename", menu_rename_clicked, "s", NULL, NULL },
    { "properties", menu_file_properties_clicked, "s", NULL, NULL },
    { "preview", menu_preview_clicked, "s", NULL, NULL },
    { "new_folder", menu_new_folder_clicked, "s", NULL, NULL },
    { "open_terminal", menu_open_terminal_clicked, "s", NULL, NULL },
    { "open_in_tab", menu_open_tab_clicked, "s", NULL, NULL },
    { "dir_properties", menu_dir_properties_clicked, "s", NULL, NULL }
};

static const GActionEntry win_entries[] = {
    { "sort_name_asc",  on_sort_name_asc,  NULL, NULL, NULL },
    { "sort_name_desc", on_sort_name_desc, NULL, NULL, NULL },
    { "sort_date_asc",  on_sort_date_asc,  NULL, NULL, NULL },
    { "sort_date_desc", on_sort_date_desc, NULL, NULL, NULL },
    { "sort_size_asc",  on_sort_size_asc,  NULL, NULL, NULL },
    { "sort_size_desc", on_sort_size_desc, NULL, NULL, NULL },
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
    gtk_widget_set_tooltip_text(settings_button, "Folder context menu");
    g_signal_connect(settings_button, "clicked", G_CALLBACK(on_settings_button_clicked), NULL);
    gtk_box_append(GTK_BOX(toolbar.toolbar), settings_button);

    g_action_map_add_action_entries(G_ACTION_MAP(window), win_actions, G_N_ELEMENTS(win_actions), window);
    g_action_map_add_action_entries(G_ACTION_MAP(window), win_entries, G_N_ELEMENTS(win_entries), window);

    //
    // Show hidden toggle
    //
    GSimpleAction *toggle_action = g_simple_action_new_stateful("toggle_hidden", NULL, g_variant_new_boolean(show_hidden_files));
    g_signal_connect(toggle_action, "change-state", G_CALLBACK(toggle_hidden_action_handler), NULL);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(toggle_action));

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
    GListModel *files = G_LIST_MODEL(gtk_multi_selection_get_model(GTK_MULTI_SELECTION(model)));

    GFile *file = g_list_model_get_item(files, position);
    GFileInfo *info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    GFileType type = g_file_info_get_file_type(info);

    if (type == G_FILE_TYPE_DIRECTORY) {
        gtk_revealer_set_reveal_child(GTK_REVEALER(ctx->preview_revealer), FALSE);
        const char *path = g_file_get_path(file);
        populate_files_in_container(path, ctx->scrolled_window, ctx);
    } else {
        open_file_with_default_app(file);
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
    // Clean up old data
    g_clear_pointer(&ctx->current_directory, g_free);
    ctx->current_directory = g_strdup(directory);

    size_t file_count = 0;
    GListStore* files = get_files_in_directory(directory, &file_count, show_hidden_files);
    if (!files) return;

    // Save the raw file store in context
    ctx->file_store = files;

    // Create a GtkSortListModel wrapping the file store
    GtkSortListModel *sort_model = gtk_sort_list_model_new(G_LIST_MODEL(files), NULL);
    gtk_sort_list_model_set_incremental(sort_model, FALSE); // full sorting

    ctx->sort_model = sort_model;

    // Set up selection and factory
    GtkMultiSelection *selection = gtk_multi_selection_new(G_LIST_MODEL(sort_model));
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_file_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_file_item), NULL);

    // Create the grid view
    GtkWidget *view = gtk_grid_view_new(GTK_SELECTION_MODEL(selection), factory);
    gtk_grid_view_set_single_click_activate(GTK_GRID_VIEW(view), FALSE);

    // Save the view in context
    ctx->file_grid_view = GTK_GRID_VIEW(view);

    // Set click and right-click handlers
    g_signal_connect(view, "activate", G_CALLBACK(file_clicked), ctx);

    GtkGesture *right_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(right_click), GDK_BUTTON_SECONDARY);
    gtk_widget_add_controller(view, GTK_EVENT_CONTROLLER(right_click));
    g_signal_connect(right_click, "released", G_CALLBACK(directory_right_clicked), ctx);

    // Swap in the new view
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(container), view);

    // Update path and tab label
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
    GListStore *raw_files = get_files_in_directory(ctx->current_directory, &file_count, show_hidden_files);
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
    // Get the current tab context
    TabContext *ctx = get_current_tab_context();
    if (!ctx || !ctx->file_grid_view) {
        g_warning("Invalid tab context or scrolled window");
        return;
    }

    GtkGridView* view = ctx->file_grid_view;
    GFile** selected_files = get_selection(view, &count);

    GtkListItem *list_item = GTK_LIST_ITEM(g_object_get_data(G_OBJECT(box), "list-item"));
    if (!list_item) {
        g_warning("Invalid list item");
        if (selected_files) g_free(selected_files);
        return;
    }

    GFile *file = G_FILE(gtk_list_item_get_item(list_item));
    if (!file) {
        g_warning("Invalid file");
        if (selected_files) g_free(selected_files);
        return;
    }

    char* params;
    if (count == 0) {
        params = g_file_get_path(file);
    } else {
        params = join_basenames(selected_files, count, ";", g_file_get_path(file));
        g_free(selected_files);
    }

    GtkPopoverMenu* popover = create_file_context_menu(params, window);
    gtk_widget_set_parent(GTK_WIDGET(popover), box);
    const GdkRectangle rect = { gtk_widget_get_width(box)/2, gtk_widget_get_height(box), 1, 1 };
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
    gtk_popover_popup(GTK_POPOVER(popover));
}

void directory_right_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {

    TabContext *ctx = user_data;

    GtkPopoverMenu* popover = create_directory_context_menu(ctx->current_directory, window, show_hidden_files);
    gtk_widget_set_parent(GTK_WIDGET(popover), ctx->scrolled_window);
    const GdkRectangle rect = { (int)x, (int)y, 1, 1 };
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
    gtk_popover_popup(GTK_POPOVER(popover));
}

static void menu_delete_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const char *params = g_variant_get_string(parameter, NULL);
    size_t count;
    char** files = split_basenames(params,";", &count);
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
    char** files = split_basenames(params,";", &count);
    dialog_t dialog = create_dialog("Rename File", "Enter new name for the file:");

    gtk_window_set_transient_for(GTK_WINDOW(dialog.dialog), GTK_WINDOW(window));
    gtk_window_set_modal(GTK_WINDOW(dialog.dialog), TRUE);

    char* path = strdup(files[0]);

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

void new_folder_confirm(GtkEntry* self, gpointer data1, gpointer data2) {
    char* dir = data2 ? data2 : data1;
    const char *folder_name = gtk_editable_get_text(GTK_EDITABLE(self));

    if (folder_name == NULL || strlen(folder_name) == 0) {
        g_free(dir);
        return; // No folder name provided
    }

    // Construct the full path for the new folder
    char *new_folder_path = g_build_filename(dir, folder_name, NULL);

    // Create a GFile for the new folder path
    GFile *new_folder = g_file_new_for_path(new_folder_path);

    GError *error = NULL;
    // Create the directory with GFile
    gboolean success = g_file_make_directory(new_folder, NULL, &error);

    if (!success) {
        // If the folder already exists or there's another error
        g_warning("Could not create folder '%s': %s", new_folder_path, error->message);
        g_error_free(error);
    } else {
        g_print("Successfully created folder: %s\n", new_folder_path);
    }

    // Clean up
    g_object_unref(new_folder);
    g_free(new_folder_path);

    reload_current_directory();
}

static void menu_file_properties_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {

    const char *params = g_variant_get_string(parameter, NULL);
    size_t count;
    char** files = split_basenames(params,";", &count);

    char* path = strdup(files[0]);

    GtkWindow* properties_window = create_properties_window(path);
    gtk_window_set_transient_for(properties_window, GTK_WINDOW(window));
    gtk_window_set_modal(properties_window, TRUE);

    gtk_window_present(properties_window);
}

void undo_button_clicked(GtkButton *button, gpointer user_data) {
    undo_last_operation();
    reload_current_directory();
}

void redo_button_clicked(GtkButton *button, gpointer user_data) {
    redo_last_undo();
    reload_current_directory();
}

static void menu_new_folder_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {

    const gchar* dir = g_variant_get_string(parameter, NULL);
    if (!dir) return;

    const dialog_t dialog = create_dialog("New Folder", "Enter name for the new folder:");

    gtk_window_set_transient_for(GTK_WINDOW(dialog.dialog), GTK_WINDOW(window));
    gtk_window_set_modal(GTK_WINDOW(dialog.dialog), TRUE);

    gtk_editable_set_text(GTK_EDITABLE(dialog.entry), "New folder");
    g_signal_connect(GTK_WIDGET(dialog.entry), "icon-press", G_CALLBACK(new_folder_confirm), dir);
    g_signal_connect(GTK_WIDGET(dialog.entry), "activate", G_CALLBACK(new_folder_confirm), dir);

    g_signal_connect(GTK_WIDGET(dialog.entry), "icon-press", G_CALLBACK(close_dialog_window), dialog.dialog);
    g_signal_connect(GTK_WIDGET(dialog.entry), "activate", G_CALLBACK(close_dialog_window), dialog.dialog);

    gtk_window_present(GTK_WINDOW(dialog.dialog));
}

static void menu_open_terminal_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const gchar* dir = g_variant_get_string(parameter, NULL);
    if (!dir) return;

    char command[PATH_MAX + 50];
    snprintf(command, sizeof(command), "gnome-terminal --working-directory='%s'", dir);
    system(command);
}

static void menu_open_tab_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const gchar* dir = g_variant_get_string(parameter, NULL);
    if (!dir) return;

    add_tab_with_directory(dir);
}

static void menu_dir_properties_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const gchar* dir = g_variant_get_string(parameter, NULL);
    if (!dir) return;

    GtkWindow* properties_window = create_properties_window(dir);
    gtk_window_set_transient_for(properties_window, GTK_WINDOW(window));
    gtk_window_set_modal(properties_window, TRUE);

    gtk_window_present(properties_window);
}

static void menu_sort_dir_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const gchar* dir = g_variant_get_string(parameter, NULL);
    if (!dir) return;

    TabContext* ctx = get_current_tab_context();

    g_print("Sorting is not yet implemented\n");

    reload_current_directory();
}

void on_settings_button_clicked(GtkButton *button, gpointer user_data) {

    TabContext *ctx = get_current_tab_context();

    GtkPopoverMenu* popover = create_directory_context_menu(ctx->current_directory, window, show_hidden_files);
    gtk_widget_set_parent(GTK_WIDGET(popover), GTK_WIDGET(button));
    const GdkRectangle rect = { gtk_widget_get_width(GTK_WIDGET(button))/2, gtk_widget_get_height(GTK_WIDGET(button)), 1, 1 };
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
    gtk_popover_popup(GTK_POPOVER(popover));
}

gint compare_by_name(gconstpointer a, gconstpointer b, gpointer user_data) {
    const char *name_a = g_file_get_basename(G_FILE(a));
    const char *name_b = g_file_get_basename(G_FILE(b));
    int result = g_ascii_strcasecmp(name_a, name_b);
    SortContext *ctx = (SortContext *)user_data;
    return ctx->ascending ? result : -result;
}


gint compare_by_date(gconstpointer a, gconstpointer b, gpointer user_data) {
    GFileInfo *info_a = g_file_query_info(G_FILE(a), G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    GFileInfo *info_b = g_file_query_info(G_FILE(b), G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (!info_a || !info_b) {
        if (info_a) g_object_unref(info_a);
        if (info_b) g_object_unref(info_b);
        return 0;
    }
    guint64 mod_a = g_file_info_get_attribute_uint64(info_a, G_FILE_ATTRIBUTE_TIME_MODIFIED);
    guint64 mod_b = g_file_info_get_attribute_uint64(info_b, G_FILE_ATTRIBUTE_TIME_MODIFIED);
    g_object_unref(info_a);
    g_object_unref(info_b);
    gint result = (mod_a > mod_b) - (mod_a < mod_b);
    SortContext *ctx = (SortContext *)user_data;
    return ctx->ascending ? result : -result;
}


gint compare_by_size(gconstpointer a, gconstpointer b, gpointer user_data) {
    GFileInfo *info_a = g_file_query_info(G_FILE(a), G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    GFileInfo *info_b = g_file_query_info(G_FILE(b), G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (!info_a || !info_b) {
        if (info_a) g_object_unref(info_a);
        if (info_b) g_object_unref(info_b);
        return 0;
    }
    goffset size_a = g_file_info_get_size(info_a);
    goffset size_b = g_file_info_get_size(info_b);
    g_object_unref(info_a);
    g_object_unref(info_b);
    gint result = (size_a > size_b) - (size_a < size_b);
    SortContext *ctx = (SortContext *)user_data;
    return ctx->ascending ? result : -result;
}


void sort_files_by(gboolean ascending, const char *criteria) {
    TabContext *ctx = get_current_tab_context();
    if (!ctx || !ctx->file_store || !ctx->file_grid_view || !ctx->sort_model) return;

    GtkSorter *sorter = NULL;
    SortContext *sort_ctx = g_new(SortContext, 1);
    sort_ctx->ascending = ascending;

    if (g_strcmp0(criteria, "name") == 0) {
        sorter = GTK_SORTER(gtk_custom_sorter_new(compare_by_name, sort_ctx, g_free));
    } else if (g_strcmp0(criteria, "date") == 0) {
        sorter = GTK_SORTER(gtk_custom_sorter_new(compare_by_date, sort_ctx, g_free));
    } else if (g_strcmp0(criteria, "size") == 0) {
        sorter = GTK_SORTER(gtk_custom_sorter_new(compare_by_size, sort_ctx, g_free));
    } else {
        g_free(sort_ctx);
        return;
    }

    gtk_sort_list_model_set_sorter(ctx->sort_model, sorter);
    g_object_unref(sorter);
}


void on_sort_name_asc(GSimpleAction *action, GVariant *param, gpointer user_data) {
    sort_files_by(TRUE, "name");
}
void on_sort_name_desc(GSimpleAction *action, GVariant *param, gpointer user_data) {
    sort_files_by(FALSE, "name");
}
void on_sort_date_asc(GSimpleAction *action, GVariant *param, gpointer user_data) {
    sort_files_by(TRUE, "date");
}
void on_sort_date_desc(GSimpleAction *action, GVariant *param, gpointer user_data) {
    sort_files_by(FALSE, "date");
}
void on_sort_size_asc(GSimpleAction *action, GVariant *param, gpointer user_data) {
    sort_files_by(TRUE, "size");
}
void on_sort_size_desc(GSimpleAction *action, GVariant *param, gpointer user_data) {
    sort_files_by(FALSE, "size");
}

static void toggle_hidden_action_handler(GSimpleAction *action, GVariant *state, gpointer user_data) {
    show_hidden_files = g_variant_get_boolean(state);
    g_simple_action_set_state(action, state);

    TabContext *ctx = get_current_tab_context();
    if (!ctx || !ctx->current_directory) return;

    char *path_copy = g_strdup(ctx->current_directory);
    populate_files_in_container(path_copy, ctx->scrolled_window, ctx);
    g_free(path_copy);
}

static void menu_preview_clicked(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const char *file_path = g_variant_get_string(parameter, NULL);
    if (!file_path) return;

    GFile *file = g_file_new_for_path(file_path);

    GFileInfo *info = g_file_query_info(file,
        G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (!info) {
        g_object_unref(file);
        return;
    }

    const char *mime = g_file_info_get_content_type(info);
    if (g_str_has_prefix(mime, "text/")) {
        TabContext *ctx = get_current_tab_context();
        update_preview_text(ctx, file);
    }

    g_object_unref(info);
    g_object_unref(file);
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
