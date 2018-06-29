#ifndef __CLIENT_H__
#define __CLIENT_H__
#include <event2/event.h>
#include <event2/listener.h>

// #define event_base struct event_base
// #define event struct event
// #define evconnlistener struct evconnlistener

typedef struct event_base event_base;
typedef struct event event;
typedef struct evconnlistener evconnlistener;

#endif