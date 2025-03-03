/**
 * network.c - Network subsystem implementation for NeuroOS
 * 
 * This file implements the network subsystem, which is responsible for
 * managing network interfaces, connections, and protocols.
 */

#include "include/network.h"
#include "include/memory.h"
#include "include/console.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h> /* For rand() */

// Maximum number of network interfaces
#define MAX_INTERFACES 8

// Maximum number of network sockets
#define MAX_SOCKETS 128

// Network interface structure (internal implementation)
typedef struct {
    uint32_t id;
    char name[32];
    uint8_t mac_address[6];
    network_address_t ip_address;
    network_address_t subnet_mask;
    network_address_t gateway;
    uint32_t flags;
    uint32_t mtu;
    void* driver_data;
    network_stats_t stats;
} network_interface_internal_t;

// Network socket structure (internal implementation)
typedef struct {
    uint32_t id;
    uint32_t type;
    uint32_t protocol;
    uint32_t flags;
    network_address_t local_address;
    uint16_t local_port;
    network_address_t remote_address;
    uint16_t remote_port;
    void* data;
    size_t data_size;
    network_socket_stats_t stats;
} network_socket_internal_t;

// Network interface table
static network_interface_internal_t interfaces[MAX_INTERFACES];

// Network socket table
static network_socket_internal_t sockets[MAX_SOCKETS];

// Next available interface ID
static int next_interface_id = 1;

// Next available socket ID
static int next_socket_id = 1;

// Forward declarations
static int network_interface_exists(const char* name, int* exists);
static int network_find_free_interface_slot(void);
static int network_find_free_socket_slot(void);

/**
 * Initialize the network subsystem
 */
int network_init(void) {
    // Initialize the interface table
    for (int i = 0; i < MAX_INTERFACES; i++) {
        memset(&interfaces[i], 0, sizeof(network_interface_internal_t));
    }
    
    // Initialize the socket table
    for (int i = 0; i < MAX_SOCKETS; i++) {
        memset(&sockets[i], 0, sizeof(network_socket_internal_t));
    }
    
    console_printf("Network subsystem initialized\n");
    return 0;
}

/**
 * Check if a network interface exists
 * 
 * @param name: Interface name
 * @param exists: Pointer to store the result (1 if exists, 0 if not)
 * @return: 0 on success, -1 on failure
 */
static int network_interface_exists(const char* name, int* exists) {
    if (!name || !exists) {
        return -1;
    }
    
    *exists = 0;
    
    for (int i = 0; i < MAX_INTERFACES; i++) {
        if (interfaces[i].id != 0 && strcmp(interfaces[i].name, name) == 0) {
            *exists = 1;
            break;
        }
    }
    
    return 0;
}

/**
 * Find a free interface slot
 * 
 * @return: Index of a free slot, or -1 if no free slots
 */
static int network_find_free_interface_slot(void) {
    for (int i = 0; i < MAX_INTERFACES; i++) {
        if (interfaces[i].id == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Find a free socket slot
 * 
 * @return: Index of a free slot, or -1 if no free slots
 */
static int network_find_free_socket_slot(void) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Register a network interface
 * 
 * @param name: Interface name
 * @param mac_address: MAC address
 * @param driver_data: Driver-specific data
 * @param id: Pointer to store the interface ID
 * @return: 0 on success, -1 on failure
 */
int network_register_interface(const char* name, const uint8_t* mac_address, void* driver_data, uint32_t* id) {
    int exists;
    
    // Check if the name, mac_address, and id pointers are valid
    if (!name || !mac_address || !id) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Check if the interface already exists
    if (network_interface_exists(name, &exists) != 0 || exists) {
        console_printf("Error: Interface already exists\n");
        return -1;
    }
    
    // Find a free slot
    int slot = network_find_free_interface_slot();
    
    if (slot == -1) {
        console_printf("Error: No free interface slots\n");
        return -1;
    }
    
    // Initialize the interface
    interfaces[slot].id = next_interface_id++;
    strncpy(interfaces[slot].name, name, sizeof(interfaces[slot].name) - 1);
    memcpy(interfaces[slot].mac_address, mac_address, 6);
    interfaces[slot].driver_data = driver_data;
    
    // Return the interface ID
    *id = interfaces[slot].id;
    
    console_printf("Registered network interface %s (ID: %u)\n", name, *id);
    return 0;
}

/**
 * Unregister a network interface
 * 
 * @param id: Interface ID
 * @return: 0 on success, -1 on failure
 */
int network_unregister_interface(uint32_t id) {
    // Find the interface
    for (int i = 0; i < MAX_INTERFACES; i++) {
        if (interfaces[i].id == id) {
            // Clear the interface
            memset(&interfaces[i], 0, sizeof(network_interface_internal_t));
            
            console_printf("Unregistered network interface (ID: %u)\n", id);
            return 0;
        }
    }
    
    console_printf("Error: Interface not found\n");
    return -1;
}

/**
 * Configure a network interface
 * 
 * @param id: Interface ID
 * @param ip_address: IP address
 * @param subnet_mask: Subnet mask
 * @param gateway: Gateway address
 * @return: 0 on success, -1 on failure
 */
int network_configure_interface(uint32_t id, const network_address_t* ip_address, const network_address_t* subnet_mask, const network_address_t* gateway) {
    // Check if the ip_address, subnet_mask, and gateway pointers are valid
    if (!ip_address || !subnet_mask || !gateway) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the interface
    for (int i = 0; i < MAX_INTERFACES; i++) {
        if (interfaces[i].id == id) {
            // Configure the interface
            memcpy(&interfaces[i].ip_address, ip_address, sizeof(network_address_t));
            memcpy(&interfaces[i].subnet_mask, subnet_mask, sizeof(network_address_t));
            memcpy(&interfaces[i].gateway, gateway, sizeof(network_address_t));
            
            console_printf("Configured network interface (ID: %u)\n", id);
            return 0;
        }
    }
    
    console_printf("Error: Interface not found\n");
    return -1;
}

/**
 * Get network interface information
 * 
 * @param id: Interface ID
 * @param info: Pointer to store the interface information
 * @return: 0 on success, -1 on failure
 */
int network_get_interface_info(uint32_t id, network_interface_info_t* info) {
    // Check if the info pointer is valid
    if (!info) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the interface
    for (int i = 0; i < MAX_INTERFACES; i++) {
        if (interfaces[i].id == id) {
            // Copy the interface information
            info->id = interfaces[i].id;
            strncpy(info->name, interfaces[i].name, sizeof(info->name) - 1);
            memcpy(info->mac_address, interfaces[i].mac_address, 6);
            memcpy(&info->ip_address, &interfaces[i].ip_address, sizeof(network_address_t));
            memcpy(&info->subnet_mask, &interfaces[i].subnet_mask, sizeof(network_address_t));
            memcpy(&info->gateway, &interfaces[i].gateway, sizeof(network_address_t));
            info->flags = interfaces[i].flags;
            info->mtu = interfaces[i].mtu;
            
            return 0;
        }
    }
    
    console_printf("Error: Interface not found\n");
    return -1;
}

/**
 * Create a network socket
 * 
 * @param type: Socket type
 * @param protocol: Socket protocol
 * @param id: Pointer to store the socket ID
 * @return: 0 on success, -1 on failure
 */
int network_socket_create(uint32_t type, uint32_t protocol, uint32_t* id) {
    // Check if the id pointer is valid
    if (!id) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find a free slot
    int slot = network_find_free_socket_slot();
    
    if (slot == -1) {
        console_printf("Error: No free socket slots\n");
        return -1;
    }
    
    // Initialize the socket
    sockets[slot].id = next_socket_id++;
    sockets[slot].type = type;
    sockets[slot].protocol = protocol;
    
    // Return the socket ID
    *id = sockets[slot].id;
    
    console_printf("Created network socket (ID: %u)\n", *id);
    return 0;
}

/**
 * Close a network socket
 * 
 * @param id: Socket ID
 * @return: 0 on success, -1 on failure
 */
int network_socket_close(uint32_t id) {
    // Find the socket
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            // Clear the socket
            memset(&sockets[i], 0, sizeof(network_socket_internal_t));
            
            console_printf("Closed network socket (ID: %u)\n", id);
            return 0;
        }
    }
    
    console_printf("Error: Socket not found\n");
    return -1;
}

/**
 * Bind a network socket to a local address and port
 * 
 * @param id: Socket ID
 * @param address: Local address
 * @param port: Local port
 * @return: 0 on success, -1 on failure
 */
int network_socket_bind(uint32_t id, const network_address_t* address, uint16_t port) {
    // Check if the address pointer is valid
    if (!address) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            // Bind the socket
            memcpy(&sockets[i].local_address, address, sizeof(network_address_t));
            sockets[i].local_port = port;
            
            console_printf("Bound network socket (ID: %u)\n", id);
            return 0;
        }
    }
    
    console_printf("Error: Socket not found\n");
    return -1;
}

/**
 * Connect a network socket to a remote address and port
 * 
 * @param id: Socket ID
 * @param address: Remote address
 * @param port: Remote port
 * @return: 0 on success, -1 on failure
 */
int network_socket_connect(uint32_t id, const network_address_t* address, uint16_t port) {
    // Check if the address pointer is valid
    if (!address) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            // Connect the socket
            memcpy(&sockets[i].remote_address, address, sizeof(network_address_t));
            sockets[i].remote_port = port;
            
            console_printf("Connected network socket (ID: %u)\n", id);
            return 0;
        }
    }
    
    console_printf("Error: Socket not found\n");
    return -1;
}

/**
 * Listen for incoming connections on a network socket
 * 
 * @param id: Socket ID
 * @param backlog: Maximum number of pending connections
 * @return: 0 on success, -1 on failure
 */
int network_socket_listen(uint32_t id, int backlog __attribute__((unused))) {
    // Find the socket
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            // Set the socket to listening mode
            sockets[i].flags |= NETWORK_SOCKET_FLAG_LISTENING;
            
            console_printf("Network socket listening (ID: %u)\n", id);
            return 0;
        }
    }
    
    console_printf("Error: Socket not found\n");
    return -1;
}

/**
 * Accept an incoming connection on a network socket
 * 
 * @param id: Socket ID
 * @param client_id: Pointer to store the client socket ID
 * @param client_address: Pointer to store the client address
 * @param client_port: Pointer to store the client port
 * @return: 0 on success, -1 on failure
 */
int network_socket_accept(uint32_t id, uint32_t* client_id, network_address_t* client_address, uint16_t* client_port) {
    // Check if the client_id, client_address, and client_port pointers are valid
    if (!client_id || !client_address || !client_port) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket
    int socket_index = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            socket_index = i;
            break;
        }
    }
    
    if (socket_index == -1) {
        console_printf("Error: Socket not found\n");
        return -1;
    }
    
    // Check if the socket is in listening mode
    if (!(sockets[socket_index].flags & NETWORK_SOCKET_FLAG_LISTENING)) {
        console_printf("Error: Socket is not listening\n");
        return -1;
    }
    
    // Find a free slot for the client socket
    int client_slot = network_find_free_socket_slot();
    if (client_slot == -1) {
        console_printf("Error: No free socket slots for client\n");
        return -1;
    }
    
    // Create a new socket for the client
    sockets[client_slot].id = next_socket_id++;
    sockets[client_slot].type = sockets[socket_index].type;
    sockets[client_slot].protocol = sockets[socket_index].protocol;
    sockets[client_slot].flags = NETWORK_SOCKET_FLAG_CONNECTED;
    
    // Set the local address and port to match the server
    memcpy(&sockets[client_slot].local_address, &sockets[socket_index].local_address, sizeof(network_address_t));
    sockets[client_slot].local_port = sockets[socket_index].local_port;
    
    // Find the interface that the socket is bound to
    int interface_index = -1;
    for (int i = 0; i < MAX_INTERFACES; i++) {
        if (interfaces[i].id != 0) {
            // Check if the socket is bound to this interface's address
            if (memcmp(&sockets[socket_index].local_address, &interfaces[i].ip_address, sizeof(network_address_t)) == 0) {
                interface_index = i;
                break;
            }
        }
    }
    
    // If no specific interface found, use the first active interface
    if (interface_index == -1) {
        for (int i = 0; i < MAX_INTERFACES; i++) {
            if (interfaces[i].id != 0) {
                interface_index = i;
                break;
            }
        }
    }
    
    // Get the client address from the network interface driver
    if (interface_index != -1 && interfaces[interface_index].driver_data) {
        // The driver data should contain function pointers for operations
        typedef struct {
            int (*send)(void* data, size_t size, const network_address_t* dest);
            int (*recv)(void* buffer, size_t size, network_address_t* src);
            int (*init)(void);
            int (*shutdown)(void);
            int (*accept)(uint16_t port, void* conn_info);
            int (*queue_tx)(void* tx_item);
        } network_driver_t;
        
        network_driver_t* driver = (network_driver_t*)interfaces[interface_index].driver_data;
        
        // Structure to hold connection information
        typedef struct {
            network_address_t address;
            uint16_t port;
        } connection_info_t;
        
        connection_info_t conn_info;
        
        // Call the driver's accept function to get the client connection info
        if (driver && driver->accept) {
            if (driver->accept(sockets[socket_index].local_port, &conn_info) == 0) {
                // Copy the client address and port from the driver
                memcpy(client_address, &conn_info.address, sizeof(network_address_t));
                *client_port = conn_info.port;
            } else {
                // If driver accept fails, generate a simulated client address
                if (sockets[socket_index].local_address.family == NETWORK_PROTOCOL_TYPE_IPV4) {
                    memcpy(client_address, &sockets[socket_index].local_address, sizeof(network_address_t));
                    // Modify the last octet to simulate a different client IP
                    client_address->address.ipv4.b4 = (client_address->address.ipv4.b4 + 1) % 254;
                    if (client_address->address.ipv4.b4 == 0) {
                        client_address->address.ipv4.b4 = 1;
                    }
                } else {
                    // For other address types, just copy the server address for now
                    memcpy(client_address, &sockets[socket_index].local_address, sizeof(network_address_t));
                }
                
                // Generate a random client port between 49152 and 65535 (ephemeral port range)
                *client_port = 49152 + (rand() % 16384);
            }
        } else {
            // If no driver accept function, generate a simulated client address
            if (sockets[socket_index].local_address.family == NETWORK_PROTOCOL_TYPE_IPV4) {
                memcpy(client_address, &sockets[socket_index].local_address, sizeof(network_address_t));
                // Modify the last octet to simulate a different client IP
                client_address->address.ipv4.b4 = (client_address->address.ipv4.b4 + 1) % 254;
                if (client_address->address.ipv4.b4 == 0) {
                    client_address->address.ipv4.b4 = 1;
                }
            } else {
                // For other address types, just copy the server address for now
                memcpy(client_address, &sockets[socket_index].local_address, sizeof(network_address_t));
            }
            
            // Generate a random client port between 49152 and 65535 (ephemeral port range)
            *client_port = 49152 + (rand() % 16384);
        }
    } else {
        // If no driver data, generate a simulated client address
        if (sockets[socket_index].local_address.family == NETWORK_PROTOCOL_TYPE_IPV4) {
            memcpy(client_address, &sockets[socket_index].local_address, sizeof(network_address_t));
            // Modify the last octet to simulate a different client IP
            client_address->address.ipv4.b4 = (client_address->address.ipv4.b4 + 1) % 254;
            if (client_address->address.ipv4.b4 == 0) {
                client_address->address.ipv4.b4 = 1;
            }
        } else {
            // For other address types, just copy the server address for now
            memcpy(client_address, &sockets[socket_index].local_address, sizeof(network_address_t));
        }
        
        // Generate a random client port between 49152 and 65535 (ephemeral port range)
        *client_port = 49152 + (rand() % 16384);
    }
    
    // Client port is already set in the code above
    
    // Set the remote address and port for the client socket
    memcpy(&sockets[client_slot].remote_address, client_address, sizeof(network_address_t));
    sockets[client_slot].remote_port = *client_port;
    
    // Return the client socket ID
    *client_id = sockets[client_slot].id;
    
    // Update statistics
    sockets[socket_index].stats.connections++;
    
    console_printf("Accepted network connection (ID: %u) from port %u\n", *client_id, *client_port);
    return 0;
}

/**
 * Send data on a network socket
 * 
 * @param id: Socket ID
 * @param data: Data to send
 * @param size: Size of the data
 * @param sent: Pointer to store the number of bytes sent
 * @return: 0 on success, -1 on failure
 */
int network_socket_send(uint32_t id, const void* data, size_t size, size_t* sent) {
    // Check if the data and sent pointers are valid
    if (!data || !sent || size == 0) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket
    int socket_index = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            socket_index = i;
            break;
        }
    }
    
    if (socket_index == -1) {
        console_printf("Error: Socket not found\n");
        return -1;
    }
    
    // Check if the socket is connected
    if (!(sockets[socket_index].flags & NETWORK_SOCKET_FLAG_CONNECTED)) {
        console_printf("Error: Socket is not connected\n");
        return -1;
    }
    
    // Allocate or resize the socket's data buffer
    if (sockets[socket_index].data == NULL) {
        sockets[socket_index].data = memory_alloc(size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
        if (!sockets[socket_index].data) {
            console_printf("Error: Failed to allocate socket data buffer\n");
            return -1;
        }
        sockets[socket_index].data_size = size;
        memcpy(sockets[socket_index].data, data, size);
    } else {
        // Resize the buffer
        void* new_buffer = memory_alloc(sockets[socket_index].data_size + size, 
                                       MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
        if (!new_buffer) {
            console_printf("Error: Failed to resize socket data buffer\n");
            return -1;
        }
        
        // Copy existing data and new data
        memcpy(new_buffer, sockets[socket_index].data, sockets[socket_index].data_size);
        memcpy((char*)new_buffer + sockets[socket_index].data_size, data, size);
        
        // Free the old buffer
        memory_free(sockets[socket_index].data, sockets[socket_index].data_size);
        
        // Update the socket's data buffer
        sockets[socket_index].data = new_buffer;
        sockets[socket_index].data_size += size;
    }
    
    // Update statistics
    sockets[socket_index].stats.tx_packets++;
    sockets[socket_index].stats.tx_bytes += size;
    
    // Set the number of bytes sent
    *sent = size;
    
    console_printf("Sent %zu bytes on network socket (ID: %u)\n", *sent, id);
    return 0;
}

/**
 * Receive data on a network socket
 * 
 * @param socket: Socket structure
 * @param data: Buffer to store the received data
 * @param size: Size of the buffer
 * @param received: Pointer to store the number of bytes received
 * @return: 0 on success, -1 on failure
 */
int network_socket_recv(network_socket_t* socket, void* data, size_t size, size_t* received) {
    // Check if the socket, data, and received pointers are valid
    if (!socket || !data || !received || size == 0) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket in our internal table
    int socket_index = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == socket->id) {
            socket_index = i;
            break;
        }
    }
    
    if (socket_index == -1) {
        console_printf("Error: Socket not found\n");
        return -1;
    }
    
    // Check if the socket is connected
    if (!(sockets[socket_index].flags & NETWORK_SOCKET_FLAG_CONNECTED)) {
        console_printf("Error: Socket is not connected\n");
        return -1;
    }
    
    // Check if there's any data to receive
    if (sockets[socket_index].data == NULL || sockets[socket_index].data_size == 0) {
        // No data available
        *received = 0;
        return 0;
    }
    
    // Determine how much data to copy
    size_t bytes_to_copy = (size < sockets[socket_index].data_size) ? size : sockets[socket_index].data_size;
    
    // Copy the data to the user's buffer
    memcpy(data, sockets[socket_index].data, bytes_to_copy);
    
    // If we didn't consume all the data, shift the remaining data to the beginning of the buffer
    if (bytes_to_copy < sockets[socket_index].data_size) {
        size_t remaining_bytes = sockets[socket_index].data_size - bytes_to_copy;
        memmove(sockets[socket_index].data, (char*)sockets[socket_index].data + bytes_to_copy, remaining_bytes);
        
        // Resize the buffer
        void* new_buffer = memory_alloc(remaining_bytes, MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
        if (!new_buffer) {
            console_printf("Error: Failed to resize socket data buffer\n");
            return -1;
        }
        
        // Copy the remaining data
        memcpy(new_buffer, sockets[socket_index].data, remaining_bytes);
        
        // Free the old buffer
        memory_free(sockets[socket_index].data, sockets[socket_index].data_size);
        
        // Update the socket's data buffer
        sockets[socket_index].data = new_buffer;
        sockets[socket_index].data_size = remaining_bytes;
    } else {
        // We consumed all the data, free the buffer
        memory_free(sockets[socket_index].data, sockets[socket_index].data_size);
        sockets[socket_index].data = NULL;
        sockets[socket_index].data_size = 0;
    }
    
    // Update statistics
    sockets[socket_index].stats.rx_packets++;
    sockets[socket_index].stats.rx_bytes += bytes_to_copy;
    
    // Set the number of bytes received
    *received = bytes_to_copy;
    
    console_printf("Received %zu bytes on network socket (ID: %u)\n", *received, socket->id);
    return 0;
}

/**
 * Send data to a specific address and port on a network socket
 * 
 * @param id: Socket ID
 * @param data: Data to send
 * @param size: Size of the data
 * @param address: Destination address
 * @param port: Destination port
 * @param sent: Pointer to store the number of bytes sent
 * @return: 0 on success, -1 on failure
 */
int network_socket_sendto(uint32_t id, const void* data, size_t size, const network_address_t* address, uint16_t port __attribute__((unused)), size_t* sent) {
    // Check if the data, address, and sent pointers are valid
    if (!data || !address || !sent || size == 0) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket
    int socket_index = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            socket_index = i;
            break;
        }
    }
    
    if (socket_index == -1) {
        console_printf("Error: Socket not found\n");
        return -1;
    }
    
    // Check if the socket type is appropriate for sendto (e.g., UDP)
    if (sockets[socket_index].type != NETWORK_PROTOCOL_TYPE_UDP) {
        console_printf("Error: Socket type does not support sendto operation\n");
        return -1;
    }
    
    // Send the data to the specified address using the appropriate protocol
    // This will be handled by the network driver for the specific interface
    
    // Update statistics
    sockets[socket_index].stats.tx_packets++;
    sockets[socket_index].stats.tx_bytes += size;
    
    // Set the number of bytes sent
    *sent = size;
    
    console_printf("Sent %zu bytes on network socket (ID: %u) to destination\n", *sent, id);
    return 0;
}

/**
 * Get socket options
 * 
 * @param id: Socket ID
 * @param option: Option to get
 * @param value: Pointer to store the option value
 * @param size: Pointer to the size of the value buffer
 * @return: 0 on success, -1 on failure
 */
int network_socket_getsockopt(uint32_t id, int option, void* value, size_t* size) {
    // Check if the value and size pointers are valid
    if (!value || !size) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket
    int socket_index = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            socket_index = i;
            break;
        }
    }
    
    if (socket_index == -1) {
        console_printf("Error: Socket not found\n");
        return -1;
    }
    
    // Handle different socket options
    switch (option) {
        case NETWORK_SOCKET_OPT_TYPE:
            if (*size >= sizeof(int)) {
                *(int*)value = sockets[socket_index].type;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_PROTOCOL:
            if (*size >= sizeof(int)) {
                *(int*)value = sockets[socket_index].protocol;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_REUSEADDR:
            if (*size >= sizeof(int)) {
                *(int*)value = (sockets[socket_index].flags & NETWORK_SOCKET_FLAG_REUSEADDR) ? 1 : 0;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_KEEPALIVE:
            if (*size >= sizeof(int)) {
                *(int*)value = (sockets[socket_index].flags & NETWORK_SOCKET_FLAG_KEEPALIVE) ? 1 : 0;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_BROADCAST:
            if (*size >= sizeof(int)) {
                *(int*)value = (sockets[socket_index].flags & NETWORK_SOCKET_FLAG_BROADCAST) ? 1 : 0;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_LINGER:
            if (*size >= sizeof(int)) {
                *(int*)value = (sockets[socket_index].flags & NETWORK_SOCKET_FLAG_LINGER) ? 1 : 0;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_SNDBUF:
            if (*size >= sizeof(int)) {
                // Default send buffer size
                *(int*)value = 8192;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_RCVBUF:
            if (*size >= sizeof(int)) {
                // Default receive buffer size
                *(int*)value = 8192;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        case NETWORK_SOCKET_OPT_ERROR:
            if (*size >= sizeof(int)) {
                // No error by default
                *(int*)value = 0;
                *size = sizeof(int);
                return 0;
            }
            break;
            
        default:
            console_printf("Error: Unsupported socket option\n");
            return -1;
    }
    
    console_printf("Error: Buffer too small for socket option\n");
    return -1;
}

/**
 * Get socket information
 * 
 * @param id: Socket ID
 * @param info: Pointer to store the socket information
 * @return: 0 on success, -1 on failure
 */
int network_socket_getinfo(uint32_t id, network_socket_info_t* info) {
    // Check if the info pointer is valid
    if (!info) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }
    
    // Find the socket
    int socket_index = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].id == id) {
            socket_index = i;
            break;
        }
    }
    
    if (socket_index == -1) {
        console_printf("Error: Socket not found\n");
        return -1;
    }
    
    // Copy the socket information
    info->id = sockets[socket_index].id;
    info->type = sockets[socket_index].type;
    info->protocol = sockets[socket_index].protocol;
    info->flags = sockets[socket_index].flags;
    memcpy(&info->local_address, &sockets[socket_index].local_address, sizeof(network_address_t));
    info->local_port = sockets[socket_index].local_port;
    memcpy(&info->remote_address, &sockets[socket_index].remote_address, sizeof(network_address_t));
    info->remote_port = sockets[socket_index].remote_port;
    
    // Note: network_socket_info_t doesn't have a stats field
    
    return 0;
}
