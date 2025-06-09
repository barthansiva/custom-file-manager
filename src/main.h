#ifndef MAIN_H
#define MAIN_H

#include <gtk/gtk.h>
#include "utils.h"
#include "ui_builder.h"
#include <stdlib.h>

typedef struct {
    GtkWidget *scrolled_window;
    GtkWidget *tab_label;
    char *current_directory;
} TabContext;

TabContext* get_current_tab_context();

gboolean populate_files(const char *directory);

void populate_files_in_container(const char *directory, GtkWidget *container, TabContext *ctx);

#endif //MAIN_H
