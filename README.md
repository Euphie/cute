## Cute [![Travis](https://img.shields.io/travis/rust-lang/rust.svg)]() ![](https://img.shields.io/badge/version-v0.7-yellow.svg)

## Description

A simple proxy server with websocket.

## Installation

On Linux compile the software using "make" or "make install". 
```
cd src
make install
cd ..
bin/cute 
```

## Basic usage

* Cute Server

Command line syntax goes as follows:
```
usage: bin/cute options
  [-l address]
  [-p port]
  [-d daemon]
  [-c config file]
  [-h help]
example: bin/cute -p 9999 -d -c config/cute.ini
```

* Cute Client

The Cute client does not currently implement by C language, just only the GOLANG version in the bin directory. I compiled different client-side executables for different platforms.

```
usage: bin/cute-client-xxx options
  -u user name
  -k password
  -s cute server address
  [-p cute server port]
  [-l local socks5 port]
  [-t timeout in seconds]
  [-h help]
example: bin/cute-client-osx -s 119.88.88.88 -p 9999 -k 123456 -u cute
```
