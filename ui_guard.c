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
    if (!buffer || !text) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
    printf("📥 Se agregó al buffer GTK: %s\n", text);
}

static gboolean accept_connection(GIOChannel *source, GIOCondition condition, gpointer data) {
    (void)condition;
    int server_fd = g_io_channel_unix_get_fd(source);
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd < 0) {
        perror("❌ Error al aceptar conexión");
        return TRUE;
    }

    printf("✅ Nueva conexión aceptada\n");

    char buf[512];
    ssize_t bytes = read(client_fd, buf, sizeof(buf) - 1);
    if (bytes > 0) {
        buf[bytes] = '\0';
        printf("🟢 Recibido: %s\n", buf);
        append_to_buffer(global_buffer, buf);
    } else {
        perror("❌ read");
    }

    close(client_fd);
    return TRUE;
}

static void iniciar_socket_servidor(GtkTextBuffer *buffer) {
    global_buffer = buffer;

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("❌ Error creando socket");
        return;
    }

    unlink(SOCKET_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("❌ Error en bind()");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        perror("❌ Error en listen()");
        close(server_fd);
        return;
    }

    printf("🧠 Servidor escuchando en %s\n", SOCKET_PATH);

    GIOChannel *channel = g_io_channel_unix_new(server_fd);
    g_io_add_watch(channel, G_IO_IN, accept_connection, NULL);
}

static void lanzar_devices_service() {
    GError *error = NULL;
    gchar *argv[] = {"./Devices_service/devices_service", NULL};

    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
        fprintf(stderr, "❌ Error al lanzar devices_service: %s\n", error->message);
        g_error_free(error);
    } else {
        printf("🚀 devices_service lanzado automáticamente\n");
    }
}

static GtkWidget* crear_interfaz(GtkTextBuffer **buffer_ref) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *label = gtk_label_new("Escaneo de Dispositivos USB:");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 2);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    *buffer_ref = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    return box;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MatCom Guard 🛡️");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkTextBuffer *buffer;
    GtkWidget *contenido = crear_interfaz(&buffer);
    gtk_container_add(GTK_CONTAINER(window), contenido);

    iniciar_socket_servidor(buffer);
    lanzar_devices_service();

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}