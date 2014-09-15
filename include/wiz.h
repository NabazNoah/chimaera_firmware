/*
 * Copyright (c) 2014 Hanspeter Portner (dev@open-music-kontrollers.ch)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 */

#ifndef _WIZ_H_
#define _WIZ_H_

#include <stdint.h>

#include <libmaple/gpio.h>

#include <config.h>

extern const uint8_t wiz_broadcast_ip [];
extern const uint8_t wiz_nil_ip [];
extern const uint8_t wiz_nil_mac [];
extern const uint8_t wiz_broadcast_mac [];

#define WIZ_UDP_HDR_SIZE			8

#if WIZ_CHIP == 5200
#	define WIZ_MAX_SOCK_NUM			8
#	define WIZ_SEND_OFFSET			4
#elif WIZ_CHIP == 5500
#	define WIZ_MAX_SOCK_NUM 		8
#	define WIZ_SEND_OFFSET			3
#endif

/* socket interrupt masks */
#define WIZ_Sn_IR_PRECV						(1U << 7) // valid only on socket 0 & PPPoE and W5200
#define WIZ_Sn_IR_PFAIL						(1U << 6) // valid only on socket 0 & PPPoE and W5200
#define WIZ_Sn_IR_PNEXT						(1U << 5) // valid only on socket 0 & PPPoE and W5200
#define WIZ_Sn_IR_SEND_OK					(1U << 4)
#define WIZ_Sn_IR_TIMEOUT					(1U << 3)
#define WIZ_Sn_IR_RECV						(1U << 2)
#define WIZ_Sn_IR_DISCON					(1U << 1)
#define WIZ_Sn_IR_CON							(1U << 0)

typedef void(*Wiz_UDP_Dispatch_Cb)(uint8_t *ip, uint16_t port, uint8_t *buf, uint16_t len);
typedef void(*Wiz_IRQ_Cb)(uint8_t isr);
typedef enum _Wiz_Socket_State Wiz_Socket_State;

enum _Wiz_Socket_State {
	WIZ_SOCKET_STATE_CLOSED		= 0,
	WIZ_SOCKET_STATE_CONNECT	= 1,
	WIZ_SOCKET_STATE_OPEN			= 2,
};

extern Wiz_Socket_State wiz_socket_state [];

void wiz_init(gpio_dev *dev, uint8_t bit);
void wiz_sockets_set(uint8_t tx_mem[WIZ_MAX_SOCK_NUM], uint8_t rx_mem[WIZ_MAX_SOCK_NUM]);
uint_fast8_t wiz_link_up(void);

void wiz_mac_set(uint8_t *mac);
void wiz_ip_set(uint8_t *ip);
void wiz_gateway_set(uint8_t *gateway);
void wiz_subnet_set(uint8_t *subnet);
void wiz_comm_set(uint8_t *mac, uint8_t *ip, uint8_t *gateway, uint8_t *subnet);

uint_fast8_t wiz_is_broadcast(uint8_t *ip);
uint_fast8_t wiz_is_multicast(uint8_t *ip);

void wiz_irq_handle(void);
void wiz_irq_set(Wiz_IRQ_Cb cb, uint8_t mask);
void wiz_irq_unset(void);

void wiz_socket_irq_set(uint8_t socket, Wiz_IRQ_Cb cb, uint8_t mask);
void wiz_socket_irq_unset(uint8_t socket);

/* 
 * UDP
 */

void udp_begin(uint8_t sock, uint16_t port, uint_fast8_t multicast);
void udp_end(uint8_t sock);

void udp_update_read_write_pointers(uint8_t sock);
void udp_reset_read_write_pointers(uint8_t sock);

void udp_set_remote(uint8_t sock, uint8_t *ip, uint16_t port);
void udp_get_remote(uint8_t sock, uint8_t *ip, uint16_t *port);
void udp_set_remote_har(uint8_t sock, uint8_t *har);

void udp_send(uint8_t sock, uint8_t *o_buf, uint16_t len);
uint_fast8_t udp_send_nonblocking(uint8_t sock, uint8_t *o_buf, uint16_t len);
void udp_send_block(uint8_t sock);

uint16_t udp_available(uint8_t sock);

void udp_receive(uint8_t sock, uint8_t *i_buf, uint16_t len);
void udp_peek(uint8_t sock, uint8_t *i_buf, uint16_t len);
uint_fast8_t udp_receive_nonblocking(uint8_t sock, uint8_t *i_buf, uint16_t len);
void udp_receive_block(uint8_t sock);

void udp_dispatch(uint8_t sock, uint8_t *i_buf, Wiz_UDP_Dispatch_Cb cb);

void udp_skip(uint8_t sock, uint16_t len);
#define udp_ignore(sock) udp_skip((sock), udp_available((sock)))

/*
 * TCP
 */

void tcp_begin(uint8_t sock, uint16_t port, uint_fast8_t server);
void tcp_end(uint8_t sock);

void tcp_send(uint8_t sock, uint8_t *o_buf, uint16_t len);
#define tcp_send_nonblocking udp_send_nonblocking
void tcp_send_block(uint8_t sock);

#define tcp_receive udp_receive
#define tcp_peek udp_peek
#define tcp_available udp_available
void tcp_dispatch(uint8_t sock, uint8_t *ip, uint16_t port, uint8_t *i_buf, Wiz_UDP_Dispatch_Cb cb, uint8_t slip);

#define tcp_skip udp_skip
#define tcp_ignore udp_ignore

/*
 * OSC
 */

void osc_send(OSC_Config *osc, uint8_t *o_buf, uint16_t len);
uint_fast8_t osc_send_nonblocking(OSC_Config *osc, uint8_t *o_buf, uint16_t len);
void osc_send_block(OSC_Config *osc);

void osc_dispatch(OSC_Config *osc, uint8_t *i_buf, Wiz_UDP_Dispatch_Cb cb);

#define osc_skip udp_skip
#define osc_ignore udp_ignore

/*
 * MACRAW
 */

typedef struct _MACRAW_Header MACRAW_Header;

struct _MACRAW_Header {
	uint8_t dst_mac [6];
	uint8_t src_mac [6];
	uint16_t type;
} __attribute((packed,aligned(2)));

#define MACRAW_HEADER_SIZE sizeof(MACRAW_Header)

typedef void(*Wiz_MACRAW_Dispatch_Cb)(uint8_t *buf, uint16_t len, void *data);

void macraw_begin(uint8_t sock, uint_fast8_t mac_filter);
#define macraw_end udp_end

#define macraw_send udp_send
#define macraw_available udp_available
#define macraw_receive udp_receive

void macraw_dispatch(uint8_t sock, uint8_t *i_buf, Wiz_MACRAW_Dispatch_Cb cb, void *data);

#endif // _WIZ_H_
