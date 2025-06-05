#include "ui_builder.h"
#include "utils.h"
#include <stdlib.h>

/**
 * Creates an appropriate icon image for a file
 * @param file File structure with metadata
 * @return GtkWidget* containing the icon image
 */
static GtkWidget* create_file_image(file_t* file) {
    GtkWidget *icon;

    if (file->is_dir) {
        // Default GTK folder icon (not supposed to be used liek that)
        icon = gtk_image_new_from_icon_name("folder-symbolic");
    } else {
        // Use generic icon for all of the files for now
        icon = gtk_image_new_from_icon_name("text-x-generic-symbolic");
    }

    gtk_image_set_pixel_size(GTK_IMAGE(icon), 48);
    return icon;
}

/**
 * Creates GTK widgets for an array of files
 * @param files Array of file_t structures
 * @param file_count Number of files in the array
 * @param click_handler Function to call when a file is clicked
 * @return Array of GtkWidget pointers (must be freed by the caller)
 */
GtkWidget** create_file_widgets(file_t* files, size_t file_count, FileClickHandler click_handler) {
    if (files == NULL || file_count == 0) {
        return NULL;
    }

    GtkWidget** widgets = malloc(file_count * sizeof(GtkWidget*));

    for (size_t i = 0; i < file_count; i++) {

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

        GtkWidget *icon = create_file_image(&files[i]);
        gtk_box_append(GTK_BOX(box), icon);

        //Sizing
        gtk_widget_set_hexpand(icon, TRUE);
        gtk_widget_set_vexpand(icon, TRUE);
        gtk_widget_set_halign(icon, GTK_ALIGN_FILL);
        gtk_widget_set_valign(icon, GTK_ALIGN_FILL);

        //
        // Creating label
        //

        GtkWidget *label = gtk_label_new(files[i].name);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 15); //max width
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END); //for some reason this is the syntax to set it to truncate after max width

        gtk_box_append(GTK_BOX(box), label);

        //Sizing
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(label, GTK_ALIGN_END);
        gtk_widget_set_hexpand(label, FALSE);
        gtk_widget_set_vexpand(label, FALSE);

        //
        // Memory management BS I don't understand
        //

        file_t* file_copy = copy_file_data(&files[i]);
        if (!file_copy) {
            g_printerr("Failed to copy file data for %s\n", files[i].name);
            g_object_unref(button);
            continue;
        }

        // Connect the click signal with the copied data
        g_signal_connect(button, "clicked", G_CALLBACK(click_handler), file_copy);

        // Frees memory automatically when the button is destroyed, thanks GTK
        g_signal_connect_data(button, "destroy", G_CALLBACK(free_file_data),
                             file_copy, (GClosureNotify)NULL, G_CONNECT_SWAPPED);

        widgets[i] = button;
    }

    return widgets;
}
