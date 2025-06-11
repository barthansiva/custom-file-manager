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
    GtkWidget *preview_text_view;
    GtkWidget *preview_revealer;
    GtkGridView *file_grid_view;
    GtkSortListModel *sort_model;
    GListStore *file_store;
} TabContext;

void open_file_with_default_app(GFile *file);

TabContext* get_current_tab_context();

gboolean populate_files(const char *directory);

void populate_files_in_container(const char *directory, GtkWidget *container, TabContext *ctx);

void file_right_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

#endif //MAIN_H
