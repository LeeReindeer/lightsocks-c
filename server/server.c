/**
 * Created by leer on 6/22/18.
 **/
#include "server.h"
#include "../lib/log.h"
#include "../lib/password.h"
#include "../lib/securesocket.h"

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
// For inet_addr
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
// For gethostbyname
#include <netdb.h>
#endif

#define SOCKS_BUF 257

typedef struct {
  // SecureSocket *ss;
  bufferevent *remote_bev; // what you really want
  bufferevent *local_bev;  // listen to lightsocks local
} LsServer;

static event_base *base;
static Password *global_password;

/**
 * @brief parse SOCKS5 to get remote addr, and response.
 * @param  *bev: bufferevent to read
 * @param  *remote_addr: sockaddr_in must has been alloc
 * @see  example: parse SOCKS4
 * http://www.wangafu.net/~nickm/libevent-book/Ref7_evbuffer.html
 * https://www.ietf.org/rfc/rfc1928.txt
 * @see  evbuffer_pullup()
 * @retval None
 */
static int parse_socks5(bufferevent *bev, sockaddr_in *remote_addr) {

  //           +----+----------+----------+
  //             |VER | NMETHODS | METHODS  |
  //             +----+----------+----------+
  //             | 1  |    1     | 1 to 255 |
  //             +----+----------+----------+
  byte buf[SOCKS_BUF] = {0};
  int n = decode_read(global_password, bev, buf, SOCKS_BUF);
  log_d("read %d bytes", n);

  if (buf[0] != 0x05) {
    log_e("SOCKS5 only!");
    return -1;
  }
  //    +----+--------+
  //    |VER | METHOD |
  //    +----+--------+
  //    | 1  |   1    |
  //    +----+--------+
  byte reply_auth[2] = {0x05, 0x00};
  encode_data(reply_auth, 2, global_password);
  n = bufferevent_write(bev, reply_auth, 2);
  // n = encode_write(global_password, bev, reply_auth, 2);
  log_d("write %d bytes", n);
  // sub-negotiation
  //
  // * +----+-----+-------+------+----------+----------+
  //   |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
  //   +----+-----+-------+------+----------+----------+
  //   | 1  |  1  | X'00' |  1   | Variable |    2     |
  //   +----+-----+-------+------+----------+----------+
  n = decode_read(global_password, bev, buf, SOCKS_BUF);
  log_d("read %d bytes", n);

  if (buf[1] != 0x01) {
    log_e("CONNECT only");
    return -1;
  }

  //  ATYP   address type of following address
  //     IP V4 address: X'01'
  //     DOMAINNAME: X'03'
  //     IP V6 address: X'04'
  remote_addr->sin_family = AF_INET;
  remote_addr->sin_port = htons(buf[n - 2] << 8 | buf[n - 1]);

  int name_len = n - 4 - 2;
  char domain[name_len];
  switch (buf[3]) {
  case 0x01:
    log_w("IP v4");
    in_addr_t ipv4 = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
    remote_addr->sin_addr.s_addr = htonl(ipv4);
    // remote_addr->sin_port = htons(buf[8] << 8 | buf[9]);
    break;
  case 0x03:
    log_w("domain name");
    for (int i = 0; i < name_len; i++) {
      domain[i] = buf[i + 4];
    }
    // remote_addr->sin_port = htons(buf[n - 2] << 8 | buf[n - 1]);
    log_d("resolve domain: %s", domain);
    struct hostent *host = gethostbyname(domain);
    remote_addr->sin_addr.s_addr = ((struct in_addr *)host->h_name)->s_addr;
    // struct addrinfo hints, *res;
    // memset(&hints, 0, sizeof hints);
    // hints.ai_family = AF_INET; // IPv4
    // hints.ai_socktype = SOCK_STREAM;
    // // hints.ai_flags = AI_PASSIVE;
    // status = getaddrinfo(domain, ntohs(remote_addr->sin_port), &hints, &res);
    return -1;
    break;
  case 0x04:
    log_w("IP v4 only");
    return -1;
    break;
  default:
    log_e("ATYP error");
    return -1;
    break;
  }
  return 0;
  // byte auth_buf[] = evbuffer_pullup(bufferevent_get_input(bev), 3);
}

// read from remote, then encode data send to Lslocal
void remote_readcb(bufferevent *remote_bev, void *arg) {
  LsServer *server = arg;
  check(server, "null context");

  check(encode_copy_pass(global_password, server->local_bev, remote_bev) != -1,
        "read error");
  return;
error:
  bufferevent_clear_free(server->local_bev);
  bufferevent_clear_free(remote_bev);
}

// read from Lslocal, prase SOCKS5, dial real remote addr.
void local_readcb(bufferevent *bev, void *arg) {
  LsServer *server = arg;
  check(server, "null context");

  bufferevent *remote_bev;
  sockaddr_in remote_addr; // connect to remote
  memset(&remote_addr, 0, sizeof(sockaddr_in));

  // parse socks5
  check(parse_socks5(bev, &remote_addr) != -1, "parse socks5 error");

  remote_bev = bufferevent_socket_new(
      base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

  int rc = bufferevent_socket_connect(
      remote_bev, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in));
  check(rc != -1, "can't connect remote server");
  log_d("connected to remote server");

  // disable linger
  int remotefd = bufferevent_getfd(remote_bev);
  struct linger linger;
  memset(&linger, 0, sizeof(struct linger));
  check(setsockopt(remotefd, SOL_SOCKET, SO_LINGER, (const void *)&linger,
                   sizeof(struct linger)) == 0,
        "can't disable linger");

  server->remote_bev = remote_bev;

  bufferevent_setcb(remote_bev, remote_readcb, NULL, NULL, server);
  bufferevent_enable(remote_bev, EV_READ | EV_WRITE);

  //+----+-----+-------+------+----------+----------+
  //|VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
  //+----+-----+-------+------+----------+----------+
  //| 1  |  1  | X'00' |  1   | Variable |    2     |
  //+----+-----+-------+------+----------+----------+
  // concted to remote server , send socks5 reply to Lslocal
  byte socks5_reply[] = {0x05, 0x00, 0x00, 0x01, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00};
  encode_write(global_password, bev, socks5_reply, 10);

  // SOCKS5 FINISH, START TRANSMIT
  // read data from local bev, then decode data and send to remote server
  check(decode_copy_pass(global_password, remote_bev, bev) != -1, "read error");

  return;
error:
  bufferevent_clear_free(bev);
  bufferevent_clear_free(remote_bev);
}

void local_eventcb(bufferevent *bev, short events, void *arg) {
  if (events && (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
    // event_base *base = arg;
    LsServer *server = arg;
    if (bev) {
      bufferevent_clear_free(bev); // client->local_bev
      if (server) {
        bufferevent_clear_free(server->remote_bev);
        free(server);
      }
    }
    if (events & BEV_EVENT_EOF) {
      log_d("connection %d closed", bufferevent_getfd(bev));
    } else if (events & BEV_EVENT_ERROR) {
      log_e("got an error on the connection");
    }
  }
}

void local_listener_cb(evconnlistener *listener, evutil_socket_t fd,
                       struct sockaddr *sa, int socklen, void *arg) {
  // set nonblocking
  check(evutil_make_socket_nonblocking(fd) == 0, "can't make no blocking");
  // no waiting for unsent data before closing socketing
  struct linger linger;
  memset(&linger, 0, sizeof(struct linger));
  check(setsockopt(fd, SOL_SOCKET, SO_LINGER, (const void *)&linger,
                   sizeof(struct linger)) == 0,
        "can't disable linger");

  Password *pass = arg;
  LsServer *ls_server = calloc(1, sizeof(LsServer));

  bufferevent *local_bev, *remote_bev;
  local_bev = bufferevent_socket_new(
      base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
  log_d("new client conncted");

  ls_server->local_bev = local_bev;

  bufferevent_setcb(local_bev, local_readcb, NULL, local_eventcb, ls_server);
  bufferevent_enable(local_bev, EV_READ | EV_WRITE);

  return;
error:
  bufferevent_clear_free(local_bev);
  free(ls_server);
}

void listener_errorcb(evconnlistener *listener, void *arg) {
  log_e("listen error, exit");
  event_base_loopbreak(base);
}

void signal_cb(evutil_socket_t sig, short events, void *user_data) {
  event_base *base = user_data;
  struct timeval delay = {1, 0};

  log_d("caught an interrupt signal; exiting cleanly in one "
        "seconds");

  event_base_loopexit(base, &delay);
}

int main() {
  log_d("lightsocks-server: starting...");
  // todo read password form json
  // ss->password = rand_password();
  global_password = calloc(1, sizeof(Password));
  byte table[] = {
      162, 135, 28,  12,  149, 252, 19,  6,   147, 47,  146, 53,  37,  143, 65,
      226, 77,  92,  115, 157, 151, 79,  26,  16,  89,  36,  82,  41,  153, 129,
      184, 23,  203, 160, 110, 250, 170, 42,  124, 182, 117, 233, 125, 5,   94,
      176, 249, 91,  59,  136, 76,  199, 205, 139, 108, 225, 43,  243, 140, 152,
      116, 171, 103, 48,  188, 17,  49,  27,  228, 102, 158, 50,  90,  3,   232,
      224, 173, 60,  132, 213, 45,  98,  118, 254, 211, 210, 128, 207, 175, 201,
      163, 206, 55,  161, 165, 0,   126, 154, 97,  198, 1,   183, 181, 239, 222,
      167, 39,  15,  58,  51,  236, 195, 96,  245, 131, 85,  9,   127, 142, 8,
      84,  185, 134, 166, 174, 218, 114, 155, 46,  7,   21,  104, 69,  194, 248,
      137, 99,  159, 10,  62,  172, 169, 138, 71,  220, 193, 216, 208, 179, 241,
      235, 130, 189, 180, 88,  64,  31,  24,  75,  192, 133, 123, 70,  255, 63,
      144, 231, 197, 87,  78,  18,  240, 119, 164, 145, 106, 190, 52,  80,  20,
      227, 196, 223, 202, 242, 244, 95,  22,  14,  230, 25,  74,  122, 4,   200,
      215, 38,  221, 204, 86,  105, 68,  238, 247, 214, 177, 2,   212, 66,  148,
      111, 187, 35,  109, 61,  168, 32,  30,  56,  178, 40,  112, 34,  209, 93,
      67,  251, 217, 237, 107, 73,  113, 83,  234, 29,  253, 191, 72,  186, 120,
      33,  13,  229, 44,  150, 219, 57,  121, 81,  141, 11,  156, 54,  100, 101,
      246};
  for (int i = 0; i < PASS_LENGTH; i++) {
    global_password->x[i] = table[i];
  }

  event *signal_event;
  evconnlistener *listener;

  sockaddr_in local; // listen to local
  memset(&local, 0, sizeof(sockaddr_in));
  local.sin_family = AF_INET;
  int local_port = 42619;
  local.sin_port = htons(local_port);
  local.sin_addr.s_addr = htonl(INADDR_ANY);

  base = event_base_new();
  listener =
      evconnlistener_new_bind(base, local_listener_cb, global_password,
                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 16,
                              (struct sockaddr *)&local, sizeof(local));
  check(listener, "listen error");
  evconnlistener_set_error_cb(listener, listener_errorcb);

  log_d("\nListen on [::]:%d\nPassword:\n%s", local_port,
        password_string(global_password, NULL));

  // add Ctrl-C signal event
  signal_event = evsignal_new(base, SIGINT, signal_cb, base);
  event_add(signal_event, NULL);

  event_base_dispatch(base);

error: // fall
  free(global_password);
  event_base_free(base);
  evconnlistener_free(listener);
  event_free(signal_event);
  return 0;
}
