#include "securesocket.h"
#include "log.h"
#include <stdlib.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
/**For portable socket on windows**/
#include <event2/util.h>

// static Password *password = NULL;

// void set_password(Password *this_password) {
//   if (password) {
//     free(password);
//   }
//   password = this_password;
// }

int decode_read(SecureSocket *ss, bufferevent *bufev, byte bytes[],
                size_t size) {
  int n = bufferevent_read(bufev, bytes, BUFFER_SIZE);
  check(n > 0, "read error: %d", n);
  decode_data(bytes, n, ss->password);

error: // fallthrogh
  return n;
}

int encode_write(SecureSocket *ss, bufferevent *bufev, byte bytes[],
                 size_t size) {
  encode_data(bytes, size, ss->password);
  int n = bufferevent_write(bufev, bytes, BUFFER_SIZE);
  check(n > 0, "write error: %d", n);

error:
  return n;
}

void decode_copy(SecureSocket *ss, bufferevent *dst, bufferevent *src) {
  byte buf[BUFFER_SIZE] = {0};
  int n;
  while ((n = decode_read(ss, src, buf, BUFFER_SIZE)) > 0) {
    bufferevent_write(dst, buf, n);
  }
}

void encode_copy(SecureSocket *ss, bufferevent *dst, bufferevent *src) {
  byte buf[BUFFER_SIZE] = {0};
  int n;
  while ((n = bufferevent_read(src, buf, BUFFER_SIZE)) > 0) {
    encode_write(ss, dst, buf, n); // write encode data
  }
}

void securesocket_free(SecureSocket *ss) {
  free(ss->password);
  free(ss);
}