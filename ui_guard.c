#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>
#include <pango/pango.h>

typedef struct {
    const char *socket_path;
    const char *label;
    const char *exec_rel_path;
    GtkTextBuffer *buffer;
} ServicioUI;

static gboolean aceptar_conexion(GIOChannel *source, GIOCondition condition, gpointer data) {
    ServicioUI *svc = (ServicioUI *)data;
    int server_fd = g_io_channel_unix_get_fd(source);
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd < 0) {
        perror("❌ Error al aceptar conexión");
        return TRUE;
    }

    char buf[512];
    ssize_t bytes = read(client_fd, buf, sizeof(buf) - 1);
    if (bytes > 0) {
        buf[bytes] = '\0';
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(svc->buffer, &end);
        gtk_text_buffer_insert(svc->buffer, &end, buf, -1);
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

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(server_fd, 5) < 0) {
        perror("❌ Error en bind() o listen()");
        close(server_fd);
        return;
    }

    GIOChannel *channel = g_io_channel_unix_new(server_fd);
    g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR, aceptar_conexion, svc);
}

static void lanzar_servicio(const char *exec_rel_path) {
    GError *error = NULL;
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) return;
    exe_path[len] = '\0';
    char *dir = dirname(exe_path);

    char full_exec[PATH_MAX];
    snprintf(full_exec, sizeof(full_exec), "%s/%s", dir, exec_rel_path);

    // 🔴 Agregado: matar procesos anteriores antes de lanzar uno nuevo
    char comando[PATH_MAX + 50];
    snprintf(comando, sizeof(comando), "pkill -f '%s' > /dev/null 2>&1", full_exec);
    system(comando);

    // 🟢 Continúa con la ejecución normal
    char base_dir[PATH_MAX];
    strcpy(base_dir, full_exec);
    char *last_slash = strrchr(base_dir, '/');
    if (last_slash) *last_slash = '\0';

    gchar *argv[] = {full_exec, NULL};
    g_spawn_async(base_dir, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
    if (error) {
        g_printerr("❌ %s\n", error->message);
        g_error_free(error);
    }
}


int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    ServicioUI servicios[] = {
        {"/tmp/devices.sock", "🖴 Dispositivos USB", "Devices_service/devices_service", NULL},
        {"/tmp/processes.sock", "🧠 Procesos del sistema", "Process_service/process_service", NULL},
        {"/tmp/ports.sock", "🛡️ Escaneo de Puertos", "Ports_service/ports_service", NULL}
    };
    int total = sizeof(servicios) / sizeof(servicios[0]);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MatCom Guard 🛡️");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    for (int i = 0; i < total; ++i) {
        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_hexpand(scrolled, TRUE);
        gtk_widget_set_vexpand(scrolled, TRUE);

        GtkWidget *text_view = gtk_text_view_new();
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
        gtk_container_add(GTK_CONTAINER(scrolled), text_view);

        // Estilo monoespaciado como consola
        PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
        gtk_widget_override_font(text_view, font_desc);
        pango_font_description_free(font_desc);

        servicios[i].buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

        GtkWidget *label = gtk_label_new(servicios[i].label);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled, label);

        iniciar_socket_servidor(&servicios[i]);
        lanzar_servicio(servicios[i].exec_rel_path);
    }

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
