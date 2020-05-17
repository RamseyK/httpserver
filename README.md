# HTTP Server

Ramsey Kant
https://github.com/RamseyK/httpserver

A high performance, single threaded, HTTP server written in C++ to serve as a kqueue socket management and HTTP protocol learning tool on BSD systems

## Features
* Clean, documented code
* Efficient socket management with kqueue
* Easy to understand HTTP protocol parser (from my [ByteBuffer](https://github.com/RamseyK/ByteBufferCpp) project)
* Tested on FreeBSD and MacOS

## Compiling
* Only BSD based systems are supported.  Linux _may_ work when [libkqueue is compiled from Github sources](https://github.com/mheily/libkqueue) and linked, but this is unsupported.
* On FreeBSD, compile with gmake

## Usage

```

$ cat server.config 
vhost=10.0.10.86,acme.local
port=8080
diskpath=./htdocs

$ ./httpserver 
Primary port: 8080, disk path: ./htdocs
vhost: 10.0.10.86
vhost: acme.local
Server ready. Listening on port 8080...
```

## License
See LICENSE.TXT
