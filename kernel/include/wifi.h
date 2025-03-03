/**
 * wifi.h - WiFi interface for NeuroOS
 * 
 * This file contains the WiFi interface definitions and declarations.
 */

#ifndef NEUROOS_WIFI_H
#define NEUROOS_WIFI_H

#include <stddef.h>
#include <stdint.h>
#include "network.h"

// Maximum length of WiFi SSID
#define WIFI_SSID_MAX_LENGTH 32

// Maximum length of WiFi password
#define WIFI_PASSWORD_MAX_LENGTH 64

// WiFi security types
typedef enum {
    WIFI_SECURITY_NONE = 0,
    WIFI_SECURITY_WEP = 1,
    WIFI_SECURITY_WPA = 2,
    WIFI_SECURITY_WPA2 = 3,
    WIFI_SECURITY_WPA3 = 4,
    WIFI_SECURITY_ENTERPRISE = 5,
    WIFI_SECURITY_CUSTOM = 6
} wifi_security_t;

// WiFi command types
typedef enum {
    WIFI_CMD_NONE = 0,
    WIFI_CMD_SCAN = 1,
    WIFI_CMD_CONNECT = 2,
    WIFI_CMD_DISCONNECT = 3,
    WIFI_CMD_GET_STATUS = 4,
    WIFI_CMD_SET_POWER_SAVE = 5,
    WIFI_CMD_GET_POWER_SAVE = 6,
    WIFI_CMD_SET_CHANNEL = 7,
    WIFI_CMD_GET_CHANNEL = 8,
    WIFI_CMD_SET_RATE = 9,
    WIFI_CMD_GET_RATE = 10,
    WIFI_CMD_SET_TX_POWER = 11,
    WIFI_CMD_GET_TX_POWER = 12,
    WIFI_CMD_SET_ANTENNA = 13,
    WIFI_CMD_GET_ANTENNA = 14,
    WIFI_CMD_SET_MODE = 15,
    WIFI_CMD_GET_MODE = 16,
    WIFI_CMD_SET_COUNTRY = 17,
    WIFI_CMD_GET_COUNTRY = 18,
    WIFI_CMD_SET_ROAMING = 19,
    WIFI_CMD_GET_ROAMING = 20,
    WIFI_CMD_CUSTOM = 21
} wifi_cmd_type_t;

// MAC address structure
typedef struct {
    uint8_t addr[6];
} mac_addr_t;

// WiFi network structure
typedef struct {
    char ssid[WIFI_SSID_MAX_LENGTH];
    mac_addr_t bssid;
    uint8_t channel;
    int8_t signal_strength;
    wifi_security_t security;
} wifi_network_t;

// WiFi scan result structure
typedef struct {
    char ssid[WIFI_SSID_MAX_LENGTH];
    uint8_t bssid[6];
    uint8_t channel;
    int8_t signal_strength;
    wifi_security_t security;
} wifi_scan_result_t;

// WiFi connection parameters
typedef struct {
    char ssid[WIFI_SSID_MAX_LENGTH];
    char password[WIFI_PASSWORD_MAX_LENGTH];
    wifi_security_t security;
} wifi_connect_params_t;

// WiFi connection information
typedef struct {
    char ssid[WIFI_SSID_MAX_LENGTH];
    uint8_t bssid[6];
    uint8_t channel;
    int8_t signal_strength;
    wifi_security_t security;
    uint32_t ip_address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns1;
    uint32_t dns2;
} wifi_connection_info_t;

// WiFi status structure
typedef struct {
    uint8_t connected;
    uint8_t scanning;
    uint8_t error;
    uint8_t reserved;
    uint32_t error_code;
} wifi_status_t;

// WiFi command structure
typedef struct {
    wifi_cmd_type_t type;
    void* data;
    size_t data_size;
} wifi_command_t;


// DHCP result structure
typedef struct {
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns1;
    uint32_t dns2;
    uint32_t lease_time;
} dhcp_result_t;

// WiFi initialization and shutdown
int wifi_init(const char* interface_name);
int wifi_shutdown(void);

// WiFi scanning
size_t wifi_scan(wifi_network_t* networks, size_t max_count);
int wifi_is_scan_complete(network_interface_t* interface);
int wifi_get_scan_results(network_interface_t* interface, wifi_scan_result_t** results, size_t* count);

// WiFi connection
int wifi_connect(const char* ssid, const char* password, wifi_security_t security);
int wifi_disconnect(void);
int wifi_get_status(int* connected, wifi_network_t* network);
int wifi_get_connection_info(network_interface_t* interface, wifi_connection_info_t* info);

// WiFi configuration
int wifi_set_power_save(int enable);
int wifi_get_power_save(int* enable);
int wifi_set_channel(uint8_t channel);
int wifi_get_channel(uint8_t* channel);
int wifi_set_rate(uint32_t rate);
int wifi_get_rate(uint32_t* rate);
int wifi_set_tx_power(int8_t power);
int wifi_get_tx_power(int8_t* power);
int wifi_set_antenna(uint8_t antenna);
int wifi_get_antenna(uint8_t* antenna);
int wifi_set_mode(uint8_t mode);
int wifi_get_mode(uint8_t* mode);
int wifi_set_country(const char* country);
int wifi_get_country(char* country, size_t size);
int wifi_set_roaming(int enable);
int wifi_get_roaming(int* enable);

// WiFi command interface
int wifi_send_command(network_interface_t* interface, wifi_command_t* command);
int wifi_recv_response(network_interface_t* interface, void* response, size_t size);

// DHCP functions
int dhcp_get_ip_address(const char* interface_name, dhcp_result_t* result);
int dhcp_release_ip_address(const char* interface_name);
int dhcp_renew_ip_address(const char* interface_name);

// Utility functions
int get_device_unique_id(uint8_t* id, size_t size);
int network_packet_available(void);
int network_read_packet(network_packet_t* packet);
int network_handle_arp_packet(network_packet_t* packet);
int network_handle_ip_packet(network_packet_t* packet);
int network_handle_icmp_packet(network_packet_t* packet);
int network_handle_tcp_packet(network_packet_t* packet);
int network_handle_udp_packet(network_packet_t* packet);

#endif // NEUROOS_WIFI_H
