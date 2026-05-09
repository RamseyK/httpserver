# HTTP Server

A high-performance, single-threaded, event-driven HTTP/1.1 server
written in C++23, built on kqueue. Intended as a learning tool for
kqueue-based socket management and the HTTP/1.1 protocol on BSD systems.

## Features
* Efficient event-driven I/O via kqueue
* Optional privilege dropping (`drop_uid` / `drop_gid`) after binding
* Self-contained HTTP/1.1 parser built on the [ByteBuffer](https://github.com/RamseyK/ByteBufferCpp) library
* Clean, documented code
* Tested on FreeBSD and macOS

## Requirements
* A BSD-based system (FreeBSD, macOS) — Linux _may_ work via [libkqueue](https://github.com/mheily/libkqueue)
* A C++23-capable clang compiler
* `make` on macOS; `gmake` on FreeBSD

## Quick start

```sh
git clone https://github.com/RamseyK/httpserver.git
cd httpserver
make                       # produces ./bin/httpserver
mkdir htdocs               # document root
$EDITOR server.config      # see Configuration below
./bin/httpserver
```

## Configuration

`server.config` is read from the working directory. Lines starting with
`#` are ignored. Format: `key=value`, one per line.

| Key         | Required | Description                                              |
| ----------- | -------- | -------------------------------------------------------- |
| `vhost`     | yes      | Comma-separated list of host aliases to serve            |
| `port`      | yes      | TCP port (1–65535)                                       |
| `diskpath`  | yes      | Path to document root (must exist)                       |
| `drop_uid`  | no       | UID to drop to after binding (must be set with `drop_gid`) |
| `drop_gid`  | no       | GID to drop to after binding (must be set with `drop_uid`) |

### Example

```
$ cat server.config
vhost=10.0.10.86,acme.local
port=8080
diskpath=./htdocs

$ ./bin/httpserver
Primary port: 8080, disk path: ./htdocs
vhost: 10.0.10.86
vhost: acme.local
Server ready. Listening on port 8080...
```

## License
Apache License v2.0. See LICENSE file.

---
Ramsey Kant — https://github.com/RamseyK/httpserver
