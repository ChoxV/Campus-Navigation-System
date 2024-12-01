#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>

#define MAX_VERTICES 6
#define VERTEX_RADIUS 25
#define HOVER_RADIUS 30

// Color scheme
const double COLOR_BACKGROUND[3] = {0.95, 0.97, 1.0};     // Light blue-gray
const double COLOR_PRIMARY[3] = {0.2, 0.6, 0.9};          // Blue
const double COLOR_SECONDARY[3] = {0.3, 0.8, 0.4};        // Green
const double COLOR_HIGHLIGHT[3] = {0.95, 0.3, 0.3};       // Red
const double COLOR_TEXT[3] = {0.2, 0.2, 0.3};            // Dark blue-gray

int graph[MAX_VERTICES][MAX_VERTICES] = {
    {0, 10, 0, 0, 0, 0},
    {10, 0, 0, 12, 0, 0},
    {0, 0, 0, 10, 0, 0},
    {0, 12, 10, 0, 2, 0},
    {0, 0, 0, 2, 0, 8},
    {0, 0, 0, 0, 8, 0}
};

const char *vertices[MAX_VERTICES] = {"NHSRCL", "Pahune", "Manthan", "AcadBlock", "Prastuti", "SportsComplex"};
int vertices_coords[MAX_VERTICES][2] = {
    {100, 100}, {300, 100}, {200, 300},
    {400, 300}, {500, 100}, {600, 300}
};

int shortest_path[MAX_VERTICES];
int shortest_path_count = 0;
int selected_vertex = -1;
int hovered_vertex = -1;

typedef struct {
    GtkWidget *source_combo;
    GtkWidget *dest_combo;
    GtkWidget *drawing_area;
    GtkWidget *distance_label;
} AppWidgets;


static GtkWidget* create_styled_button(const char* label) {
    GtkWidget *button = gtk_button_new_with_label(label);
    GtkStyleContext *context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(context, "suggested-action");
    return button;
}

//
static GtkWidget* create_styled_combo() {
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_widget_set_size_request(combo, 150, -1);
    return combo;
}

// Apply rounded rectangle clipping
static void clip_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    double degrees = G_PI / 180.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + w - r, y + h - r, r, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + r, y + h - r, r, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + r, y + r, r, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
}

// Draw shadow effect
static void draw_shadow(cairo_t *cr, double x, double y, double width, double height, double radius) {
    for(int i = 3; i >= 0; i--) {
        cairo_set_source_rgba(cr, 0, 0, 0, 0.03);
        clip_rounded_rect(cr, x - i, y - i, width + 2*i, height + 2*i, radius + i);
        cairo_fill(cr);
    }
}



static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    // Draw background
    cairo_set_source_rgb(cr, COLOR_BACKGROUND[0], COLOR_BACKGROUND[1], COLOR_BACKGROUND[2]);
    cairo_paint(cr);

    // Draw grid pattern
    cairo_set_source_rgba(cr, 0, 0, 0, 0.03);
    for(int i = 0; i < 800; i += 20) {
        cairo_move_to(cr, i, 0);
        cairo_line_to(cr, i, 600);
        cairo_move_to(cr, 0, i);
        cairo_line_to(cr, 800, i);
    }
    cairo_stroke(cr);

    // Draw edges
    cairo_set_line_width(cr, 2);
    for (int i = 0; i < MAX_VERTICES; i++) {
        for (int j = i + 1; j < MAX_VERTICES; j++) {
            if (graph[i][j] > 0) {
                // Draw edge shadow
                cairo_set_source_rgba(cr, 0, 0, 0, 0.1);
                cairo_move_to(cr, vertices_coords[i][0] + 2, vertices_coords[i][1] + 2);
                cairo_line_to(cr, vertices_coords[j][0] + 2, vertices_coords[j][1] + 2);
                cairo_stroke(cr);

                // Draw actual edge
                cairo_set_source_rgb(cr, COLOR_PRIMARY[0], COLOR_PRIMARY[1], COLOR_PRIMARY[2]);
                cairo_move_to(cr, vertices_coords[i][0], vertices_coords[i][1]);
                cairo_line_to(cr, vertices_coords[j][0], vertices_coords[j][1]);
                cairo_stroke(cr);
                // Draw weight with background
                int mid_x = (vertices_coords[i][0] + vertices_coords[j][0]) / 2;
                int mid_y = (vertices_coords[i][1] + vertices_coords[j][1]) / 2;
                
                char weight_str[10];
                sprintf(weight_str, "%d", graph[i][j]);
                
                // Weight background
                cairo_set_source_rgb(cr, 1, 1, 1);
                cairo_arc(cr, mid_x, mid_y, 15, 0, 2 * G_PI);
                cairo_fill(cr);
                
                // Weight text
                cairo_set_source_rgb(cr, COLOR_TEXT[0], COLOR_TEXT[1], COLOR_TEXT[2]);
                cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 12);
                cairo_move_to(cr, mid_x - 6, mid_y + 4);
                cairo_show_text(cr, weight_str);
            }
        }
    }

    // Highlight shortest path
    if (shortest_path_count > 0) {
        cairo_set_source_rgb(cr, COLOR_HIGHLIGHT[0], COLOR_HIGHLIGHT[1], COLOR_HIGHLIGHT[2]);
        cairo_set_line_width(cr, 4);
        for (int i = 0; i < shortest_path_count - 1; i++) {
            cairo_move_to(cr, vertices_coords[shortest_path[i]][0], vertices_coords[shortest_path[i]][1]);
            cairo_line_to(cr, vertices_coords[shortest_path[i + 1]][0], vertices_coords[shortest_path[i + 1]][1]);
            cairo_stroke(cr);
        }
    }

    // Draw vertices
    for (int i = 0; i < MAX_VERTICES; i++) {
        // Draw shadow
        draw_shadow(cr, vertices_coords[i][0] - VERTEX_RADIUS, 
                   vertices_coords[i][1] - VERTEX_RADIUS,
                   2 * VERTEX_RADIUS, 2 * VERTEX_RADIUS, VERTEX_RADIUS);

        // Draw vertex
        cairo_arc(cr, vertices_coords[i][0], vertices_coords[i][1], 
                 i == hovered_vertex ? HOVER_RADIUS : VERTEX_RADIUS, 0, 2 * G_PI);
        
        if (i == selected_vertex) {
            cairo_set_source_rgb(cr, COLOR_HIGHLIGHT[0], COLOR_HIGHLIGHT[1], COLOR_HIGHLIGHT[2]);
        } else {
            cairo_set_source_rgb(cr, COLOR_SECONDARY[0], COLOR_SECONDARY[1], COLOR_SECONDARY[2]);
        }
        cairo_fill_preserve(cr);
        
        cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
        cairo_set_line_width(cr, 2);
        cairo_stroke(cr);

        // Draw vertex label
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12);
        cairo_set_source_rgb(cr, 1, 1, 1);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, vertices[i], &extents);
        cairo_move_to(cr, 
                     vertices_coords[i][0] - extents.width/2,
                     vertices_coords[i][1] + extents.height/2);
        cairo_show_text(cr, vertices[i]);
    }

    return FALSE;
}
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    for (int i = 0; i < MAX_VERTICES; i++) {
        double dx = event->x - vertices_coords[i][0];
        double dy = event->y - vertices_coords[i][1];
        if (sqrt(dx * dx + dy * dy) <= VERTEX_RADIUS) {
            selected_vertex = i;
            gtk_widget_queue_draw(widget);
            break;
        }
    }
    return TRUE;
}

// Event handler for motion notify
static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    hovered_vertex = -1;
    for (int i = 0; i < MAX_VERTICES; i++) {
        double dx = event->x - vertices_coords[i][0];
        double dy = event->y - vertices_coords[i][1];
        if (sqrt(dx * dx + dy * dy) <= HOVER_RADIUS) {
            hovered_vertex = i;
            gtk_widget_queue_draw(widget);
            break;
        }
    }
    return TRUE;
}

// Event handler for button release
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    selected_vertex = -1;
    gtk_widget_queue_draw(widget);
    return TRUE;
}

// Function to find the shortest path using Dijkstra's algorithm
void find_shortest_path(int source, int destination) {
    bool visited[MAX_VERTICES] = {false};
    int distance[MAX_VERTICES];
    int parent[MAX_VERTICES];

    for (int i = 0; i < MAX_VERTICES; i++) {
        distance[i] = INT_MAX;
        parent[i] = -1;
    }
    distance[source] = 0;

    for (int i = 0; i < MAX_VERTICES - 1; i++) {
        int min_dist = INT_MAX, min_index;
        for (int v = 0; v < MAX_VERTICES; v++) {
            if (!visited[v] && distance[v] < min_dist) {
                min_dist = distance[v];
                min_index = v;
            }
        }

        visited[min_index] = true;

        for (int v = 0; v < MAX_VERTICES; v++) {
            if (!visited[v] && graph[min_index][v] && distance[min_index] != INT_MAX &&
                distance[min_index] + graph[min_index][v] < distance[v]) {
                distance[v] = distance[min_index] + graph[min_index][v];
                parent[v] = min_index;
            }
        }
    }

    shortest_path_count = 0;
    for (int v = destination; v != -1; v = parent[v]) {
        shortest_path[shortest_path_count++] = v;
    }
    for (int i = 0; i < shortest_path_count / 2; i++) {
        int temp = shortest_path[i];
        shortest_path[i] = shortest_path[shortest_path_count - 1 - i];
        shortest_path[shortest_path_count - 1 - i] = temp;
    }
}

// Event handler for the "Find Shortest Path" button
static void on_find_path_clicked(GtkWidget *button, gpointer data) {
    AppWidgets *widgets = (AppWidgets *)data;

    int source = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets->source_combo));
    int destination = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets->dest_combo));

    if (source == -1 || destination == -1 || source == destination) {
        gtk_label_set_text(GTK_LABEL(widgets->distance_label), "Invalid source or destination");
        return;
    }

    find_shortest_path(source, destination);

    char distance_str[100];
    sprintf(distance_str, "Shortest distance: %d", graph[source][destination]);
    gtk_label_set_text(GTK_LABEL(widgets->distance_label), distance_str);

    gtk_widget_queue_draw(widgets->drawing_area);
}


int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Load CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "button.suggested-action {"
        "  background: linear-gradient(to bottom, #4a90e2, #357abd);"
        "  border: none;"
        "  border-radius: 5px;"
        "  color: white;"
        "  padding: 8px 15px;"
        "  text-shadow: 0 1px rgba(0,0,0,0.3);"
        "  box-shadow: 0 1px 3px rgba(0,0,0,0.2);"
        "}"
        "button.suggested-action:hover {"
        "  background: linear-gradient(to bottom, #357abd, #4a90e2);"
        "}"
        "combobox {"
        "  border-radius: 5px;"
        "  padding: 5px;"
        "}"
        "label {"
        "  font-weight: bold;"
        "  color: #333;"
        "}", -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Create main window with dark title bar
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Campus Navigation System");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create main container with padding
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 15);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Create styled control panel
    GtkWidget *control_panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), control_panel, FALSE, FALSE, 0);

    AppWidgets *widgets = g_new(AppWidgets, 1);

    // Create styled widgets
    GtkWidget *source_label = gtk_label_new("From:");
    widgets->source_combo = create_styled_combo();
    GtkWidget *dest_label = gtk_label_new("To:");
    widgets->dest_combo = create_styled_combo();
    
    // Populate combo boxes
    for (int i = 0; i < MAX_VERTICES; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->source_combo), vertices[i]);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->dest_combo), vertices[i]);
    }

    // Create styled button and label
    GtkWidget *find_path_button = create_styled_button("Find Shortest Path");
    widgets->distance_label = gtk_label_new("Select source and destination");

    // Add widgets to control panel with proper spacing
    gtk_box_pack_start(GTK_BOX(control_panel), source_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(control_panel), widgets->source_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(control_panel), dest_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(control_panel), widgets->dest_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(control_panel), find_path_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(control_panel), widgets->distance_label, FALSE, FALSE, 0);

    // Create drawing area with shadow effect
    widgets->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(widgets->drawing_area, 800, 550);
    gtk_widget_set_events(widgets->drawing_area, 
        GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | 
        GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | 
        GDK_LEAVE_NOTIFY_MASK);

    // Create frame for drawing area
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(frame), widgets->drawing_area);
    gtk_box_pack_start(GTK_BOX(main_box), frame, TRUE, TRUE, 0);

    // Connect signals
    g_signal_connect(widgets->drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(widgets->drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(widgets->drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(widgets->drawing_area, "button-release-event", G_CALLBACK(on_button_release), NULL);
     g_signal_connect(find_path_button, "clicked", G_CALLBACK(on_find_path_clicked), widgets);

    // Show all widgets
    gtk_widget_show_all(window);

    gtk_main();

    g_free(widgets);
    return 0;
}