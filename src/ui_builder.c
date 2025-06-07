#include "ui_builder.h"
#include "utils.h"
#include "main.h"
#include <stdlib.h>

/**
 * Callback for g_file_query_info_async that sets the icon for the image widget
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

void setup_file_item(GtkListItemFactory *factory, GtkListItem *list_item) {

    // Everything is inside a button so we can click it
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_focusable(button, TRUE);

    // This makes it look not shit (flat button instead of a raised one)
    GtkStyleContext *context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(context, "flat");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_button_set_child(GTK_BUTTON(button), box);

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

    gtk_list_item_set_child(list_item, button);
}

void bind_file_item(GtkListItemFactory *factory, GtkListItem *list_item) {
    GtkWidget *button = gtk_list_item_get_child(list_item);
    GtkWidget *icon = gtk_widget_get_first_child(gtk_widget_get_first_child(button));
    GtkWidget *label = gtk_widget_get_next_sibling(icon);

    g_print("Binding file item\n");

    GFile *file = G_FILE(gtk_list_item_get_item(list_item));
    if (!file) {
        g_warning("No file available for list item");
        return;
    }

    // Update the label
    char *basename = g_file_get_basename(file);
    gtk_label_set_text(GTK_LABEL(label), basename);
    g_free(basename);  // Free the basename

    // Store the path as a string in button data instead of the GFile object
    char *file_path = g_file_get_path(file);
    if (file_path) {
        // Set the path as a property on the button
        g_object_set_data_full(G_OBJECT(button), "file-path",
                              g_strdup(file_path), (GDestroyNotify)g_free);
        g_free(file_path);
    }

    // Disconnect any existing signal handler to avoid duplicates
    g_signal_handlers_disconnect_by_func(button, G_CALLBACK(file_clicked), NULL);

    // Connect click handler to the button without passing file data
    g_signal_connect(button, "clicked", G_CALLBACK(file_clicked), NULL);

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
