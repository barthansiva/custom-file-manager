#include "ui_builder.h"
#include "utils.h"
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

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
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
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 56);

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
 * Creates the widget structure for the side panel
 * @return GtkWidget* containing the side panel
 */
GtkWidget* create_side_box(void) {
    // Create the side box
    GtkWidget* side_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(side_box, 100, -1); // Set a fixed width for the side box

    // Add a label to the side box
    GtkWidget* label = gtk_label_new("Side Panel");
    gtk_box_append(GTK_BOX(side_box), label);

    // Add a flow box to the side box (for future use)
    GtkWidget* flow_box = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow_box), GTK_SELECTION_NONE);
    gtk_box_append(GTK_BOX(side_box), flow_box);

    return side_box;
}
