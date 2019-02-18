# lightsocks-c (WIP)

> A tiny SOCKS5 proxy which is inspired by [lightsocks-go](https://github.com/gwuhaolin/lightsocks) and uses [libevent](http://libevent.org/).
> 
> **WARNING: Lightsocks-server does not work now.**

## Build
 
**required libevent installed.**

```shell
make all
```

## Usage

``` shell
./bin/client <your config json path>
```

## Thanks

- [libevent](http://libevent.org/)

- [b64.c](https://github.com/littlstar/b64.c)

- [log.c](https://github.com/rxi/log.c)

- [parson](https://github.com/kgabis/parson)
