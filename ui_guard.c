#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GtkTextBuffer *buffer;
    const char *command;
} TaskContext;

static void append_to_buffer(GtkTextBuffer *buffer, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
}

static gboolean read_output(GIOChannel *source, GIOCondition condition, gpointer data) {
    TaskContext *ctx = (TaskContext *)data;
    gchar *string;
    gsize size;
    GIOStatus status = g_io_channel_read_line(source, &string, &size, NULL, NULL);

    if (status == G_IO_STATUS_NORMAL) {
        append_to_buffer(ctx->buffer, string);
        g_free(string);
        return TRUE;
    }

    g_free(string);
    return FALSE;
}

static void launch_task(GtkTextBuffer *buffer, const char *cmd) {
    TaskContext *ctx = g_new(TaskContext, 1);
    ctx->buffer = buffer;
    ctx->command = cmd;

    GError *error = NULL;
    gchar **argv = g_strsplit(cmd, " ", -1);
    gint stdout_fd;
    GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD;
    GPid child_pid;

    if (!g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &child_pid, NULL, &stdout_fd, NULL, &error)) {
        append_to_buffer(buffer, error->message);
        g_error_free(error);
        g_strfreev(argv);
        return;
    }

    GIOChannel *channel = g_io_channel_unix_new(stdout_fd);
    g_io_add_watch(channel, G_IO_IN | G_IO_HUP, read_output, ctx);
    g_io_channel_unref(channel);
    g_strfreev(argv);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MatCom Guard 🛡️");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget *label = gtk_label_new("Escaneo de Dispositivos USB:");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 2);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    // Comando real del servicio USB
    launch_task(buffer, "./Devices_service/devices_service");

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
