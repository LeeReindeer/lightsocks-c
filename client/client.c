/**
 * Created by leer on 6/28/18.
 **/
#include "client.h"
#include "../lib/log.h"
#include "../lib/password.h"
#include "../lib/securesocket.h"
#include "../lib/util.h"

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

static event_base *base = NULL;
static SecureSocket *ss = NULL;

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

// todo this is stupid..
int main(int argc, const char *argv[]) {
  log_set_level(LOG_DEBUG);
  check(argc == 1 || argc == 2, "number of args error, required 0 or 1 arg");
  log_d("lightsocks-local: starting...");
  ss = calloc(1, sizeof(SecureSocket));
  event *signal_event = NULL;
  evconnlistener *listener = NULL;

  int local_port, server_port;
  const char *server_addr = NULL;
  const char *password_str = NULL;

  JSON_Value *schema =
      json_parse_string("{\"local_port\":0,\"server_addr\":\"\",\"server_"
                        "port\":0,\"password\":\"\"}");

  JSON_Value *data = NULL;
  data = (argc == 1) ? json_parse_file(".lightsocks-config.json")
                     : json_parse_file(argv[1]);
  check(data, "null json");
  check(json_validate(schema, data) == JSONSuccess, "schema not match");

  local_port = json_object_get_number((json_object(data)), "local_port");
  server_addr = json_object_get_string((json_object(data)), "server_addr");
  server_port = json_object_get_number(json_object(data), "server_port");
  password_str = json_object_get_string((json_object(data)), "password");

  Password *password =
      gen_password_by_string(password_str, strlen(password_str));
  ss->password = password;
  sockaddr_in local; // listen to local
  memset(&local, 0, sizeof(sockaddr_in));
  local.sin_family = AF_INET;
  local.sin_port = htons(local_port);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  ss->local_addr = &local;

  sockaddr_in remote; // connect to remote
  memset(&remote, 0, sizeof(sockaddr_in));
  remote.sin_family = AF_INET;
  remote.sin_port = htons(server_port);
  remote.sin_addr.s_addr = inet_addr(server_addr);
  ss->remote_addr = &remote;

  base = event_base_new();
  check(base, "event base error");
  listener = evconnlistener_new_bind(
      base, local_listener_cb, ss, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
      16, (struct sockaddr *)ss->local_addr, sizeof(local));
  evconnlistener_set_error_cb(listener, listener_errorcb);
  check(listener, "listen error");

  log_d("\nListen on [::]:%d\nRemote:%s:%d\nPassword:\n%s", local_port,
        server_addr, server_port, password_string(password, NULL));

  // now there's no reason to keep json string
  json_value_free(schema);
  json_value_free(data);
  // add Ctrl-C signal event
  signal_event = evsignal_new(base, SIGINT, signal_cb, base);
  check(signal_event && event_add(signal_event, NULL) == 0,
        "signal even error");

  event_base_dispatch(base);

  securesocket_free(ss);
  if (listener) {
    evconnlistener_free(listener);
  }
  if (signal_event) {
    event_free(signal_event);
  }
  event_base_free(base);
  log_w("lightsocks-loacl: exit(0)");
  return 0;

error: // fallthrough
  log_w("lightsocks-loacl: exit(1)");
  if (schema) {
    json_value_free(schema);
  }
  if (data) {
    json_value_free(data);
  }
  securesocket_free(ss);
  if (listener) {
    evconnlistener_free(listener);
  }
  if (signal_event) {
    event_free(signal_event);
  }
  event_base_free(base);
  return -1;
}