// Create RUDP socket
int rudp_socket();

// Send after connection establishment
int rudp_send(int socket);

// Receiving data
int rudp_recv(int socket);

// Close socket
int rudp_close(int socket);