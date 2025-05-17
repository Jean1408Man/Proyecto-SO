#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

typedef struct {
    const char *socket_path;
    const char *label;
    const char *exec_path;
    GtkTextBuffer *buffer;
} ServicioUI;

static gboolean aceptar_conexion(GIOChannel *source, GIOCondition condition, gpointer data) {
    ServicioUI *svc = (ServicioUI *)data;
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
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(svc->buffer, &end);
        gtk_text_buffer_insert(svc->buffer, &end, buf, -1);
    } else {
        perror("❌ read");
    }

    close(client_fd);
    return TRUE;
}

static void iniciar_socket_servidor(ServicioUI *svc) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("❌ Error creando socket");
        return;
    }

    unlink(svc->socket_path);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, svc->socket_path, sizeof(addr.sun_path) - 1);

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

    printf("🧠 Servidor escuchando en %s\n", svc->socket_path);

    GIOChannel *channel = g_io_channel_unix_new(server_fd);
    g_io_add_watch(channel, G_IO_IN, aceptar_conexion, svc);
}

static void lanzar_servicio(const char *exec_path) {
    GError *error = NULL;
    char comando[256];
    snprintf(comando, sizeof(comando), "pgrep -f '%s' > /dev/null", exec_path);
    if (system(comando) == 0) {
        printf("⚠️  %s ya está en ejecución.\n", exec_path);
        return;
    }

    gchar *argv[] = {(gchar *)exec_path, NULL};
    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
        fprintf(stderr, "❌ Error al lanzar %s: %s\n", exec_path, error->message);
        g_error_free(error);
    } else {
        printf("🚀 %s lanzado automáticamente\n", exec_path);
    }
}

static GtkWidget* crear_interfaz(GtkTextBuffer **buffer_ref, const char *titulo) {
    GtkWidget *frame = gtk_frame_new(titulo);
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_container_add(GTK_CONTAINER(frame), scrolled);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    *buffer_ref = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    return frame;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    ServicioUI servicios[] = {
        {"/tmp/devices.sock", "Dispositivos USB:", "./Devices_service/devices_service", NULL},
    };
    int total = sizeof(servicios) / sizeof(servicios[0]);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MatCom Guard 🛡️");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    for (int i = 0; i < total; ++i) {
        GtkWidget *seccion = crear_interfaz(&servicios[i].buffer, servicios[i].label);
        gtk_box_pack_start(GTK_BOX(vbox), seccion, TRUE, TRUE, 5);
        iniciar_socket_servidor(&servicios[i]);
        lanzar_servicio(servicios[i].exec_path);
    }

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
