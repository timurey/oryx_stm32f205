/**
 * @file tcp_ip_stack_config.h
 * @brief CycloneTCP configuration file
 *
 * @section License
 *
 * Copyright (C) 2010-2014 Oryx Embedded. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded (www.oryx-embedded.com)
 * @version 1.6.3
 **/

#ifndef _NET_CONFIG_H
#define _NET_CONFIG_H

//#define NET_TICK_PRIORITY 4
//#define NET_RX_PRIORITY 3
//Trace level for TCP/IP stack debugging
#define MEM_TRACE_LEVEL          4
#define NIC_TRACE_LEVEL          2
#define ETH_TRACE_LEVEL          2
#define ARP_TRACE_LEVEL          2
#define IP_TRACE_LEVEL           2
#define IPV4_TRACE_LEVEL         2
#define IPV6_TRACE_LEVEL         2
#define ICMP_TRACE_LEVEL         2
#define IGMP_TRACE_LEVEL         0
#define ICMPV6_TRACE_LEVEL       2
#define MLD_TRACE_LEVEL          2
#define NDP_TRACE_LEVEL          2
#define UDP_TRACE_LEVEL          2
#define TCP_TRACE_LEVEL          2
#define SOCKET_TRACE_LEVEL       2
#define RAW_SOCKET_TRACE_LEVEL   2
#define BSD_SOCKET_TRACE_LEVEL   2
#define SLAAC_TRACE_LEVEL        2
#define DHCP_TRACE_LEVEL         2
#define DHCPV6_TRACE_LEVEL       2
#define DNS_TRACE_LEVEL          2
#define NBNS_TRACE_LEVEL         2
#define LLMNR_TRACE_LEVEL        2
#define FTP_TRACE_LEVEL          2
#define HTTP_TRACE_LEVEL         2
#define SMTP_TRACE_LEVEL         2
#define SNTP_TRACE_LEVEL         2
#define STD_SERVICES_TRACE_LEVEL 2
#define DNS_SD_TRACE_LEVEL       2
#define MDNS_TRACE_LEVEL		 2
#define AUTO_IP_TRACE_LEVEL      2

#define ETH_FAST_CRC_SUPPORT ENABLED
#define NET_DEPRECATED_API_SUPPORT DISABLED

//Number of network adapters
#define NET_INTERFACE_COUNT 1

//Maximum size of the MAC filter table
#define MAC_FILTER_MAX_SIZE 4

//IPv4 support
#define IPV4_SUPPORT ENABLED
//Maximum size of the IPv4 filter table
#define IPV4_FILTER_MAX_SIZE 4

//IPv4 fragmentation support
#define IPV4_FRAG_SUPPORT DISABLED
//Maximum number of fragmented packets the host will accept
//and hold in the reassembly queue simultaneously
#define IPV4_MAX_FRAG_DATAGRAMS 2
//Maximum datagram size the host will accept when reassembling fragments
#define IPV4_MAX_FRAG_DATAGRAM_SIZE 2048

//Size of ARP cache
#define ARP_CACHE_SIZE 8
//Maximum number of packets waiting for address resolution to complete
#define ARP_MAX_PENDING_PACKETS 2

//IGMP support
#define IGMP_SUPPORT ENABLED

//IPv6 support
#define IPV6_SUPPORT DISABLED
//Maximum size of the IPv6 filter table
#define IPV6_FILTER_MAX_SIZE 8

//IPv6 fragmentation support
#define IPV6_FRAG_SUPPORT ENABLED
//Maximum number of fragmented packets the host will accept
//and hold in the reassembly queue simultaneously
#define IPV6_MAX_FRAG_DATAGRAMS 2
//Maximum datagram size the host will accept when reassembling fragments
#define IPV6_MAX_FRAG_DATAGRAM_SIZE 2048

#define APP_USE_SLAAC ENABLED

//MLD support
#define MLD_SUPPORT ENABLED

//Neighbor cache size
#define NDP_CACHE_SIZE 8
//Maximum number of packets waiting for address resolution to complete
#define NDP_MAX_PENDING_PACKETS 2

//ICMP support
#define ICMP_SUPPORT ENABLED

//TCP support
#define TCP_SUPPORT ENABLED
//Default buffer size for transmission
#define TCP_DEFAULT_TX_BUFFER_SIZE (1536*2)
//#define TCP_MAX_TX_BUFFER_SIZE (1536*16)
//Default buffer size for reception
#define TCP_DEFAULT_RX_BUFFER_SIZE (1536*4)//(1430*6)
//#define TCP_MAX_RX_BUFFER_SIZE (1536*8)
//SYN queue size for listening sockets
//#define TCP_DEFAULT_SYN_QUEUE_SIZE 8
//Maximum number of retransmissions
#define TCP_MAX_RETRIES 5
//Selective acknowledgment support
#define TCP_SACK_SUPPORT ENABLED

//UDP support
#define UDP_SUPPORT ENABLED
//Receive queue depth for connectionless sockets
#define UDP_RX_QUEUE_SIZE 4

//Raw socket support
#define RAW_SOCKET_SUPPORT DISABLED
//Receive queue depth for raw sockets
#define RAW_SOCKET_RX_QUEUE_SIZE 6

//Number of sockets that can be opened simultaneously
#define SOCKET_MAX_COUNT 8

#define DNS_CLIENT_SUPPORT ENABLED
#define MDNS_CLIENT_SUPPORT ENABLED
#define MDNS_RESPONDER_SUPPORT ENABLED
#define DNS_SD_SUPPORT ENABLED

#define NBNS_CLIENT_SUPPORT DISABLED
#define NBNS_RESPONDER_SUPPORT DISABLED

#define BSD_SOCKET_SUPPORT DISABLED
#define NET_MAX_HOSTNAME_LEN 32

//#define NET_MEM_POOL_SUPPORT ENABLED
//#define NET_MEM_POOL_BUFFER_COUNT 16
//#define NET_MEM_POOL_BUFFER_SIZE 1536

/*HTTP section*/
//#define HTTP_SERVER_PRIORITY 3
#define HTTP_SERVER_STACK_SIZE (512-64)

//#define HTTP_SERVER_SSI_SUPPORT ENABLED
#define HTTP_SERVER_SUPPORT ENABLED
#define APP_HTTP_MAX_CONNECTIONS 12
#define HTTP_SERVER_BUFFER_SIZE 512
#define HTTP_SERVER_FS_SUPPORT ENABLED
//#define HTTP_SERVER_TIMEOUT 5000
//#define HTTP_SERVER_MAX_REQUESTS 1000
#define HTTP_SERVER_CACHE_CONTROL ENABLED
#define HTTP_SERVER_MAX_AGE 604800
#define HTTP_SERVER_GZIPED_FILES ENABLED
#define HTTP_SERVER_MULTIPART_TYPE_SUPPORT ENABLED
//Force HTTP server to use persistent connections
#define HTTP_SERVER_PERSISTENT_CONN_SUPPORT DISABLED
#define HTTP_SERVER_ROOT_DIR_MAX_LEN 15
#define HTTP_SERVER_DEFAULT_DOC_MAX_LEN 15
#define HTTP_SERVER_URI_MAX_LEN 127
#define HTTP_SERVER_QUERY_STRING_MAX_LEN 32 //max len of parameteres
#define FTP_SERVER_SUPPORT ENABLED
//#define FTP_SERVER_PRIORITY 3
//#define FTP_SERVER_STACK_SIZE (552)
#define FTP_SERVER_CTRL_SOCKET_BUFFER_SIZE 1430
#define FTP_SERVER_DATA_SOCKET_BUFFER_SIZE 1430
#define FTP_SERVER_BUFFER_SIZE 1536

//#define AUTO_IP_SUPPORT ENABLED

#define DNS_CACHE_SIZE 16

//#define NET_TICK_STACK_SIZE 550*4
//#define NET_RX_STACK_SIZE 650*4
#endif
