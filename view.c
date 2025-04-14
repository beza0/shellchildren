#include "view.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "controller.h"
#include "model.h"

static GtkWidget *output_view;
static GtkWidget *input_entry;
static GtkWidget *message_view;
static ShmBuf *shmp;
static int terminal_id = -1;  // Her terminal için benzersiz ID

void update_messages(GtkWidget *message_view, ShmBuf *shmp) {
    if (message_view == NULL || shmp == NULL) return;

    char *msg_all = model_read_messages(shmp);
    if (!msg_all) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    char *line = strtok(msg_all, "\n");
    while (line) {
        char match[20];
        snprintf(match, sizeof(match), "[to:%d]", terminal_id);

        if (strstr(line, "[to:all]") != NULL || strstr(line, match) != NULL) {
            GtkTextIter end;
            gtk_text_buffer_get_end_iter(buffer, &end);
            gtk_text_buffer_insert(buffer, &end, line, -1);
            gtk_text_buffer_insert(buffer, &end, "\n", -1);
        }
        line = strtok(NULL, "\n");
    }
    free(msg_all);
}

gboolean timer_callback(gpointer data) {
    update_messages(message_view, shmp);
    return TRUE;
}

void on_enter(GtkEntry *entry, gpointer user_data) {
    const char *input = gtk_entry_get_text(entry);
    if (input == NULL || *input == '\0') {
        return;
    }

    char *result = handle_input(shmp, input);
    if (result) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
        gtk_text_buffer_set_text(buffer, result, -1);
        free(result);
    }

    gtk_entry_set_text(entry, "");
}

void on_new_terminal_clicked(GtkButton *button, gpointer user_data) {
    pid_t pid = fork();
    if (pid == 0) {
        char *args[] = {"./shell", NULL};
        execv(args[0], args);
        perror("execv failed");
        exit(EXIT_FAILURE);
    }
}

void setup_gui(int *argc, char ***argv, ShmBuf *shared_buf) {
    shmp = shared_buf;
    gtk_init(argc, argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    // Pencere başlığına terminal ID’yi yaz
    char title[64];
    snprintf(title, sizeof(title), "Multi-User Shell [ID: %d]", terminal_id);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *new_terminal_button = gtk_button_new_with_label("Yeni Terminal Aç");
    g_signal_connect(new_terminal_button, "clicked", G_CALLBACK(on_new_terminal_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), new_terminal_button, FALSE, FALSE, 0);

    output_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_view), FALSE);
    GtkWidget *output_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(output_scroll), output_view);
    gtk_box_pack_start(GTK_BOX(vbox), output_scroll, TRUE, TRUE, 0);

    message_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_view), FALSE);
    GtkWidget *msg_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(msg_scroll), message_view);
    gtk_box_pack_start(GTK_BOX(vbox), msg_scroll, TRUE, TRUE, 0);

    input_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(input_entry), "Enter command or @msg [id/all] message");
    g_signal_connect(input_entry, "activate", G_CALLBACK(on_enter), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), input_entry, FALSE, FALSE, 0);

    g_timeout_add(1000, timer_callback, NULL);

    gtk_widget_show_all(window);
    gtk_main();
}

int main(int argc, char *argv[]) {
    ShmBuf *shared_buf = buf_init();
    if (shared_buf == NULL) {
        fprintf(stderr, "Failed to initialize shared buffer\n");
        return EXIT_FAILURE;
    }

    // Benzersiz terminal ID ata
    terminal_id = allocate_terminal_id(shared_buf);

    setup_gui(&argc, &argv, shared_buf);
    cleanup(shared_buf);
    return EXIT_SUCCESS;
}
