# saas

Turn a command line program into a network service.

## Synopsis

**saas** _hostname_ _port_ -- _program_ ...

**saas** [_sockfile_] -- _program_ ...

## Description

**saas** listens on the given _hostname_ and TCP _port_ or UNIX socket
_sockfile_ (`/tmp/saas.sock` by default) and dispatches incoming connections
to te given _program_.

## Examples

Serve quotes on port 5000:

    $ saas 0.0.0.0 5000 -- fortune

Read them with nc(1):

    $ nc localhost 5000

## Installation

From source, after tweaking the Makefile to taste:

    make
    make install

Uninstall with `make uninstall`.

## See also

nc(1), websocketd(1).

## Authors

Sijmen J. Mulder (<ik@sjmulder.nl>)

## Caveats

If a program buffers its output (as most do when not connected to a an
interactive terminal) nothing will be sent to the client until the stream
is flushed.  Disable output buffering in the program or use a psuedo-TTY
program to simulate a terminal.
