#include "ui_builder.h"
#include "utils.h"
#include "main.h"
#include <stdlib.h>

/**
 * Callback for g_file_query_info_async that sets the icon for the image widget of a file item.
 */
static void on_file_info_ready(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GFile *file = G_FILE(source_object);
    GtkWidget *icon = GTK_WIDGET(user_data);
    GError *error = NULL;

    GFileInfo *info = g_file_query_info_finish(file, res, &error);
    if (info != NULL) {
        GIcon *gicon = g_file_info_get_icon(info);
        gtk_image_set_from_gicon(GTK_IMAGE(icon), gicon);
        g_object_unref(info);
    } else if (error != NULL) {
        g_warning("Error getting file icon: %s", error->message);
        g_error_free(error);
    }
}

/**
 * Sets up a list item factory for displaying files in a list.
 * This function is called when the factory is created.
 * It sets up the structure of each file item in the list. (only the structure, not the data)
 *
 * @param factory The GtkListItemFactory to set up
 * @param list_item The GtkListItem to set up
 */
void setup_file_item(GtkListItemFactory *factory, GtkListItem *list_item) {

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING);

    gtk_widget_set_focusable(box, TRUE);

    //
    // Creating icon
    //

    GtkWidget *icon = gtk_image_new();
    gtk_box_append(GTK_BOX(box), icon);

    //Sizing
    gtk_widget_set_hexpand(icon, TRUE);
    gtk_widget_set_vexpand(icon, TRUE);
    gtk_widget_set_halign(icon, GTK_ALIGN_FILL);
    gtk_widget_set_valign(icon, GTK_ALIGN_FILL);

    //
    // Creating label
    //

    GtkWidget *label = gtk_label_new("");
    gtk_label_set_max_width_chars(GTK_LABEL(label), 15); //max width
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END); //for some reason this is the syntax to set it to truncate after max width

    gtk_box_append(GTK_BOX(box), label);

    //Sizing
    gtk_widget_set_hexpand(label, FALSE);
    gtk_widget_set_vexpand(label, FALSE);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label, GTK_ALIGN_END);

    gtk_list_item_set_child(list_item, box);
}



/**
 * Binds data to a list item factory for displaying files in a list.
 * This function is called when the factory is used to create a list item.
 * It updates the contents of the file item with the actual file data. (that way the same structure can be reused for different files)
 *
 * @param factory The GtkListItemFactory to set up
 * @param list_item The GtkListItem to set up
 */
void bind_file_item(GtkListItemFactory *factory, GtkListItem *list_item) {

    GtkWidget *box = gtk_list_item_get_child(list_item);
    GtkWidget *icon = gtk_widget_get_first_child(box);
    GtkWidget *label = gtk_widget_get_next_sibling(icon);

    GFile *file = G_FILE(gtk_list_item_get_item(list_item));
    if (!file) {
        g_warning("No file available for list item");
        return;
    }

    // Store list item in the box data for later retrieval in right-click handler
    g_object_set_data(G_OBJECT(box), "list-item", list_item);

    // Add right-click gesture controller
    GtkGesture *right_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(right_click), GDK_BUTTON_SECONDARY);  // Right mouse button
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(right_click));
    g_signal_connect(right_click, "released", G_CALLBACK(file_right_clicked), box);

    // Update the label
    gtk_label_set_text(GTK_LABEL(label), g_file_get_basename(file));
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 100);

    // Get file icon asynchronously
    g_file_query_info_async(file,
                            G_FILE_ATTRIBUTE_STANDARD_ICON,
                            G_FILE_QUERY_INFO_NONE,
                            G_PRIORITY_DEFAULT,
                            NULL,
                            on_file_info_ready,
                            icon);
}

/**
 * Bints Bogos (amazing comments btw)
 * @param factory
 * @param list_item
 */
void setup_dir_item(GtkListItemFactory *factory, GtkListItem *list_item) {
    // This should set up the, well, directories in the sidebar
    GtkWidget *label = gtk_label_new(NULL);
    gtk_list_item_set_child(list_item, label);

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
}

/**
 * Binds binted Bogos
 * @param factory
 * @param list_item
 */
void bind_dir_item(GtkListItemFactory *factory, GtkListItem *list_item) {
    GFileInfo *file_info = G_FILE_INFO(gtk_list_item_get_item(list_item));
    const char *filename = g_file_info_get_name(file_info);

    GtkWidget *label = gtk_list_item_get_child(list_item);
    gtk_label_set_text(GTK_LABEL(label), filename);
}

/**
 * This is responsible for creating additional rows when unfolding directories
 * @param file
 * @return
 */
static GtkWidget* create_directory_row(GFile *file) {
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // Create an arrow icon for expansion indicator
    GtkWidget *arrow = gtk_image_new_from_icon_name("pan-end-symbolic");
    gtk_widget_set_margin_start(arrow, 2);
    gtk_widget_set_margin_end(arrow, 5);

    // Create the label for the directory name
    GtkWidget *label = gtk_label_new(g_file_get_basename(file));
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_hexpand(label, TRUE);

    // Add the arrow and label to the header
    gtk_box_append(GTK_BOX(header), arrow);
    gtk_box_append(GTK_BOX(header), label);
    gtk_box_append(GTK_BOX(row_box), header);

    // Store the arrow widget for later access
    g_object_set_data(G_OBJECT(row_box), "arrow-icon", arrow);

    // Attach gesture for click handling
    GtkGesture *click = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(click), GTK_PHASE_CAPTURE);
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    g_signal_connect(click, "pressed", G_CALLBACK(on_directory_row_clicked), row_box);
    gtk_widget_add_controller(header, GTK_EVENT_CONTROLLER(click));

    // Store the file as data for later expansion
    g_object_set_data_full(G_OBJECT(row_box), "file", g_object_ref(file), g_object_unref);

    return row_box;
}

/**
 * Handles the sidebar directories being clicked
 * @param gesture
 * @param n_press
 * @param x
 * @param y
 * @param user_data
 */
static void on_directory_row_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
    GtkWidget *row_box = GTK_WIDGET(user_data);
    GtkWidget *arrow = g_object_get_data(G_OBJECT(row_box), "arrow-icon");

    GtkWidget *existing_list = g_object_get_data(G_OBJECT(row_box), "child-list");
    if (existing_list) {
        // Collapse the directory
        gtk_widget_unparent(existing_list);
        g_object_set_data(G_OBJECT(row_box), "child-list", NULL);

        // Change arrow to point right (collapsed)
        gtk_image_set_from_icon_name(GTK_IMAGE(arrow), "pan-end-symbolic");
        return;
    }

    // Change arrow to point down (expanded)
    gtk_image_set_from_icon_name(GTK_IMAGE(arrow), "pan-down-symbolic");

    GFile *dir = g_object_get_data(G_OBJECT(row_box), "file");
    if (!dir) return;

    GError *error = NULL;
    GFileEnumerator *enumerator = g_file_enumerate_children(dir, "standard::name,standard::type", G_FILE_QUERY_INFO_NONE, NULL, &error);
    if (!enumerator) {
        g_warning("Failed to enumerate directory: %s", error->message);
        g_error_free(error);
        return;
    }

    GtkWidget *child_list = gtk_list_box_new();
    gtk_widget_set_margin_start(child_list, 10);

    GFileInfo *info;
    while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) {
        if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY) {
            GFile *child_file = g_file_get_child(dir, g_file_info_get_name(info));
            GtkWidget *child_row = create_directory_row(child_file);
            gtk_list_box_append(GTK_LIST_BOX(child_list), child_row);
            g_object_unref(child_file);
            g_object_unref(info);
        }
    }

    GFile *clicked_dir = g_object_get_data(G_OBJECT(user_data), "file");
    char *path = g_file_get_path(clicked_dir);
    TabContext* ctx = get_current_tab_context();
    populate_files_in_container(path, ctx->scrolled_window, ctx);
    g_free(path);

    g_object_unref(enumerator);

    gtk_box_append(GTK_BOX(row_box), child_list);
    g_object_set_data(G_OBJECT(row_box), "child-list", child_list);
}

/**
 * guh
 * Creates the widget structure for the side panel
 * @return GtkWidget* containing the side panel
 */
left_box_t create_left_box() {

    left_box_t left_box;

    left_box.side_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING);

    left_box.undo_button = GTK_BUTTON(gtk_button_new_from_icon_name("edit-undo-symbolic"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(left_box.undo_button), "Undo last operation");

    left_box.redo_button = GTK_BUTTON(gtk_button_new_from_icon_name("edit-redo-symbolic"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(left_box.redo_button), "Redo last undone operation");

    gtk_box_append(GTK_BOX(hbox), GTK_WIDGET(left_box.undo_button));
    gtk_box_append(GTK_BOX(hbox), GTK_WIDGET(left_box.redo_button));
    gtk_box_append(GTK_BOX(left_box.side_panel), hbox);

    gtk_widget_set_margin_start(left_box.side_panel, SPACING);
    gtk_widget_set_margin_end(left_box.side_panel, SPACING);
    gtk_widget_set_margin_top(left_box.side_panel, SPACING);
    gtk_widget_set_margin_bottom(left_box.side_panel, SPACING);

    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_append(GTK_BOX(left_box.side_panel), sw);

    GtkWidget *list = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), list);
    gtk_widget_set_vexpand(list, TRUE);
    gtk_widget_set_hexpand(list, TRUE);

    // Disable the default blue highlighting of list items using CSS
    gtk_widget_add_css_class(list, "sidebar-list");

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        ".sidebar-list { background-color: transparent; }"
        ".sidebar-list row:selected { background-color: transparent; }"
        ".sidebar-list row:hover { background-color: rgba(128, 128, 128, 0.1); }");

    GdkDisplay *display = gtk_widget_get_display(list);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    GFile *root = g_file_new_for_path("/");

    GError *error = NULL;
    GFileEnumerator *enumerator = g_file_enumerate_children(root, "standard::name,standard::type", G_FILE_QUERY_INFO_NONE, NULL, &error);
    if (!enumerator) {
        g_warning("Failed to enumerate root: %s", error->message);
        g_error_free(error);
    }

    GFileInfo *info;
    while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) {
        if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY) {
            GFile *child = g_file_get_child(root, g_file_info_get_name(info));
            GtkWidget *row = create_directory_row(child);
            gtk_list_box_append(GTK_LIST_BOX(list), row);
            g_object_unref(child);
            g_object_unref(info);
        }
    }

    g_object_unref(enumerator);
    g_object_unref(root);

    return left_box;
}


/**
 * Creates a toolbar with navigation controls
 * @param default_directory Initial directory to display
 * @return Toolbar struct containing all toolbar widgets
 */
toolbar_t create_toolbar(const char* default_directory) {
    toolbar_t toolbar;

    // Create the toolbar container
    toolbar.toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING);

    // Create the "Up" button
    toolbar.up_button = gtk_button_new_from_icon_name("go-up-symbolic");

    // Create the directory entry
    toolbar.directory_entry = gtk_entry_new();
    gtk_widget_set_hexpand(toolbar.directory_entry, TRUE);

    if (default_directory) {
        gtk_editable_set_text(GTK_EDITABLE(toolbar.directory_entry), default_directory);
    }

    gtk_entry_set_icon_from_gicon(GTK_ENTRY(toolbar.directory_entry),
                                  GTK_ENTRY_ICON_PRIMARY,
                                  g_themed_icon_new("folder-symbolic"));
    gtk_entry_set_icon_activatable(GTK_ENTRY(toolbar.directory_entry),
                                   GTK_ENTRY_ICON_PRIMARY,
                                   FALSE);
    gtk_entry_set_icon_tooltip_text(GTK_ENTRY(toolbar.directory_entry),
                                   GTK_ENTRY_ICON_PRIMARY,
                                   "Current Directory");

    gtk_entry_set_icon_from_gicon(GTK_ENTRY(toolbar.directory_entry),
                              GTK_ENTRY_ICON_SECONDARY,
                              g_themed_icon_new("object-select-symbolic"));
    gtk_entry_set_icon_activatable(GTK_ENTRY(toolbar.directory_entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   TRUE);
    gtk_entry_set_icon_tooltip_text(GTK_ENTRY(toolbar.directory_entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   "Go to Directory");

    // Create the search entry
    toolbar.search_entry = gtk_search_entry_new();

    // Add widgets to toolbar
    gtk_box_append(GTK_BOX(toolbar.toolbar), toolbar.up_button);
    gtk_box_append(GTK_BOX(toolbar.toolbar), toolbar.directory_entry);
    gtk_box_append(GTK_BOX(toolbar.toolbar), toolbar.search_entry);

    return toolbar;
}

dialog_t create_dialog(const char* title, const char* message) {
    dialog_t dialog;

    //Window
    dialog.dialog = GTK_WINDOW(gtk_window_new());
    gtk_window_set_title(dialog.dialog, title);
    gtk_widget_set_size_request(GTK_WIDGET(dialog.dialog), 400, 100);

    //Container
    GtkBox* vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING*2));
    gtk_widget_set_margin_start(GTK_WIDGET(vbox), SPACING*2);
    gtk_widget_set_margin_end(GTK_WIDGET(vbox), SPACING*2);
    gtk_widget_set_margin_top(GTK_WIDGET(vbox), SPACING*2);
    gtk_widget_set_margin_bottom(GTK_WIDGET(vbox), SPACING*2);

    // Label
    GtkWidget* label = gtk_label_new(message);
    gtk_widget_set_vexpand(label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);

    //Entry
    dialog.entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_icon_from_icon_name(dialog.entry,GTK_ENTRY_ICON_SECONDARY, "object-select-symbolic");

    // Adding children
    gtk_box_append(vbox, label);
    gtk_box_append(vbox, GTK_WIDGET(dialog.entry));
    gtk_window_set_child(dialog.dialog, GTK_WIDGET(vbox));

    return dialog;
}

GtkPopoverMenu* create_file_context_menu(const char* params) {
    GMenu *menu = g_menu_new();

    //
    // Add items to the menu
    //

    // Delete item
    GMenuItem *delete_item = g_menu_item_new("Delete", "win.delete");
    g_menu_item_set_action_and_target_value(delete_item, "win.delete", g_variant_new_string(params));
    g_menu_append_item(menu, delete_item);
    g_object_unref(delete_item);

    // Rename item
    GMenuItem *rename_item = g_menu_item_new("Rename", "win.rename");
    g_menu_item_set_action_and_target_value(rename_item, "win.rename", g_variant_new_string(params));
    g_menu_append_item(menu, rename_item);
    g_object_unref(rename_item);

    GMenuModel *menu_model = G_MENU_MODEL(menu);

    // 2. Create the PopoverMenu from the model
    GtkPopoverMenu* popover = GTK_POPOVER_MENU(gtk_popover_menu_new_from_model(menu_model));
    g_object_unref(menu);

    return popover;
}

GtkPopoverMenu* create_directory_context_menu(const char* params) {
    GMenu *menu = g_menu_new();

    //
    // Add items to the menu
    //

    // New Folder item
    GMenuItem *folder_item = g_menu_item_new("New Folder", "win.new_folder");
    g_menu_item_set_action_and_target_value(folder_item, "win.new_folder", g_variant_new_string(params));
    g_menu_append_item(menu, folder_item);
    g_object_unref(folder_item);

    // Open in Tab item
    GMenuItem *tab_item = g_menu_item_new("Open in New Tab", "win.open_in_tab");
    g_menu_item_set_action_and_target_value(tab_item, "win.open_in_tab", g_variant_new_string(params));
    g_menu_append_item(menu, tab_item);
    g_object_unref(tab_item);

    // Open in Terminal item
    GMenuItem *terminal_item = g_menu_item_new("Open in Terminal", "win.open_terminal");
    g_menu_item_set_action_and_target_value(terminal_item, "win.open_terminal", g_variant_new_string(params));
    g_menu_append_item(menu, terminal_item);
    g_object_unref(terminal_item);

    // Sort by item
    GMenuItem *sort_item = g_menu_item_new("Sort by", "win.sort_dir");
    g_menu_item_set_action_and_target_value(sort_item, "win.sort_dir", g_variant_new_string(params));
    g_menu_append_item(menu, sort_item);
    g_object_unref(sort_item);

    // Properties item
    GMenuItem *properties_item = g_menu_item_new("Properties", "win.dir_properties");
    g_menu_item_set_action_and_target_value(properties_item, "win.dir_properties", g_variant_new_string(params));
    g_menu_append_item(menu, properties_item);
    g_object_unref(properties_item);

    GMenuModel *menu_model = G_MENU_MODEL(menu);

    // 2. Create the PopoverMenu from the model
    GtkPopoverMenu* popover = GTK_POPOVER_MENU(gtk_popover_menu_new_from_model(menu_model));
    g_object_unref(menu);

    return popover;
}

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

GtkWindow* create_properties_window(const char* file_path) {

    GFile *file = g_file_new_for_path(file_path);
    if (!file) {
        g_error("Could not create GFile for path: %s", file_path);
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
        g_error("Error querying file info for %s: %s", file_path, error->message);
        g_error_free(error);
        g_object_unref(file);
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


    GtkWidget* dialog = gtk_window_new(); // Create dialog without buttons initially
    gtk_window_set_title(GTK_WINDOW(dialog), "Properties");
    gtk_widget_set_size_request(dialog, 400, 250);

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
    add_property_row(grid, "Path:", file_path, 0);
    add_property_row(grid, "Kind:", kind_str, 1);
    add_property_row(grid, "Size:", size_str, 2);
    add_property_row(grid, "Created:", created_str, 3);
    add_property_row(grid, "Modified:", modified_str, 4);

    g_free(kind_str);
    g_free(size_str);
    g_free(created_str);
    g_free(modified_str);
    g_object_unref(info);
    g_object_unref(file);

    return GTK_WINDOW(dialog);
}