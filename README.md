# xandra-server
Receives and executes key and mouse events from
[xandra-android](https://github.com/ddast/xandra-android).
For more information on what xandra does see the description of xandra-android.

Tested on GNU/Linux and BSD.

## Usage

Usually starting xandra without parameters should suffice.
It is, however, possible to start the server on a different port.
```
Usage: xandra [-4|-6] [PORT]
Start xandra server on port PORT (default 64296).

  -4          use only IPv4
  -6          use only IPv6
  --help      display this help and exit

```

## Installation

There are no binaries for download so you have to compile for yourself.

### Prerequisites

xandra requires the libxdo library which is available for most distributions:

* Arch Linux  
  ```
  # pacman -S xdotool  
  ```
* Debian/Ubuntu  
  ```
  # apt-get install libxdo-dev
  ```
* Fedora  
  ```
  # dnf install libxdo-devel libX11-devel
  ```
* openSUSE  
  ```
  # zypper install xdotool-devel
  ```
* FreeBSD  
  The version in ports (x11/xdotool) is too old (2.20110530.1) and does not
  work.  With a more current version from upstream it works fine.

Depending on your installation installing a C compiler (probably *gcc*) or
*make* may also be required.

### Compiling
Download the current version from
[here](https://github.com/ddast/xandra-server/releases) and unpack.
```
$ cd xandra-server/src
$ make
```
Copy the resulting binary *xandra* to your preferred place (or use *make
install*).


## Troubleshooting

**xandra uses the wrong keyboard layout (e.g. types z instead of y)**

If the keyboard layout is set system wide via a xorg.conf file for some
configurations xandra ignores these settings and defaults to the English
layout.
This can usually be fixed by setting the keyboard layout in the settings of
your desktop environment or by simply executing *setxkbmap* (no arguments
necessary).

**Cannot connect to xandra**

Some distributions (such as Fedora) use a firewall that will deny access.
To solve this you have to allow TCP connections on port 64296.
