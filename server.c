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
GtkWidget *main_box;
GtkWidget *status_label;
GtkWidget *text_view;

void *handle_client(void *arg) {
    int clientSocket = *((int *)arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_received = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
            g_print("Client disconnected.\n");
            break;
        }
        buffer[bytes_received] = '\0';  // Add null terminator to received data
        g_print("Received request: %s\n", buffer);

        // Execute the request and capture output
        FILE *fp = popen(buffer, "r");
        if (fp == NULL) {
            perror("popen");
            exit(EXIT_FAILURE);
        }

        // Send output back to client
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            ssize_t bytes_sent = send(clientSocket, buffer, strlen(buffer), 0);
            if (bytes_sent < 0) {
                perror("send");
                exit(EXIT_FAILURE);
            }
        }

        // Close the file pointer
        pclose(fp);
    }

    // Close client socket
    close(clientSocket);
    free(arg);
    return NULL;
}

void *server_thread(void *arg) {
    int serverSocket;
    struct sockaddr_in serverAddr;

    // Create server socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind server socket to the address
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    g_print("Server is running and listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        // Accept incoming connections
        int *clientSocket = malloc(sizeof(int));
        *clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (*clientSocket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Print received request to the text view
        char address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, address, INET_ADDRSTRLEN);
        char port_str[6];
        snprintf(port_str, sizeof(port_str), "%d", ntohs(clientAddr.sin_port));
        char message[100];
        snprintf(message, sizeof(message), "Received request from: %s:%s\n", address, port_str);
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_insert_at_cursor(buffer, message, -1);

        // Create a new thread to handle client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)clientSocket) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Close server socket
    close(serverSocket);

    return NULL;
}

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Server");

    // Create the main box
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Create the status label
    status_label = gtk_label_new("Server is running.");
    gtk_box_pack_start(GTK_BOX(main_box), status_label, FALSE, FALSE, 5);

    // Create the text view widget
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_box_pack_start(GTK_BOX(main_box), text_view, TRUE, TRUE, 5);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start the server thread
    pthread_t server_tid;
    if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Run the GTK main loop
    gtk_main();

    return 0;
}
