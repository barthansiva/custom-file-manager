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

    GtkWidget *label = gtk_label_new(g_file_get_basename(file));
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_hexpand(label, TRUE);

    gtk_box_append(GTK_BOX(header), label);
    gtk_box_append(GTK_BOX(row_box), header);

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

    GtkWidget *existing_list = g_object_get_data(G_OBJECT(row_box), "child-list");
    if (existing_list) {
        gtk_widget_unparent(existing_list);
        g_object_set_data(G_OBJECT(row_box), "child-list", NULL);
        return;
    }

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
GtkWidget* create_left_box() {

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    //gtk_widget_set_size_request(box, 200, -1);

    GtkWidget *label = gtk_label_new("Directories");
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_append(GTK_BOX(box), sw);

    GtkWidget *list = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), list);
    gtk_widget_set_vexpand(list, TRUE);
    gtk_widget_set_hexpand(list, TRUE);

    GFile *root = g_file_new_for_path("/");

    GError *error = NULL;
    GFileEnumerator *enumerator = g_file_enumerate_children(root, "standard::name,standard::type", G_FILE_QUERY_INFO_NONE, NULL, &error);
    if (!enumerator) {
        g_warning("Failed to enumerate root: %s", error->message);
        g_error_free(error);
        return box;
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

    return box;
}


/**
 * Creates a toolbar with navigation controls
 * @param default_directory Initial directory to display
 * @return Toolbar struct containing all toolbar widgets
 */
Toolbar create_toolbar(const char* default_directory) {
    Toolbar toolbar;

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