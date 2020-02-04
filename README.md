# proxy

This is a cFS application that provides an interface to the core services. It is meant to be used with [https://aetd-git.gsfc.nasa.gov/cFS_lab/proxy_client/](proxy_client).

## Configuration

The file `fsw/src/proxy_defs.h` has a number of defines to change the proxy behavior.
The IPC pipe address needs to match that of proxy_client.

The program to run as a process is set by `EXEC_INSTRUCTION`, and the command line arguments (`EXEC_ARGUMENTS`) should start with the program name.

## Dependencies
