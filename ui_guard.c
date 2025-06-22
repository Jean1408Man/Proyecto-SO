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
#include <pango/pango-font.h>

typedef struct {
    const char *socket_path;
    const char *label;
    const char *exec_rel_path;
    GtkTextBuffer *buffer;
} ServicioUI;

static GtkWidget *umbral_spin;
static GtkWidget *intervalo_spin;

static GtkWidget *cpu_umbral_spin;
static GtkWidget *mem_umbral_spin;
static GtkWidget *intervalo_proc_spin;

static GtkWidget *start_port_spin;
static GtkWidget *end_port_spin;

static ServicioUI servicios[] = {
    {"/tmp/devices.sock",   "🖴 Dispositivos USB",    "Devices_service/devices_service", NULL},
    {"/tmp/processes.sock", "🧠 Procesos del sistema","Processes_Service/pm_demo",     NULL},
    {"/tmp/ports.sock",     "🛡️ Escaneo de Puertos",  "Ports_service/bin/escaner",      NULL}
};
static const int total = sizeof(servicios) / sizeof(servicios[0]);

static gboolean
aceptar_conexion(GIOChannel *source, GIOCondition condition, gpointer data)
{
    ServicioUI *svc = (ServicioUI *)data;
    int server_fd = g_io_channel_unix_get_fd(source);
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
        return TRUE;

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

static void
iniciar_socket_servidor(ServicioUI *svc)
{
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0)
        return;

    unlink(svc->socket_path);
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, svc->socket_path, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(server_fd, 5) < 0) {
        close(server_fd);
        return;
    }

    GIOChannel *channel = g_io_channel_unix_new(server_fd);
    g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR, aceptar_conexion, svc);
}

static void
lanzar_servicio(const char *exec_rel_path, int arg1, int arg2)
{
    GError *error = NULL;
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
        return;
    exe_path[len] = '\0';
    char *dir = dirname(exe_path);

    char full_exec[PATH_MAX];
    snprintf(full_exec, sizeof(full_exec), "%s/%s", dir, exec_rel_path);

    // Matar instancias previas
    char comando[PATH_MAX + 50];
    snprintf(comando, sizeof(comando), "pkill -f '%s' > /dev/null 2>&1", full_exec);
    system(comando);

    char arg1_str[16], arg2_str[16];
    snprintf(arg1_str, sizeof(arg1_str), "%d", arg1);
    snprintf(arg2_str, sizeof(arg2_str), "%d", arg2);

    gchar *argv[] = { full_exec, arg1_str, arg2_str, NULL };
    g_spawn_async(dir, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);

    if (error) {
        g_printerr("❌ %s\n", error->message);
        g_error_free(error);
    }
}

static void
lanzar_servicio_procesos(const char *exec_rel_path,
                         int cpu_thresh, int mem_thresh, int intervalo)
{
    GError *error = NULL;
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
        return;
    exe_path[len] = '\0';
    char *dir = dirname(exe_path);

    char full_exec[PATH_MAX];
    snprintf(full_exec, sizeof(full_exec), "%s/%s", dir, exec_rel_path);

    // Matar instancias previas
    char comando[PATH_MAX + 50];
    snprintf(comando, sizeof(comando), "pkill -f '%s' > /dev/null 2>&1", full_exec);
    system(comando);

    char cpu_str[16], mem_str[16], intervalo_str[16];
    snprintf(cpu_str,      sizeof(cpu_str),      "%d", cpu_thresh);
    snprintf(mem_str,      sizeof(mem_str),      "%d", mem_thresh);
    snprintf(intervalo_str,sizeof(intervalo_str),"%d", intervalo);

    gchar *argv[] = { full_exec, cpu_str, mem_str, intervalo_str, NULL };
    g_spawn_async(dir, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);

    if (error) {
        g_printerr("❌ %s\n", error->message);
        g_error_free(error);
    }
}

static void
aplicar_configuracion(GtkButton *btn, gpointer user_data)
{
    /* Valores para Dispositivos */
    int umbral_disp    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(umbral_spin));
    int intervalo_disp = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(intervalo_spin));

    /* Valores para Procesos */
    int umbral_cpu     = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cpu_umbral_spin));
    int umbral_mem     = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mem_umbral_spin));
    int intervalo_proc = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(intervalo_proc_spin));

    /* Valores para Escaneo de Puertos */
    int start_port     = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(start_port_spin));
    int end_port       = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(end_port_spin));

    for (int i = 0; i < total; ++i) {
        /* Limpiar consola antes de lanzar */
        gtk_text_buffer_set_text(servicios[i].buffer, "", -1);

        if (i == 0) {
            // Dispositivos
            lanzar_servicio(servicios[i].exec_rel_path, umbral_disp, intervalo_disp);
        } else if (i == 1) {
            // Procesos
            lanzar_servicio_procesos(servicios[i].exec_rel_path,
                                     umbral_cpu, umbral_mem, intervalo_proc);
        } else if (i == 2) {
            // Puertos
            lanzar_servicio(servicios[i].exec_rel_path, start_port, end_port);
        }
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(user_data),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "✅ La configuración ha sido aplicada correctamente."
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* Callback específico para el botón "Ejecutar" en la pestaña de Puertos */
static void
ejecutar_escaneo_puertos(GtkButton *btn, gpointer user_data)
{
    int start_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(start_port_spin));
    int end_port   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(end_port_spin));

    gtk_text_buffer_set_text(servicios[2].buffer, "", -1);
    lanzar_servicio(servicios[2].exec_rel_path, start_port, end_port);
}

int
main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MatCom Guard 🛡️");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    /* --- Pestañas de Servicios --- */
    for (int i = 0; i < total; ++i) {
        GtkWidget *text_view = gtk_text_view_new();
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);

        PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
        gtk_widget_override_font(text_view, font_desc);
        pango_font_description_free(font_desc);

        servicios[i].buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

        GtkWidget *clear_btn = gtk_button_new_with_label("🧹 Limpiar consola");
        g_signal_connect_swapped(clear_btn, "clicked",
                                 G_CALLBACK(gtk_text_buffer_set_text),
                                 servicios[i].buffer);

        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled), text_view);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        if (i == 2) {
            GtkWidget *ejecutar_btn = gtk_button_new_with_label("▶️ Ejecutar escaneo");
            g_signal_connect(ejecutar_btn, "clicked",
                             G_CALLBACK(ejecutar_escaneo_puertos), NULL);
            gtk_box_pack_start(GTK_BOX(vbox), ejecutar_btn, FALSE, FALSE, 5);
        }
        gtk_box_pack_start(GTK_BOX(vbox), clear_btn, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);

        GtkWidget *label = gtk_label_new(servicios[i].label);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

        iniciar_socket_servidor(&servicios[i]);
    }

    /* ------ Sección de CONFIGURACIÓN ------ */
    GtkWidget *config_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(config_box), 10);

    /* Dispositivos */
    GtkWidget *frame_disp = gtk_frame_new("⚙️ Dispositivos");
    GtkWidget *box_disp   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_disp), box_disp);

    GtkWidget *umbral_label = gtk_label_new("🔺 Umbral de alerta (%)");
    umbral_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(umbral_spin), 10);

    GtkWidget *intervalo_label = gtk_label_new("⏱️ Intervalo de escaneo (s)");
    intervalo_spin = gtk_spin_button_new_with_range(1, 3600, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(intervalo_spin), 5);

    gtk_box_pack_start(GTK_BOX(box_disp), umbral_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_disp), umbral_spin,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_disp), intervalo_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_disp), intervalo_spin,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(config_box), frame_disp, FALSE, FALSE, 10);

    /* Procesos */
    GtkWidget *frame_proc = gtk_frame_new("⚙️ Procesos");
    GtkWidget *box_proc   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_proc), box_proc);

    GtkWidget *cpu_label = gtk_label_new("🔺 Umbral CPU (%)");
    cpu_umbral_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(cpu_umbral_spin), 10);

    GtkWidget *mem_label = gtk_label_new("🔺 Umbral Memoria (%)");
    mem_umbral_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mem_umbral_spin), 10);

    GtkWidget *intervalo_proc_label = gtk_label_new("⏱️ Duración del intervalo (s)");
    intervalo_proc_spin = gtk_spin_button_new_with_range(1, 3600, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(intervalo_proc_spin), 5);

    gtk_box_pack_start(GTK_BOX(box_proc), cpu_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_proc), cpu_umbral_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_proc), mem_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_proc), mem_umbral_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_proc), intervalo_proc_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_proc), intervalo_proc_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(config_box), frame_proc, FALSE, FALSE, 10);

    /* Escaneo de Puertos */
    GtkWidget *frame_puertos = gtk_frame_new("⚙️ Escaneo de Puertos");
    GtkWidget *box_puertos   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_puertos), box_puertos);

    GtkWidget *start_port_label = gtk_label_new("🏁 Puerto inicial:");
    start_port_spin = gtk_spin_button_new_with_range(1, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(start_port_spin), 1);

    GtkWidget *end_port_label = gtk_label_new("🏳️ Puerto final:");
    end_port_spin = gtk_spin_button_new_with_range(1, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(end_port_spin), 6000);

    gtk_box_pack_start(GTK_BOX(box_puertos), start_port_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_puertos), start_port_spin,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_puertos), end_port_label,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_puertos), end_port_spin,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(config_box), frame_puertos, FALSE, FALSE, 10);

    GtkWidget *btn_aplicar = gtk_button_new_with_label("✅ Aplicar configuración");
    g_signal_connect(btn_aplicar, "clicked", G_CALLBACK(aplicar_configuracion), window);
    gtk_box_pack_start(GTK_BOX(config_box), btn_aplicar, FALSE, FALSE, 10);

    GtkWidget *label_config = gtk_label_new("⚙️ Configuración");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), config_box, label_config);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}