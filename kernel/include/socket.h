/**
 * socket.h - Socket interface for NeuroOS
 * 
 * This file contains the socket interface definitions and declarations.
 */

#ifndef NEUROOS_SOCKET_H
#define NEUROOS_SOCKET_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "network.h"

// Define ssize_t if not already defined
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

// Maximum hostname length
#define NETWORK_HOSTNAME_MAX 256

// Maximum interface name length
#define NETWORK_INTERFACE_NAME_MAX 32

// Socket domain types
typedef enum {
    AF_UNSPEC = 0,
    AF_INET = 1,
    AF_INET6 = 2,
    AF_UNIX = 3,
    AF_LOCAL = AF_UNIX,
    AF_PACKET = 4,
    AF_NETLINK = 5,
    AF_BLUETOOTH = 6,
    AF_CAN = 7,
    AF_CUSTOM = 8
} socket_domain_t;

// Socket type
typedef enum {
    SOCKET_TYPE_STREAM = 1,
    SOCKET_TYPE_DGRAM = 2,
    SOCKET_TYPE_RAW = 3,
    SOCKET_TYPE_SEQPACKET = 4,
    SOCKET_TYPE_RDM = 5,
    SOCKET_TYPE_CUSTOM = 6
} socket_type_t;

// Socket protocol
typedef enum {
    NETWORK_PROTOCOL_IP = 0,
    NETWORK_PROTOCOL_ICMP = 1,
    NETWORK_PROTOCOL_TCP = 6,
    NETWORK_PROTOCOL_UDP = 17,
    NETWORK_PROTOCOL_IPV6 = 41,
    NETWORK_PROTOCOL_ICMPV6 = 58,
    NETWORK_PROTOCOL_SCTP = 132,
    NETWORK_PROTOCOL_HTTP = 200,
    NETWORK_PROTOCOL_HTTPS = 201,
    NETWORK_PROTOCOL_FTP = 202,
    NETWORK_PROTOCOL_SMTP = 203,
    NETWORK_PROTOCOL_POP3 = 204,
    NETWORK_PROTOCOL_IMAP = 205,
    NETWORK_PROTOCOL_DNS = 206,
    NETWORK_PROTOCOL_CUSTOM = 255
} network_protocol_t;

// Socket flags
typedef enum {
    SOCKET_FLAG_NONE = 0,
    SOCKET_FLAG_NONBLOCK = (1 << 0),
    SOCKET_FLAG_CLOEXEC = (1 << 1),
    SOCKET_FLAG_TIMESTAMP = (1 << 2),
    SOCKET_FLAG_PASSCRED = (1 << 3),
    SOCKET_FLAG_REUSEADDR = (1 << 4),
    SOCKET_FLAG_REUSEPORT = (1 << 5),
    SOCKET_FLAG_BROADCAST = (1 << 6),
    SOCKET_FLAG_KEEPALIVE = (1 << 7),
    SOCKET_FLAG_LINGER = (1 << 8),
    SOCKET_FLAG_OOBINLINE = (1 << 9),
    SOCKET_FLAG_SNDBUF = (1 << 10),
    SOCKET_FLAG_RCVBUF = (1 << 11),
    SOCKET_FLAG_DONTROUTE = (1 << 12),
    SOCKET_FLAG_PRIORITY = (1 << 13),
    SOCKET_FLAG_RCVLOWAT = (1 << 14),
    SOCKET_FLAG_SNDLOWAT = (1 << 15),
    SOCKET_FLAG_RCVTIMEO = (1 << 16),
    SOCKET_FLAG_SNDTIMEO = (1 << 17),
    SOCKET_FLAG_CUSTOM = (1 << 18)
} socket_flags_t;

// Socket state
typedef enum {
    SOCKET_STATE_CLOSED = 0,
    SOCKET_STATE_LISTEN = 1,
    SOCKET_STATE_SYN_SENT = 2,
    SOCKET_STATE_SYN_RECEIVED = 3,
    SOCKET_STATE_ESTABLISHED = 4,
    SOCKET_STATE_FIN_WAIT_1 = 5,
    SOCKET_STATE_FIN_WAIT_2 = 6,
    SOCKET_STATE_CLOSE_WAIT = 7,
    SOCKET_STATE_CLOSING = 8,
    SOCKET_STATE_LAST_ACK = 9,
    SOCKET_STATE_TIME_WAIT = 10,
    SOCKET_STATE_CUSTOM = 11
} socket_state_t;

// Socket option levels
#define SOL_SOCKET 1
#define IPPROTO_IP 2
#define IPPROTO_IPV6 3
#define IPPROTO_TCP 4
#define IPPROTO_UDP 5
#define IPPROTO_ICMP 6
#define IPPROTO_ICMPV6 7
#define IPPROTO_RAW 8
#define IPPROTO_CUSTOM 9

// Socket options for SOL_SOCKET level
#define SO_DEBUG 1
#define SO_REUSEADDR 2
#define SO_TYPE 3
#define SO_ERROR 4
#define SO_DONTROUTE 5
#define SO_BROADCAST 6
#define SO_SNDBUF 7
#define SO_RCVBUF 8
#define SO_KEEPALIVE 9
#define SO_OOBINLINE 10
#define SO_LINGER 11
#define SO_REUSEPORT 12
#define SO_RCVLOWAT 13
#define SO_SNDLOWAT 14
#define SO_RCVTIMEO 15
#define SO_SNDTIMEO 16
#define SO_ACCEPTCONN 17
#define SO_PROTOCOL 18
#define SO_DOMAIN 19
#define SO_CUSTOM 20

// Socket options for IPPROTO_TCP level
#define TCP_NODELAY 1
#define TCP_MAXSEG 2
#define TCP_CORK 3
#define TCP_KEEPIDLE 4
#define TCP_KEEPINTVL 5
#define TCP_KEEPCNT 6
#define TCP_SYNCNT 7
#define TCP_LINGER2 8
#define TCP_DEFER_ACCEPT 9
#define TCP_WINDOW_CLAMP 10
#define TCP_INFO 11
#define TCP_QUICKACK 12
#define TCP_CONGESTION 13
#define TCP_MD5SIG 14
#define TCP_CUSTOM 15

// Socket options for IPPROTO_IP level
#define IP_TOS 1
#define IP_TTL 2
#define IP_HDRINCL 3
#define IP_OPTIONS 4
#define IP_ROUTER_ALERT 5
#define IP_RECVOPTS 6
#define IP_RETOPTS 7
#define IP_PKTINFO 8
#define IP_PKTOPTIONS 9
#define IP_MTU_DISCOVER 10
#define IP_RECVERR 11
#define IP_RECVTTL 12
#define IP_RECVTOS 13
#define IP_MTU 14
#define IP_FREEBIND 15
#define IP_IPSEC_POLICY 16
#define IP_XFRM_POLICY 17
#define IP_MULTICAST_IF 18
#define IP_MULTICAST_TTL 19
#define IP_MULTICAST_LOOP 20
#define IP_ADD_MEMBERSHIP 21
#define IP_DROP_MEMBERSHIP 22
#define IP_CUSTOM 23

// TCP flags
#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20
#define TCP_FLAG_ECE 0x40
#define TCP_FLAG_CWR 0x80

// IP address structure
typedef struct {
    uint8_t version;
    union {
        uint32_t v4;
        uint8_t v6[16];
    } addr;
} ip_addr_t;

// Socket address structure
typedef struct {
    socket_domain_t family;
    ip_addr_t addr;
    uint16_t port;
} socket_addr_t;

// Socket structure
typedef struct {
    int id;
    socket_type_t type;
    socket_state_t state;
    socket_flags_t flags;
    network_protocol_t protocol;
    socket_addr_t local_addr;
    socket_addr_t remote_addr;
    void* recv_buffer;
    size_t recv_buffer_size;
    size_t recv_buffer_used;
    void* send_buffer;
    size_t send_buffer_size;
    size_t send_buffer_used;
    int error;
    int reuse_addr;
    int keep_alive;
    int tcp_nodelay;
    int ip_ttl;
    int recv_timeout;
    int send_timeout;
} socket_t;

// TCP packet structure
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
    uint8_t options[40];
    uint8_t* data;
    size_t data_size;
} tcp_packet_t;

// UDP packet structure
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
    uint8_t* data;
    size_t data_size;
} udp_packet_t;

// TCP header structure
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} tcp_header_t;

// UDP header structure
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;

// Custom timeval structure for NeuroOS
typedef struct neuroos_timeval {
    int64_t tv_sec;
    int64_t tv_usec;
} neuroos_timeval_t;

// Socket interface functions
int socket_create(socket_type_t type, network_protocol_t protocol, socket_flags_t flags);
int socket_close(int socket_id);
int socket_bind(int socket_id, const socket_addr_t* addr);
int socket_listen(int socket_id, int backlog);
int socket_accept(int socket_id, socket_addr_t* addr);
int socket_connect(int socket_id, const socket_addr_t* addr);
ssize_t socket_send(int socket_id, const void* data, size_t size, int flags);
ssize_t socket_recv(int socket_id, void* buffer, size_t size, int flags);
ssize_t socket_sendto(int socket_id, const void* data, size_t size, int flags, const socket_addr_t* addr);
ssize_t socket_recvfrom(int socket_id, void* buffer, size_t size, int flags, socket_addr_t* addr);
int socket_setsockopt(int socket_id, int level, int option, const void* value, size_t value_len);
int socket_getsockopt(int socket_id, int level, int option, void* value, size_t* value_len);
int socket_get_info(int socket_id, socket_t* socket);

// TCP functions
int send_tcp_packet(int socket_id, tcp_packet_t* packet);
int receive_tcp_packet(int socket_id, tcp_packet_t* packet, int timeout_ms);
uint32_t generate_initial_sequence_number(void);

// Utility functions
int check_pending_connections(int socket_id);
int get_system_time_ms(void);
void schedule(void);
void sleep_ms(int ms);
network_interface_t* find_interface_for_destination(const ip_addr_t* addr);
int send_packet(network_interface_t* interface, network_packet_t* packet);
int receive_packet(network_interface_t* interface, network_packet_t* packet, int timeout_ms);
int is_ipv4_address(const char* str);
int is_ipv6_address(const char* str);
uint32_t parse_ipv4_address(const char* str);
int parse_ipv6_address(const char* str, uint8_t* addr);
int find_dns_server(ip_addr_t* server);
size_t create_dns_query(const char* hostname, uint8_t* query, size_t size);
int parse_dns_response(const uint8_t* response, size_t size, ip_addr_t* addr);
int parse_url(const char* url, char* protocol, size_t protocol_size, char* host, size_t host_size, int* port, char* path, size_t path_size);
int parse_ftp_pasv_response(const char* response);
int file_open(const char* filename, int flags);
int file_close(int file);
ssize_t file_read(int file, void* buffer, size_t size);
ssize_t file_write(int file, const void* buffer, size_t size);
size_t file_size(int file);

// File open flags
#define FILE_OPEN_READ 0x01
#define FILE_OPEN_WRITE 0x02
#define FILE_OPEN_CREATE 0x04
#define FILE_OPEN_TRUNCATE 0x08
#define FILE_OPEN_APPEND 0x10

#endif // NEUROOS_SOCKET_H
