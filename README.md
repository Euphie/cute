## Cute [![](https://img.shields.io/badge/building-passing-green.svg)](http://euphie.me) [![](https://img.shields.io/badge/version-v0.7.1-yellow.svg)](http://euphie.me) [![](https://img.shields.io/badge/beta-yes-red.svg)](http://euphie.me)

## Description

A simple proxy server with websocket.

## Features

1. Multi-user support.
2. High performance.
3. Simple to use.

## Basic usage

* Cute Server

You can use cute server as follows:
```
git clone https://github.com/Euphie/cute.git
cd src
make
cd ..
bin/cute -c config/cute.ini 

usage: bin/cute options
  [-l address]
  [-p port]
  [-d daemon]
  [-c config file]
  [-h help]
  
example: bin/cute -p 9999 -d -c config/cute.ini

ps: The options will overwrite the contents of the configuration file. 
```

* Cute Client

The Cute client does not currently implement by C language, just only the GOLANG version in the bin directory. I compiled different client-side executables for different platforms.You can use a browser plugin such as SwitchyOmega(A Google Chrome extension) after you started the client then set up the socks5 proxy with the Cute local address.

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

## Installation

In linux, you can register cute as service, but must be run as root.
```
git clone https://github.com/Euphie/cute.git
cd src
make && make install
service cuted start
```

You can also uninstall the service.
```
make uninstall
```


## Communication

E-mail: euphie@yahoo.com
