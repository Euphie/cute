## Cute [![cute](http://euphie.me/svg/cute.version.svg)](http://euphie.me) #

## Description

A simple proxy server with websocket.

## Installation

On Linux compile the software using "make". 
```
make
bin/cute 
```

## Basic usage

* Cute Server

Command line syntax goes as follows:
```
usage: cute [-l address] [-p port] [-h help] [-d daemon]
example: bin/cute -l 119.88.88.88 -p 9999 -d
```

* Cute Client

The Cute client does not currently implement by C language, just only the GOLANG version in the bin directory. I compiled different client-side executables for different platforms.

```
usage: bin/cute-client-xxx [-s cute server address] [-p cute server port] [-l local socks5 port] [-t timeout in seconds] [-h help]
example: bin/cute-client-osx -s 119.88.88.88 -p 9999
```
