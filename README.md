# web-server-c

A single-threaded, non-blocking HTTP/1.1 server in C. The goal of this project
is to learn about linux networking, operating systems, programming fundamentals
and everything else that comes along.

It serves static files from `./www` on `127.0.0.1:6969`. It is a learning
project, not a production server. _(See [current known limitations](#known-limitations))_.

## Build and run

```sh
make           # gcc -Wall -Wextra -Werror -fsanitize=address
./server       # prints "Server listening on 6969..."
```

In another shell:

```sh
curl http://127.0.0.1:6969/
make test      # asserts GET / serves the page and that path traversal is blocked
```

`make test` starts and stops the server itself. The Makefile builds with
AddressSanitizer by default, so leaks and out-of-bounds accesses fail loudly
during development.

## What it does

- One thread serves every client. There are alternative implementation
  approaches to concurrent connections; fork (`feat/fork-per-connection`) and
  threads (`feat/pthread-per-connection`) to see how the system behaves in
  different approaches _([btop](https://github.com/aristocratos/btop) is a good tool for it)_.
- Persistent HTTP/1.1 connections, including correct default semantics: an
  absent `Connection` header means keep-alive on 1.1 and close on 1.0, and an
  explicit header always wins.
- `Content-Length`, `Content-Type`, and truthful `Connection` headers on every
  response, including 400 and 404.
- Path traversal is rejected; query strings are stripped; `/` maps to
  `index.html` _(for now)_.

## Lifecycle

```
                         PROGRAM START
                              │
                              ▼
                       create sockaddr_in
                              │
                              ▼
                       server_init()
                              │
               ┌──────────────┴──────────────┐
               │                             │
            socket()                    configure address
               │                             │
               └──────────────┬──────────────┘
                              │
                           bind()
                              │
                           listen()
                              │
                              ▼
                     fds[0] = server_fd
                              │
                              ▼
                       server_loop()
                              │
                              ▼
                           poll()
                              │
                  ┌───────────┴───────────┐
                  │                       │
           server readable          client readable
                  │                       │
                  ▼                       ▼
              accept()             receive_request()
                  │                       │
                  ▼                       ▼
             new client              read bytes
                  │                       │
                  ▼                       ▼
             add to fds             detect headers
                                          │
                                          ▼
                                   parse_request()
                                          │
                                          ▼
                                     serve_file()
                                          │
                                  ┌───────┴────────┐
                                  │                │
                              keep-alive       connection close
                                  │                │
                                  ▼                ▼
                              poll again        close()
```
