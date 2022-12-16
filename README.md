# proxy

This is a cFS application that provides an interface to the core services.
It is meant to be used with [proxy_client](https://github.com/nasa/Multi-Process-Proxy-Client).

## Configuration

The file `fsw/src/proxy_defs.h` has a number of defines to change the proxy behavior.
The IPC pipe address needs to match that of proxy_client, but accounting for the different directories: proxy launches and runs with the working directory of the core-<target> executable, proxy_client will be loaded from "./cf/".
This is so the proxy client can load libraries in "./cf/".

The program to run as a process is set by `EXEC_INSTRUCTION`, and the command line arguments (`EXEC_ARGUMENTS`) should start with the program name.

## Dependencies
