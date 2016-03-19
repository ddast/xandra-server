# xandra-server
Receives and executes key and mouse events from
[xandra-android](https://github.com/ddast/xandra-android).
For more information on what xandra does see the description of xandra-android.
Tested on GNU/Linux.

## Usage

Usually starting xandra without parameters should suffice.
It is, however, possible to start the server on a different port.
```
xandra             start server on default port 64296
xandra [PORT]      start server on port [PORT]
xandra --help      display this help and exit
```

## Troubleshooting

**xandra uses the wrong keyboard layout (e.g. types z instead of y)**

If the keyboard layout is set system wide via a xorg.conf file for some
configurations xandra ignores these settings and defaults to the English
layout.
This can usually be fixed by setting the keyboard layout in the settings of
your desktop environment or by simply executing *setxkbmap* (no arguments
necessary).
