#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define SOCKET_PATH "/tmp/devices.sock"

static GtkTextBuffer *global_buffer;

static void append_to_buffer(GtkTextBuffer *buffer, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
}

static gboolean handle_client(GIOChannel *source, GIOCondition condition, gpointer data) {
    (void)data;
    if (condition & G_IO_HUP || condition & G_IO_ERR) return FALSE;

    gchar buf[512];
    gsize len = 0;
    GIOStatus status = g_io_channel_read_line(source, &buf[0], sizeof(buf), &len, NULL);

    if (status == G_IO_STATUS_NORMAL) {
        append_to_buffer(global_buffer, buf);
    }
    return TRUE;
}

static gboolean accept_connection(GIOChannel *source, GIOCondition condition, gpointer data) {
    int server_fd = g_io_channel_unix_get_fd(source);
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd >= 0) {
        GIOChannel *client_channel = g_io_channel_unix_new(client_fd);
        g_io_add_watch(client_channel, G_IO_IN | G_IO_HUP | G_IO_ERR, handle_client, data);
    }
    return TRUE;
}

static void iniciar_socket_servidor(GtkTextBuffer *buffer) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return;
    }

    unlink(SOCKET_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    GIOChannel *channel = g_io_channel_unix_new(server_fd);
    g_io_add_watch(channel, G_IO_IN, accept_connection, NULL);
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
    global_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    iniciar_socket_servidor(global_buffer);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
