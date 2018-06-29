/**
 * Created by leer on 6/28/18.
 **/
#include "client.h"
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
#endif

typedef struct {
  bufferevent *remote_bev;
  bufferevent *local_bev;
} Client;

static event_base *base;
static SecureSocket *ss;

static void bufferevent_clear_free(bufferevent *bev) {
  bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
  bufferevent_free(bev);
}

void remote_readcb(bufferevent *remote_bev, void *arg) {
  log_t("read from remote");
  Client *client = (Client *)arg;
  check(client, "null context");
  bufferevent *local_bev = client->local_bev;
  // read data from remote, decode then write to local.
  check(decode_copy(ss, local_bev, remote_bev) != -1, "close all connection");
  return;
error:
  bufferevent_clear_free(local_bev);
  bufferevent_clear_free(remote_bev);
}

void local_readcb(bufferevent *bev, void *arg) {
  log_t("read from local");
  Client *client = (Client *)arg;
  check(client, "null context");
  bufferevent *remote_bev = client->remote_bev;

  // encode data then write to remote
  check(encode_copy(ss, remote_bev, bev) != -1, "close all connection");
  return;
error:
  bufferevent_clear_free(bev);
  bufferevent_clear_free(remote_bev);
}

void local_eventcb(bufferevent *bev, short events, void *arg) {
  if (events && (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
    // event_base *base = arg;
    Client *client = (Client *)arg;
    if (bev) {
      bufferevent_clear_free(bev); // client->local_bev
      bufferevent_clear_free(client->remote_bev);
      if (client) {
        free(client);
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

  // each connection has a client.
  Client *client = calloc(1, sizeof(Client));
  ss = (SecureSocket *)arg;

  bufferevent *bufev, *remote_bev;
  bufev = bufferevent_socket_new(
      base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
  log_d("new client conncted");

  remote_bev = bufferevent_socket_new(
      base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

  int rc =
      bufferevent_socket_connect(remote_bev, (struct sockaddr *)ss->remote_addr,
                                 sizeof(struct sockaddr_in));
  check(rc != -1, "can't connect proxy server");
  log_d("connected to proxy server");

  int remotefd = bufferevent_getfd(remote_bev);
  check(setsockopt(remotefd, SOL_SOCKET, SO_LINGER, (const void *)&linger,
                   sizeof(struct linger)) == 0,
        "can't disable linger");

  client->local_bev = bufev;
  client->remote_bev = remote_bev;

  bufferevent_setcb(bufev, local_readcb, NULL, local_eventcb, client);
  bufferevent_enable(bufev, EV_READ | EV_WRITE);
  bufferevent_setcb(remote_bev, remote_readcb, NULL, NULL, client);
  bufferevent_enable(remote_bev, EV_READ | EV_WRITE);
  return;
error:
  log_d("close all connection");
  bufferevent_clear_free(bufev);
  bufferevent_clear_free(remote_bev);
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

// temp
int main() {
  log_set_level(LOG_DEBUG);
  log_d("lightsocks-local: starting...");
  ss = calloc(1, sizeof(SecureSocket));

  // todo read password form json
  // ss->password = rand_password();
  Password *tmp_password = calloc(1, sizeof(Password));
  // temp password for test
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
    tmp_password->x[i] = table[i];
  }
  ss->password = tmp_password;
  sockaddr_in local; // listen to local
  local.sin_family = AF_INET;
  int local_port = 7448;
  local.sin_port = htons(local_port);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  ss->local_addr = &local;

  sockaddr_in remote; // connect to remote
  remote.sin_family = AF_INET;
  remote.sin_port = htons(42619);
  // todo read from json
  remote.sin_addr.s_addr = inet_addr("1.2.3.4");
  ss->remote_addr = &remote;

  event *signal_event;
  evconnlistener *listener;

  base = event_base_new();
  listener = evconnlistener_new_bind(
      base, local_listener_cb, ss, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
      16, (struct sockaddr *)ss->local_addr, sizeof(local));
  evconnlistener_set_error_cb(listener, listener_errorcb);

  log_d("\nListen on [::]:%d\nRemote:[167.99.66.41]:42619\nPassword:\n%s",
        local_port, password_string(tmp_password, NULL));

  // add Ctrl-C signal event
  signal_event = evsignal_new(base, SIGINT, signal_cb, base);
  event_add(signal_event, NULL);

  event_base_dispatch(base);
  log_w("lightsocks-loacl: exit");

  securesocket_free(ss);
  event_base_free(base);
  evconnlistener_free(listener);
  event_free(signal_event);
  return 0;
}