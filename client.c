#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 5500
#define BUFFER_SIZE 1024

GtkWidget *window;
GtkWidget *text_view;
GtkWidget *entry;
GtkWidget *send_button;

int socketDescriptor;

void send_command(GtkWidget *widget, gpointer data) {
    const gchar *command = gtk_entry_get_text(GTK_ENTRY(entry));
    send(socketDescriptor, command, strlen(command), 0);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

void *receive_output(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_received = recv(socketDescriptor, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
            printf("Server disconnected.\n");
            break;
        }
        buffer[bytes_received] = '\0';  // Add null terminator to received data
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(text_buffer, &iter);
        gtk_text_buffer_insert(text_buffer, &iter, buffer, -1);
        gtk_text_buffer_insert(text_buffer, &iter, "\n", -1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serverAddr;

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Client");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create the vertical box container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create the text view widget
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

    // Create the entry widget for typing commands
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 0);

    // Create the send button
    send_button = gtk_button_new_with_label("Send");
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_command), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, TRUE, 0);

    // Show all widgets
    gtk_widget_show_all(window);

    // Create client socket
    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(socketDescriptor, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Start receiving output from the server in a separate thread
    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_output, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Run the GTK main loop
    gtk_main();

    // Close the socket
    close(socketDescriptor);

    return 0;
}
