/*
** GSC-18364-1, "Proxy Core Flight System Application and Client for External Process"
**
** Copyright © 2019-2022 United States Government as represented by
** the Administrator of the National Aeronautics and Space Administration.
** All Rights Reserved.
**
** Licensed under the NASA Open Source Agreement version 1.3
** See "NOSA GSC-18364-1.pdf"
*/

#ifndef proxy_defs_h
#define proxy_defs_h

// The program to exec after fork, and the command line arguments to pass

// Running a simple binary:
// #define EXEC_INSTRUCTION "./cf/actual_app"
// #define EXEC_ARGUMENTS "actual_app"

// Running a python script:
// #define EXEC_INSTRUCTION "python"
// #define EXEC_ARGUMENTS "python", "cf/python_exploration/python_test.py"

// Launching xterm (which then runs a python script)
#define EXEC_INSTRUCTION "/usr/bin/xterm"
#define EXEC_ARGUMENTS "xterm", "-fa", "'Monospace'", "-fs", "12", "-hold", "-e", "python", "cf/python_exploration/cfs_cli.py"

// Enable to get print messages for just about every remote function call
#define VERBOSE 0

// Timeout for NNG calls which are blocking such as nng_recv
// Proxy calls nng_recv in the run loop, so this impacts responsiveness
#define ACTUAL_NNG_TIMEOUT 500

#define IPC_PIPE_ADDRESS "ipc://./cf/pair.ipc"

#endif /* proxy_defs_h */
