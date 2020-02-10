// run directory?

// The program to exec after fork, and the command line arguments to pass
// #define EXEC_INSTRUCTION "./actual_app"
// #define EXEC_ARGUMENTS "actual_app"
#define EXEC_INSTRUCTION "python"
#define EXEC_ARGUMENTS "python", "python_exploration/python_test.py"

// Enable to get print messages for just about every remote function call
#define VERBOSE 0

// Timeout for NNG calls which are blocking such as nng_recv
// Proxy calls nng_recv in the run loop, so this impacts responsiveness
#define ACTUAL_NNG_TIMEOUT 500

#define IPC_PIPE_ADDRESS "ipc://./pair.ipc"
