// snake.c
#include "snake.h"
#include <stdlib.h>
#include <time.h>

#define CELL_SIZE 20
#define WIDTH 20
#define HEIGHT 20

typedef struct Segment {
    int x, y;
    struct Segment *next;
} Segment;

static Segment *snake = NULL;
static int food_x, food_y;
static int dx = 1, dy = 0;
static gboolean running = TRUE;

static guint game_timeout_id = 0;

static void place_food() {
    food_x = rand() % WIDTH;
    food_y = rand() % HEIGHT;
}

static void reset_game() {
    Segment *s = snake;
    while (s) {
        Segment *tmp = s;
        s = s->next;
        free(tmp);
    }

    snake = malloc(sizeof(Segment));
    snake->x = WIDTH / 2;
    snake->y = HEIGHT / 2;
    snake->next = NULL;

    dx = 1; dy = 0;
    place_food();
    running = TRUE;
}

static gboolean snake_tick(gpointer user_data) {
    if (!running) return G_SOURCE_REMOVE;

    Segment *new_head = malloc(sizeof(Segment));
    new_head->x = snake->x + dx;
    new_head->y = snake->y + dy;
    new_head->next = snake;
    snake = new_head;

    // Check wall collision
    if (snake->x < 0 || snake->x >= WIDTH || snake->y < 0 || snake->y >= HEIGHT) {
        reset_game();
        return G_SOURCE_CONTINUE;
    }

    // Check self collision
    Segment *s = snake->next;
    while (s) {
        if (s->x == snake->x && s->y == snake->y) {
            reset_game();
            return G_SOURCE_CONTINUE;
        }
        s = s->next;
    }

    // Check food
    if (snake->x == food_x && snake->y == food_y) {
        place_food();
    } else {
        // Remove tail
        Segment *prev = NULL;
        s = snake;
        while (s->next) {
            prev = s;
            s = s->next;
        }
        free(s);
        if (prev) prev->next = NULL;
    }

    gtk_widget_queue_draw(GTK_WIDGET(user_data));
    return G_SOURCE_CONTINUE;
}

static void on_snake_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    // Clear
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);

    // Draw food
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_rectangle(cr, food_x * CELL_SIZE, food_y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    cairo_fill(cr);

    // Draw snake
    cairo_set_source_rgb(cr, 0, 1, 0);
    Segment *s = snake;
    while (s) {
        cairo_rectangle(cr, s->x * CELL_SIZE, s->y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        s = s->next;
    }
    cairo_fill(cr);
}

static gboolean on_snake_key(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    switch (keyval) {
        case GDK_KEY_Up:    if (dy == 0) { dx = 0; dy = -1; } break;
        case GDK_KEY_Down:  if (dy == 0) { dx = 0; dy = 1; } break;
        case GDK_KEY_Left:  if (dx == 0) { dx = -1; dy = 0; } break;
        case GDK_KEY_Right: if (dx == 0) { dx = 1; dy = 0; } break;
    }
    return TRUE;
}

static void on_snake_window_close(GtkWindow *window, gpointer user_data) {
    if (game_timeout_id != 0) {
        g_source_remove(game_timeout_id);
        game_timeout_id = 0;
    }
    // Now allow the window to close normally:
    gtk_window_destroy(window);
}

void launch_snake(GtkButton *button, gpointer user_data) {
    srand(time(NULL));
    reset_game();

    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Snake");
    gtk_window_set_default_size(GTK_WINDOW(window), WIDTH * CELL_SIZE, HEIGHT * CELL_SIZE);
    g_signal_connect(window, "close-request", G_CALLBACK(on_snake_window_close), NULL);


    GtkWidget *area = gtk_drawing_area_new();
    gtk_window_set_child(GTK_WINDOW(window), area);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), on_snake_draw, NULL, NULL);

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(area, controller);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_snake_key), NULL);

    gtk_widget_set_focusable(area, TRUE);
    gtk_widget_grab_focus(area);

    game_timeout_id = g_timeout_add(150, snake_tick, area);

    gtk_widget_set_visible(window, TRUE);
}
