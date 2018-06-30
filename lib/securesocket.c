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

int decode_read(Password *password, bufferevent *bufev, byte bytes[],
                size_t size) {
  int n = bufferevent_read(bufev, bytes, BUFFER_SIZE);
  // check(n > 0, "read error: %d", n);
  decode_data(bytes, n, password);

error: // fallthrogh
  return n;
}

int encode_write(Password *password, bufferevent *bufev, byte bytes[],
                 size_t size) {
  encode_data(bytes, size, password);
  int n = bufferevent_write(bufev, bytes, size);
  // check(n > 0, "write error: %d", n);

error:
  return n;
}

int decode_copy_pass(Password *pass, bufferevent *dst, bufferevent *src) {
  size_t n = 0;
  int first_time = 1;
  while (1) {
    byte buf[BUFFER_SIZE] = {0};
    n = decode_read(pass, src, buf, BUFFER_SIZE);
    if (n <= 0) {
      log_t("no more data");
      if (first_time) {
        // close all
        return -1;
      }
      break;
    }
    first_time = 0;
    bufferevent_write(dst, buf, n);
  }
  return 0;
}

int decode_copy(SecureSocket *ss, bufferevent *dst, bufferevent *src) {
  if (!ss || !ss->password) {
    return -1;
  }
  return decode_copy_pass(ss->password, dst, src);
}

int encode_copy_pass(Password *pass, bufferevent *dst, bufferevent *src) {
  size_t n = 0;
  int first_time = 1;
  while (1) {
    byte buf[BUFFER_SIZE] = {0};
    n = bufferevent_read(src, buf, BUFFER_SIZE);
    if (n <= 0) {
      log_t("no more data");
      if (first_time) {
        // close all
        return -1;
      }
      break;
    }
    first_time = 0;
    encode_write(pass, dst, buf, n); // write encode data
  }
  return 0;
}

int encode_copy(SecureSocket *ss, bufferevent *dst, bufferevent *src) {
  if (!ss || !ss->password) {
    return -1;
  }
  return encode_copy_pass(ss->password, dst, src);
}

void bufferevent_clear_free(bufferevent *bev) {
  bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
  bufferevent_free(bev);
}

void securesocket_free(SecureSocket *ss) {
  free(ss->password);
  free(ss);
}