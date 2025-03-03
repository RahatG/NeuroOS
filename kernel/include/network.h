/**
 * network.h - Network interface for NeuroOS
 * 
 * This file contains the network interface definitions and declarations.
 */

#ifndef NEUROOS_NETWORK_H
#define NEUROOS_NETWORK_H

#include <stddef.h>
#include <stdint.h>

// Network interface types
#define NETWORK_INTERFACE_TYPE_UNKNOWN 0
#define NETWORK_INTERFACE_TYPE_LOOPBACK 1
#define NETWORK_INTERFACE_TYPE_ETHERNET 2
#define NETWORK_INTERFACE_TYPE_WIFI 3
#define NETWORK_INTERFACE_TYPE_CELLULAR 4
#define NETWORK_INTERFACE_TYPE_BLUETOOTH 5
#define NETWORK_INTERFACE_TYPE_VIRTUAL 6
#define NETWORK_INTERFACE_TYPE_BRIDGE 7
#define NETWORK_INTERFACE_TYPE_BOND 8
#define NETWORK_INTERFACE_TYPE_VLAN 9
#define NETWORK_INTERFACE_TYPE_TUNNEL 10
#define NETWORK_INTERFACE_TYPE_CUSTOM 11

// Network interface flags
#define NETWORK_INTERFACE_FLAG_NONE 0x00000000
#define NETWORK_INTERFACE_FLAG_UP 0x00000001
#define NETWORK_INTERFACE_FLAG_BROADCAST 0x00000002
#define NETWORK_INTERFACE_FLAG_DEBUG 0x00000004
#define NETWORK_INTERFACE_FLAG_LOOPBACK 0x00000008
#define NETWORK_INTERFACE_FLAG_POINTOPOINT 0x00000010
#define NETWORK_INTERFACE_FLAG_NOTRAILERS 0x00000020
#define NETWORK_INTERFACE_FLAG_RUNNING 0x00000040
#define NETWORK_INTERFACE_FLAG_NOARP 0x00000080
#define NETWORK_INTERFACE_FLAG_PROMISC 0x00000100
#define NETWORK_INTERFACE_FLAG_ALLMULTI 0x00000200
#define NETWORK_INTERFACE_FLAG_MASTER 0x00000400
#define NETWORK_INTERFACE_FLAG_SLAVE 0x00000800
#define NETWORK_INTERFACE_FLAG_MULTICAST 0x00001000
#define NETWORK_INTERFACE_FLAG_PORTSEL 0x00002000
#define NETWORK_INTERFACE_FLAG_AUTOMEDIA 0x00004000
#define NETWORK_INTERFACE_FLAG_DYNAMIC 0x00008000
#define NETWORK_INTERFACE_FLAG_LOWER_UP 0x00010000
#define NETWORK_INTERFACE_FLAG_DORMANT 0x00020000
#define NETWORK_INTERFACE_FLAG_ECHO 0x00040000

// Network socket flags
#define NETWORK_SOCKET_FLAG_NONE 0x00000000
#define NETWORK_SOCKET_FLAG_LISTENING 0x00000001
#define NETWORK_SOCKET_FLAG_CONNECTED 0x00000002
#define NETWORK_SOCKET_FLAG_BOUND 0x00000004
#define NETWORK_SOCKET_FLAG_BLOCKING 0x00000008
#define NETWORK_SOCKET_FLAG_NONBLOCKING 0x00000010
#define NETWORK_SOCKET_FLAG_BROADCAST 0x00000020
#define NETWORK_SOCKET_FLAG_MULTICAST 0x00000040
#define NETWORK_SOCKET_FLAG_REUSEADDR 0x00000080
#define NETWORK_SOCKET_FLAG_REUSEPORT 0x00000100
#define NETWORK_SOCKET_FLAG_KEEPALIVE 0x00000200
#define NETWORK_SOCKET_FLAG_LINGER 0x00000400
#define NETWORK_SOCKET_FLAG_OOBINLINE 0x00000800
#define NETWORK_SOCKET_FLAG_DONTROUTE 0x00001000
#define NETWORK_SOCKET_FLAG_TIMESTAMP 0x00002000
#define NETWORK_SOCKET_FLAG_CORK 0x00004000
#define NETWORK_SOCKET_FLAG_NODELAY 0x00008000
#define NETWORK_SOCKET_FLAG_QUICKACK 0x00010000
#define NETWORK_SOCKET_FLAG_FASTOPEN 0x00020000
#define NETWORK_SOCKET_FLAG_DEFER_ACCEPT 0x00040000
#define NETWORK_SOCKET_FLAG_PASSCRED 0x00080000
#define NETWORK_SOCKET_FLAG_PASSSEC 0x00100000
#define NETWORK_SOCKET_FLAG_BINDTODEVICE 0x00200000
#define NETWORK_SOCKET_FLAG_TRANSPARENT 0x00400000
#define NETWORK_SOCKET_FLAG_MARK 0x00800000
#define NETWORK_SOCKET_FLAG_TIMESTAMPNS 0x01000000
#define NETWORK_SOCKET_FLAG_TIMESTAMPING 0x02000000
#define NETWORK_SOCKET_FLAG_ACCEPTFILTER 0x04000000
#define NETWORK_SOCKET_FLAG_NOSIGPIPE 0x08000000
#define NETWORK_SOCKET_FLAG_RECVLOWAT 0x10000000
#define NETWORK_SOCKET_FLAG_RECVBUF 0x20000000
#define NETWORK_SOCKET_FLAG_SENDBUF 0x40000000
#define NETWORK_SOCKET_FLAG_PRIORITY 0x80000000

// Network socket options
#define NETWORK_SOCKET_OPT_TYPE 1
#define NETWORK_SOCKET_OPT_PROTOCOL 2
#define NETWORK_SOCKET_OPT_REUSEADDR 3
#define NETWORK_SOCKET_OPT_KEEPALIVE 4
#define NETWORK_SOCKET_OPT_BROADCAST 5
#define NETWORK_SOCKET_OPT_LINGER 6
#define NETWORK_SOCKET_OPT_SNDBUF 7
#define NETWORK_SOCKET_OPT_RCVBUF 8
#define NETWORK_SOCKET_OPT_ERROR 9
#define NETWORK_SOCKET_OPT_NODELAY 10
#define NETWORK_SOCKET_OPT_DONTROUTE 11
#define NETWORK_SOCKET_OPT_OOBINLINE 12
#define NETWORK_SOCKET_OPT_TIMESTAMP 13
#define NETWORK_SOCKET_OPT_PRIORITY 14
#define NETWORK_SOCKET_OPT_BINDTODEVICE 15

// Network interface states
#define NETWORK_INTERFACE_STATE_UNKNOWN 0
#define NETWORK_INTERFACE_STATE_DOWN 1
#define NETWORK_INTERFACE_STATE_UP 2
#define NETWORK_INTERFACE_STATE_TESTING 3
#define NETWORK_INTERFACE_STATE_DORMANT 4
#define NETWORK_INTERFACE_STATE_NOTPRESENT 5
#define NETWORK_INTERFACE_STATE_LOWERLAYERDOWN 6

// Network protocol types
#define NETWORK_PROTOCOL_TYPE_UNKNOWN 0
#define NETWORK_PROTOCOL_TYPE_IP 1
#define NETWORK_PROTOCOL_TYPE_IPV4 2
#define NETWORK_PROTOCOL_TYPE_IPV6 3
#define NETWORK_PROTOCOL_TYPE_TCP 4
#define NETWORK_PROTOCOL_TYPE_UDP 5
#define NETWORK_PROTOCOL_TYPE_ICMP 6
#define NETWORK_PROTOCOL_TYPE_ICMPV6 7
#define NETWORK_PROTOCOL_TYPE_IGMP 8
#define NETWORK_PROTOCOL_TYPE_ARP 9
#define NETWORK_PROTOCOL_TYPE_RARP 10
#define NETWORK_PROTOCOL_TYPE_DHCP 11
#define NETWORK_PROTOCOL_TYPE_DNS 12
#define NETWORK_PROTOCOL_TYPE_HTTP 13
#define NETWORK_PROTOCOL_TYPE_HTTPS 14
#define NETWORK_PROTOCOL_TYPE_FTP 15
#define NETWORK_PROTOCOL_TYPE_SMTP 16
#define NETWORK_PROTOCOL_TYPE_POP3 17
#define NETWORK_PROTOCOL_TYPE_IMAP 18
#define NETWORK_PROTOCOL_TYPE_SSH 19
#define NETWORK_PROTOCOL_TYPE_TELNET 20
#define NETWORK_PROTOCOL_TYPE_NTP 21
#define NETWORK_PROTOCOL_TYPE_SNMP 22
#define NETWORK_PROTOCOL_TYPE_LDAP 23
#define NETWORK_PROTOCOL_TYPE_TFTP 24
#define NETWORK_PROTOCOL_TYPE_CUSTOM 25

// Network error codes
#define NETWORK_ERROR_NONE 0
#define NETWORK_ERROR_INVALID_ARGUMENT -1
#define NETWORK_ERROR_OUT_OF_MEMORY -2
#define NETWORK_ERROR_PERMISSION_DENIED -3
#define NETWORK_ERROR_NOT_FOUND -4
#define NETWORK_ERROR_ALREADY_EXISTS -5
#define NETWORK_ERROR_NOT_SUPPORTED -6
#define NETWORK_ERROR_TIMEOUT -7
#define NETWORK_ERROR_BUSY -8
#define NETWORK_ERROR_INTERRUPTED -9
#define NETWORK_ERROR_IO -10
#define NETWORK_ERROR_INVALID_STATE -11
#define NETWORK_ERROR_INVALID_TYPE -12
#define NETWORK_ERROR_INVALID_FLAGS -13
#define NETWORK_ERROR_INVALID_CONFIG -14
#define NETWORK_ERROR_INVALID_FORMAT -15
#define NETWORK_ERROR_INVALID_ADDRESS -16
#define NETWORK_ERROR_INVALID_PORT -17
#define NETWORK_ERROR_INVALID_PROTOCOL -18
#define NETWORK_ERROR_INVALID_SOCKET -19
#define NETWORK_ERROR_INVALID_INTERFACE -20
#define NETWORK_ERROR_INVALID_ROUTE -21
#define NETWORK_ERROR_INVALID_PACKET -22
#define NETWORK_ERROR_INVALID_FRAME -23
#define NETWORK_ERROR_INVALID_HEADER -24
#define NETWORK_ERROR_INVALID_PAYLOAD -25
#define NETWORK_ERROR_INVALID_CHECKSUM -26
#define NETWORK_ERROR_UNKNOWN -27

// Network address structure
typedef struct {
    uint8_t type;
    uint8_t family;
    uint8_t prefix_length;
    uint8_t scope;
    union {
        uint8_t bytes[16];
        uint16_t words[8];
        uint32_t dwords[4];
        uint64_t qwords[2];
        struct {
            uint8_t b1;
            uint8_t b2;
            uint8_t b3;
            uint8_t b4;
        } ipv4;
        struct {
            uint16_t w1;
            uint16_t w2;
            uint16_t w3;
            uint16_t w4;
            uint16_t w5;
            uint16_t w6;
            uint16_t w7;
            uint16_t w8;
        } ipv6;
        struct {
            uint8_t b1;
            uint8_t b2;
            uint8_t b3;
            uint8_t b4;
            uint8_t b5;
            uint8_t b6;
        } mac;
    } address;
} network_address_t;

// Network statistics structure
typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_errors;
    uint64_t tx_errors;
    uint64_t rx_dropped;
    uint64_t tx_dropped;
    uint64_t rx_fifo_errors;
    uint64_t tx_fifo_errors;
    uint64_t rx_frame_errors;
    uint64_t tx_carrier_errors;
    uint64_t rx_compressed;
    uint64_t tx_compressed;
    uint64_t collisions;
    uint64_t multicast;
} network_stats_t;

// Network socket statistics structure
typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_errors;
    uint64_t tx_errors;
    uint64_t rx_dropped;
    uint64_t tx_dropped;
    uint64_t connections;
    uint64_t disconnections;
    uint64_t timeouts;
    uint64_t retransmissions;
} network_socket_stats_t;

// Network interface information structure
typedef struct {
    uint32_t id;
    char name[32];
    uint8_t mac_address[6];
    network_address_t ip_address;
    network_address_t subnet_mask;
    network_address_t gateway;
    uint32_t flags;
    uint32_t mtu;
} network_interface_info_t;

// Network socket information structure
typedef struct {
    uint32_t id;
    uint32_t type;
    uint32_t protocol;
    uint32_t flags;
    network_address_t local_address;
    uint16_t local_port;
    network_address_t remote_address;
    uint16_t remote_port;
} network_socket_info_t;

// Network socket structure
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
} network_socket_t;

// Network initialization and shutdown
int network_init(void);
int network_shutdown(void);

// Network interface operations
int network_register_interface(const char* name, const uint8_t* mac_address, void* driver_data, uint32_t* id);
int network_unregister_interface(uint32_t id);
int network_configure_interface(uint32_t id, const network_address_t* ip_address, const network_address_t* subnet_mask, const network_address_t* gateway);
int network_get_interface_info(uint32_t id, network_interface_info_t* info);

// Network socket operations
int network_socket_create(uint32_t type, uint32_t protocol, uint32_t* id);
int network_socket_close(uint32_t id);
int network_socket_bind(uint32_t id, const network_address_t* address, uint16_t port);
int network_socket_connect(uint32_t id, const network_address_t* address, uint16_t port);
int network_socket_listen(uint32_t id, int backlog);
int network_socket_accept(uint32_t id, uint32_t* client_id, network_address_t* client_address, uint16_t* client_port);
int network_socket_send(uint32_t id, const void* data, size_t size, size_t* sent);
int network_socket_recv(network_socket_t* socket, void* data, size_t size, size_t* received);
int network_socket_sendto(uint32_t id, const void* data, size_t size, const network_address_t* address, uint16_t port, size_t* sent);
int network_socket_recvfrom(network_socket_t* socket, void* data, size_t size, network_address_t* address, uint16_t* port, size_t* received);
int network_socket_setsockopt(uint32_t id, int option, const void* value, size_t size);
int network_socket_getsockopt(uint32_t id, int option, void* value, size_t* size);
int network_socket_getinfo(uint32_t id, network_socket_info_t* info);

// These functions are implemented as static in network.c and not exposed in the API
// Removed static function declarations to avoid "declared static but never defined" warnings

#endif // NEUROOS_NETWORK_H
