#ifndef __SECURESOCKET_H__
#define __SECURESOCKET_H__
#include "password.h"
#include <event2/bufferevent.h>
/**For portable socket on windows**/
#include <event2/util.h>

#define BUFFER_SIZE 1024
typedef struct bufferevent bufferevent;
typedef struct sockaddr_in sockaddr_in;

typedef struct {
  Password *password;
  // bufferevent *local_bufev;
  // bufferevent *remote_bufev;
  sockaddr_in *local_addr;  // local server's addr
  sockaddr_in *remote_addr; // remote proxy server's addr
} SecureSocket;

// void set_password(Password *password);

/**
 * @brief  Read encoded data from buffevent,
 * and write decode data to bytes
 * @param  *bufev:
 * @param  bytes[]:
 * @retval n bytes read
 */
int decode_read(Password *pass, bufferevent *bufev, byte bytes[], size_t size);

/**
 * @brief Encode data in bytes, then write to bufev
 * @retval n bytes writed
 */
int encode_write(Password *pass, bufferevent *bufev, byte bytes[], size_t size);

/**
 * @brief  Read encoded data from src, then write decoded to dst.
 * @param  *dst:
 * @param  *src:
 * @retval -1 if there is no data to read
 */
int decode_copy(SecureSocket *ss, bufferevent *dst, bufferevent *src);

int decode_copy_pass(Password *pass, bufferevent *dst, bufferevent *src);

/**
 * @brief  Read data from src, encode it and write into dst
 * @param  *dst:
 * @param  *src:
 * @retval -1 if there is no data to read
 */
int encode_copy(SecureSocket *ss, bufferevent *dst, bufferevent *src);

int encode_copy_pass(Password *pass, bufferevent *dst, bufferevent *src);

/**
 * @brief  CLear buffferevent callback adn free it.
 */
void bufferevent_clear_free(bufferevent *bev);

void securesocket_free(SecureSocket *ss);

#endif // __SECURESOCKET__