# proxy

This is a cFS application that provides an interface to the core services.

See [Multi-Process Proxy Client](https://github.com/nasa/Multi-Process-Proxy-Client) for usage and contribution instructions.

## Configuration

The file `fsw/platform_inc/proxy_defs.h` has a number of defines to change the proxy behavior.
The IPC pipe address needs to match that of proxy_client, but accounting for the different directories: proxy launches and runs with the working directory of the core-<target> executable, proxy_client will be loaded from "./cf/".
This is so the proxy client can load libraries in "./cf/".

The program to run as a process is set by `EXEC_INSTRUCTION`, and the command line arguments (`EXEC_ARGUMENTS`) should start with the program name.

## License and Copyright

Please refer to [NOSA GSC-18364-1.pdf](NOSA%20GSC-18364-1.pdf) and [COPYRIGHT](COPYRIGHT).
