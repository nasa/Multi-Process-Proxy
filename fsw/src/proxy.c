/*
** GSC-18364-1, "Proxy Core Flight System Application and Client for External Process"
**
** Copyright © 2019 United States Government as represented by
** the Administrator of the National Aeronautics and Space Administration.
** No copyright is claimed in the United States under Title 17, U.S. Code.
** All Other Rights Reserved.
**
** Licensed under the NASA Open Source Agreement version 1.3
** See "NOSA GSC-18364-1.pdf"
*/

/*
 * Shutdown behavior:
 * The app should detect a shutdown via runloop and call exit_app
 * If the app is hanging, then cFS will eventually send a hangup signal
 *
 * So, the proxy needs to let the app check runloop, and call exit_app...
 * otherwise it needs to send the hangup signal right before cFS sends it to the proxy
 *
 * But how is cFS sending hangup to a thread?
 */

/*
**   Include Files:
*/

#include "proxy.h"
#include "proxy_perfids.h"
#include "proxy_msgids.h"
#include "proxy_msg.h"
#include "proxy_events.h"
#include "proxy_version.h"
#include "proxy_defs.h"

#include <signal.h>

#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>

// Flat Buff Stuff
#include <cfs_api_builder.h>
#include <cfs_return_builder.h>
#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(cFS_API, x)
#undef nsr
#define nsr(x) FLATBUFFERS_WRAP_NAMESPACE(cFS_Return, x)

#define ACTUAL_STATE_UNKOWN    1
#define ACTUAL_STATE_RUNNING   2
#define ACTUAL_STATE_TIMED_OUT 3
#define ACTUAL_STATE_EXITED    4

/*
** global data
*/

flatcc_builder_t   builder;
proxy_hk_tlm_t     PROXY_HkTelemetryPkt;
CFE_SB_PipeId_t    PROXY_CommandPipe;
CFE_SB_MsgPtr_t    PROXY_MsgPtr;

nng_socket sock;

pid_t childPID;

// APP ID for the proxy event app
unsigned int proxy_evs_id = 88;

static CFE_EVS_BinFilter_t  PROXY_EventFilters[] =
    {  /* Event ID    mask */
        {PROXY_STARTUP_INF_EID,       0x0000},
        {PROXY_COMMAND_ERR_EID,       0x0000},
        {PROXY_COMMANDNOP_INF_EID,    0x0000},
        {PROXY_COMMANDRST_INF_EID,    0x0000},
    };

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* PROXY_Main() -- Application entry point and main process loop          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void PROXY_Main( void )
{
    int32  status;
    uint32 RunStatus = CFE_ES_APP_RUN;

    CFE_ES_PerfLogEntry(PROXY_PERF_ID);

    PROXY_Init();

    // Main run loop
    while (CFE_ES_RunLoop(&RunStatus) == TRUE)
    {
        // Two parts: check for proxy commands and check for messages from the actual app

        status = CFE_SB_RcvMsg(&PROXY_MsgPtr, PROXY_CommandPipe, CFE_SB_POLL);

        // TODO: Consider use of Perf markers

        if (status == CFE_SUCCESS)
        {
            PROXY_ProcessCommandPacket();
        }

        incoming_message();
    }

    // The App has been killed
    // Need to give actual a chance to shutdown on its own
    // TODO: Actual timeout...
    int the_final_countdown = 6;
    while(the_final_countdown--) {
        incoming_message();
    }

    printf("PROXY Is about to die.\n");
    kill(childPID, SIGKILL);

    cleanup_and_exit(RunStatus);
} /* End of PROXY_Main() */

void cleanup_and_exit( uint32 RunStatus )
{
    PROXY_ReportHousekeeping();

    // Clean up flatcc
    flatcc_builder_clear(&builder);

    // Clean up nng
    nng_close(sock);

    printf("Proxy exiting.\n");
    CFE_ES_ExitApp(RunStatus);
}

// None of the function arguments need to be returned
// Does send a single int32 as the return of the function
void return_regular_int32(int32 call_return)
{
    size_t size;
    int rv;
    void *flat_buffer;
    // Send the return value
    flatcc_builder_t *B = &builder;

    nsr(Empty_ref_t) empty = nsr(Empty_create(B));
    nsr(PointerReturn_union_ref_t) output = nsr(PointerReturn_as_Empty(empty));
    nsr(Integer32_ref_t) call_return_table = nsr(Integer32_create(B, call_return));
    nsr(FuncReturn_union_ref_t) retval = nsr(FuncReturn_as_Integer32(call_return_table));
    nsr(ReturnData_create_as_root(B, retval, output));

    flat_buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    rv = nng_send(sock, flat_buffer, size, 0);
    if (rv == 0)
    {
        //printf("return_regular_int32 - nng_send: %d\n", rv);
    }
    else
    {
        // TODO: Error event for NNG issues
        printf("return_regular_int32 - Oh No! nng_send: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    }

    /* free(buffer); */
    flatcc_builder_aligned_free(flat_buffer);
    /*
     * Reset, but keep allocated stack etc.,
     * or optionally reduce memory using `flatcc_builder_custom_reset`.
     */
    flatcc_builder_reset(B);
}

void return_regular_uint32(uint32 call_return)
{
    size_t size;
    int rv;
    void *flat_buffer;
    // Send the return value
    flatcc_builder_t *B = &builder;

    nsr(Empty_ref_t) empty = nsr(Empty_create(B));
    nsr(PointerReturn_union_ref_t) output = nsr(PointerReturn_as_Empty(empty));
    nsr(UnInteger32_ref_t) call_return_table = nsr(UnInteger32_create(B, call_return));
    nsr(FuncReturn_union_ref_t) retval = nsr(FuncReturn_as_UnInteger32(call_return_table));
    nsr(ReturnData_create_as_root(B, retval, output));

    flat_buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    rv = nng_send(sock, flat_buffer, size, 0);
    if (rv == 0)
    {
        //printf("return_regular_int32 - nng_send: %d\n", rv);
    }
    else
    {
        // TODO: Error event for NNG issues
        printf("return_regular_uint32 - Oh No! nng_send: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    }

    /* free(buffer); */
    flatcc_builder_aligned_free(flat_buffer);
    /*
     * Reset, but keep allocated stack etc.,
     * or optionally reduce memory using `flatcc_builder_custom_reset`.
     */
    flatcc_builder_reset(B);
}

void return_regular_int16(int16 call_return)
{
    size_t size;
    int rv;
    void *flat_buffer;
    // Send the return value
    flatcc_builder_t *B = &builder;

    nsr(Empty_ref_t) empty = nsr(Empty_create(B));
    nsr(PointerReturn_union_ref_t) output = nsr(PointerReturn_as_Empty(empty));
    nsr(Integer16_ref_t) call_return_table = nsr(Integer16_create(B, call_return));
    nsr(FuncReturn_union_ref_t) retval = nsr(FuncReturn_as_Integer16(call_return_table));
    nsr(ReturnData_create_as_root(B, retval, output));

    flat_buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    rv = nng_send(sock, flat_buffer, size, 0);
    if (rv == 0)
    {
        //printf("return_regular_int32 - nng_send: %d\n", rv);
    }
    else
    {
        // TODO: Error event for NNG issues
        printf("return_regular_int16 - Oh No! nng_send: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    }

    /* free(buffer); */
    flatcc_builder_aligned_free(flat_buffer);
    /*
     * Reset, but keep allocated stack etc.,
     * or optionally reduce memory using `flatcc_builder_custom_reset`.
     */
    flatcc_builder_reset(B);
}

void return_regular_uint16(uint16 call_return)
{
    size_t size;
    int rv;
    void *flat_buffer;
    // Send the return value
    flatcc_builder_t *B = &builder;

    nsr(Empty_ref_t) empty = nsr(Empty_create(B));
    nsr(PointerReturn_union_ref_t) output = nsr(PointerReturn_as_Empty(empty));
    nsr(UnInteger16_ref_t) call_return_table = nsr(UnInteger16_create(B, call_return));
    nsr(FuncReturn_union_ref_t) retval = nsr(FuncReturn_as_UnInteger16(call_return_table));
    nsr(ReturnData_create_as_root(B, retval, output));

    flat_buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    rv = nng_send(sock, flat_buffer, size, 0);
    if (rv == 0)
    {
        //printf("return_regular_int32 - nng_send: %d\n", rv);
    }
    else
    {
        // TODO: Error event for NNG issues
        printf("return_regular_uint16 - Oh No! nng_send: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    }

    /* free(buffer); */
    flatcc_builder_aligned_free(flat_buffer);
    /*
     * Reset, but keep allocated stack etc.,
     * or optionally reduce memory using `flatcc_builder_custom_reset`.
     */
    flatcc_builder_reset(B);
}

void return_regular_cFETime(CFE_TIME_SysTime_t time)
{
    size_t size;
    int rv;
    void *flat_buffer;
    // Send the return value
    flatcc_builder_t *B = &builder;

    nsr(Empty_ref_t) empty = nsr(Empty_create(B));
    nsr(PointerReturn_union_ref_t) output = nsr(PointerReturn_as_Empty(empty));
    cFETime_ref_t cFETime = cFETime_create(B, time.Seconds, time.Subseconds);
    nsr(FuncReturn_union_ref_t) retval = nsr(FuncReturn_as_cFETime(cFETime));
    nsr(ReturnData_create_as_root(B, retval, output));

    flat_buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    rv = nng_send(sock, flat_buffer, size, 0);
    if (rv == 0)
    {
        //printf("return_regular_int32 - nng_send: %d\n", rv);
    }
    else
    {
        // TODO: Error event for NNG issues
        printf("return_regular_cFETime - Oh No! nng_send: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    }

    /* free(buffer); */
    flatcc_builder_aligned_free(flat_buffer);
    /*
     * Reset, but keep allocated stack etc.,
     * or optionally reduce memory using `flatcc_builder_custom_reset`.
     */
    flatcc_builder_reset(B);
}

void incoming_message(void)
{
    int rv, index;
    char *buffer = NULL;
    size_t sz;
    int32 call_return;

    rv = nng_recv(sock, &buffer, &sz, NNG_FLAG_ALLOC);
    if (rv == 0)
    {
        PROXY_HkTelemetryPkt.actual_func_calls++;
        PROXY_HkTelemetryPkt.actual_run_state = ACTUAL_STATE_RUNNING;

        ns(RemoteCall_table_t) remoteCall = ns(RemoteCall_as_root(buffer));
        switch(ns(RemoteCall_input_type(remoteCall)))
        {
            // ES Functions
            case ns(Function_RunLoop):
            {
                if (VERBOSE) {printf("RunLoop called.\n");}

                // I don't know why the RunLoop status is call ExitStatus, and I don't know
                // why it gets passed as a pointer. It's not used like a pointer...
                ns(RunLoop_table_t) runLoop = (ns(RunLoop_table_t)) ns(RemoteCall_input(remoteCall));
                uint32_t ExitStatus = ns(RunLoop_ExitStatus(runLoop));
                call_return = CFE_ES_RunLoop(&ExitStatus);

                return_regular_int32(call_return);
                break;
            }
            case ns(Function_PerfLogAdd):
            {
                if (VERBOSE) {printf("PerfLogAdd called.\n");}

                ns(PerfLogAdd_table_t) perfLogAdd = (ns(PerfLogAdd_table_t)) ns(RemoteCall_input(remoteCall));
                uint32_t Marker = ns(PerfLogAdd_Marker(perfLogAdd));
                uint32_t EntryExit = ns(PerfLogAdd_EntryExit(perfLogAdd));
                CFE_ES_PerfLogAdd(Marker, EntryExit);

                // Void return
                break;
            }
            case ns(Function_RegisterApp):
            {
                if (VERBOSE) {printf("RegisterApp called.\n");}

                // This shouldn't happen: the actual app's es wrapper noops. The proxy registers.
                printf("Error: Actual app attempted to registers with ES\n");

                return_regular_int32(0);
                break;
            }
            case ns(Function_ExitApp):
            {
                if (VERBOSE) {printf("ExitApp called.\n");}

                ns(ExitApp_table_t) exitApp = (ns(ExitApp_table_t)) ns(RemoteCall_input(remoteCall));

                uint32 ExitStatus = ns(ExitApp_ExitStatus(exitApp));

                // TODO: Err... no. Not how this should go down...
                // The actual app should exit... and clean up its resources (done in proxy client es wrap)

                // Less sure about what happens to PROXY
                // send a EVS message? Then exit itself? Or stay alive? Send one last HK?
                PROXY_HkTelemetryPkt.actual_run_state = ACTUAL_STATE_EXITED;
                cleanup_and_exit(ExitStatus);

                // Void return
                break;
            }

            // EVS Functions
            case ns(Function_SendEvent):
            {
                if (VERBOSE) {printf("SendEvent called!!!\n");}

                ns(SendEvent_table_t) sendEvent = (ns(SendEvent_table_t)) ns(RemoteCall_input(remoteCall));
                uint16_t EventID = ns(SendEvent_EventID(sendEvent));
                uint16_t EventType = ns(SendEvent_EventType(sendEvent));
                const char *spec_string = ns(SendEvent_Spec(sendEvent));
                // printf("RECEIVED \"%s\"\n", spec_string);

                call_return = CFE_EVS_SendEvent(EventID, EventType, spec_string);
                return_regular_int32(call_return);
                break;
            }
            case ns(Function_SendEventWithAppID):
            {
                if (VERBOSE) {printf("SendEventWithAppID called !!!\n");}

                ns(SendEventWithAppID_table_t) sendEvent = (ns(SendEventWithAppID_table_t)) ns(RemoteCall_input(remoteCall));
                uint16_t EventID = ns(SendEventWithAppID_EventID(sendEvent));
                uint16_t EventType = ns(SendEventWithAppID_EventType(sendEvent));
                uint32_t AppID = ns(SendEventWithAppID_AppID(sendEvent));
                const char *spec_string = ns(SendEventWithAppID_Spec(sendEvent));
                // printf("RECEIVED \"%s\"\n", spec_string);

                call_return = CFE_EVS_SendEventWithAppID(EventID, EventType, AppID, spec_string);
                return_regular_int32(call_return);
                break;
            }
            case ns(Function_SendTimedEvent):
            {
                if (VERBOSE) {printf("SendTimedEvent called\n");}

                ns(SendTimedEvent_table_t) sendTimedEvent = (ns(SendTimedEvent_table_t)) ns(RemoteCall_input(remoteCall));
                cFETime_table_t time = ns(SendTimedEvent_Time(sendTimedEvent));
                CFE_TIME_SysTime_t cfe_time;
                cfe_time.Seconds = cFETime_Seconds(time);
                cfe_time.Subseconds = cFETime_Subseconds(time);
                uint16_t EventID = ns(SendTimedEvent_EventID(sendTimedEvent));
                uint16_t EventType = ns(SendTimedEvent_EventType(sendTimedEvent));

                const char *spec_string = ns(SendTimedEvent_Spec(sendTimedEvent));
                // printf("RECEIVED \"%s\"\n", spec_string);

                call_return = CFE_EVS_SendTimedEvent(cfe_time, EventID, EventType, spec_string);
                return_regular_int32(call_return);
                break;
            }
            case ns(Function_Register):
            {
                if (VERBOSE) {printf("EVS Register called\n");}

                ns(Register_table_t) registerEvents = (ns(Register_table_t)) ns(RemoteCall_input(remoteCall));
                uint16 NumFilteredEvents = ns(Register_NumFilteredEvents(registerEvents));
                uint16 FilterScheme = ns(Register_FilterScheme(registerEvents));

                ns(Filter_vec_t) filters = ns(Register_Filters(registerEvents));
                size_t filter_len = ns(Filter_vec_len(filters));

                CFE_EVS_BinFilter_t *new_filters;
                // printf("Allocating filters! %d (should equal %d)\n", filter_len, NumFilteredEvents);
                new_filters = malloc(filter_len * sizeof(CFE_EVS_BinFilter_t));

                for (index = 0; index < filter_len; index++)
                {
                    new_filters[index].EventID = ns(Filter_EventID(ns(Filter_vec_at(filters, index))));
                    new_filters[index].Mask = ns(Filter_Mask(ns(Filter_vec_at(filters, index))));
                }

                call_return = CFE_EVS_Register(new_filters, NumFilteredEvents, FilterScheme);

                return_regular_int32(call_return);

                free(new_filters);

                break;
            }
            case ns(Function_Unregister):
            {
                if (VERBOSE) {printf("Unregsiter called\n");}

                call_return = CFE_EVS_Unregister();

                return_regular_int32(call_return);

                break;
            }
            case ns(Function_ResetFilter):
            {
                if (VERBOSE) {printf("Reset Filter called\n");}

                ns(ResetFilter_table_t) resetFilter = (ns(ResetFilter_table_t)) ns(RemoteCall_input(remoteCall));
                uint16 EventID = ns(ResetFilter_EventID(resetFilter));

                call_return = CFE_EVS_ResetFilter(EventID);

                return_regular_int32(call_return);

                break;
            }
            case ns(Function_ResetAllFilters):
            {
                if (VERBOSE) {printf("ResetAllFilters called\n");}

                call_return = CFE_EVS_ResetAllFilters();

                return_regular_int32(call_return);

                break;
            }

            // TIME Functions
            case ns(Function_TIME_GetTime):
            {
                if (VERBOSE) {printf("Function_TIME_GetTime called\n");}

                return_regular_cFETime(CFE_TIME_GetTime());

                break;
            }
            case ns(Function_TIME_GetTAI):
            {
                if (VERBOSE) {printf("Function_TIME_GetTAI called\n");}

                return_regular_cFETime(CFE_TIME_GetTAI());

                break;
            }
            case ns(Function_TIME_GetUTC):
            {
                if (VERBOSE) {printf("Function_TIME_GetUTC called\n");}

                return_regular_cFETime(CFE_TIME_GetUTC());

                break;
            }
            case ns(Function_TIME_MET2SCTime):
            {
                if (VERBOSE) {printf("Function_TIME_MET2SCTime called\n");}

                ns(TIME_MET2SCTime_table_t) function_table = (ns(TIME_MET2SCTime_table_t)) ns(RemoteCall_input(remoteCall));
                cFETime_table_t time = ns(TIME_MET2SCTime_METTime(function_table));
                CFE_TIME_SysTime_t cfe_time;
                cfe_time.Seconds = cFETime_Seconds(time);
                cfe_time.Subseconds = cFETime_Subseconds(time);

                return_regular_cFETime(CFE_TIME_MET2SCTime(cfe_time));

                break;
            }
            case ns(Function_TIME_GetSTCF):
            {
                if (VERBOSE) {printf("Function_TIME_GetSTCF called\n");}

                return_regular_cFETime(CFE_TIME_GetSTCF());

                break;
            }
            case ns(Function_TIME_GetMET):
            {
                if (VERBOSE) {printf("Function_TIME_GetMET called\n");}

                return_regular_cFETime(CFE_TIME_GetMET());

                break;
            }
            case ns(Function_TIME_GetMETseconds):
            {
                if (VERBOSE) {printf("Function_TIME_GetMETseconds called\n");}

                return_regular_uint32(CFE_TIME_GetMETseconds());

                break;
            }
            case ns(Function_TIME_GetMETsubsecs):
            {
                if (VERBOSE) {printf("Function_TIME_GetMETsubsecs called\n");}

                return_regular_uint32(CFE_TIME_GetMETsubsecs());

                break;
            }
            case ns(Function_TIME_GetLeapSeconds):
            {
                if (VERBOSE) {printf("Function_TIME_GetLeapSeconds called\n");}

                return_regular_int16(CFE_TIME_GetLeapSeconds());

                break;
            }
            case ns(Function_TIME_GetClockState):
            {
                if (VERBOSE) {printf("Function_TIME_GetClockState called\n");}

                return_regular_int16(CFE_TIME_GetClockState());

                break;
            }
            case ns(Function_TIME_GetClockInfo):
            {
                if (VERBOSE) {printf("Function_TIME_GetClockInfo called\n");}

                return_regular_uint16(CFE_TIME_GetClockInfo());

                break;
            }

            default:
                // TODO: Event for unknown function
                printf("ERROR: What function is that supposed to be?: %d\n", ns(RemoteCall_input_type(remoteCall)));
        }

        nng_free(buffer, sz);
    }
    else if (rv == NNG_ETIMEDOUT)
    {
        // TODO: longer timeout check and set HK as needed
        // printf("Timed out waiting for a message from Actual\n");
    }
    else
    {
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;

        // TODO: event for nng issue
        printf("ERROR: nng_recv failed: %d\n", rv);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* PROXY_Init() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void PROXY_Init(void)
{
    /*
    ** Register the app with Executive services
    ** The actual app noops on its ES_RegisterApp call
    */
    CFE_ES_RegisterApp();

    /*
    ** Register the events -
    ** The actual app will register with EVS. Proxy uses PEVS to send events.
    */

    /*
    ** Create the Software Bus command pipe and subscribe to housekeeping
    **  messages
    */
    CFE_SB_CreatePipe(&PROXY_CommandPipe, PROXY_PIPE_DEPTH, "PROXY_CMD_PIPE");
    CFE_SB_Subscribe(PROXY_CMD_MID, PROXY_CommandPipe);
    CFE_SB_Subscribe(PROXY_SEND_HK_MID, PROXY_CommandPipe);

    CFE_SB_InitMsg(&PROXY_HkTelemetryPkt, PROXY_HK_TLM_MID, PROXY_HK_TLM_LNGTH, TRUE);

    PROXY_HkTelemetryPkt.actual_run_state = ACTUAL_STATE_UNKOWN;

    sleep(1); // Give PEVS a change to start up

    PROXY_HkTelemetryPkt.proxy_pevs_access = CFE_ES_GetAppIDByName(&proxy_evs_id, "PEVS");
    printf("PROXY Attempts to find PEVS: 0x%04X - %d\n", PROXY_HkTelemetryPkt.proxy_pevs_access, proxy_evs_id);
    if (PROXY_HkTelemetryPkt.proxy_pevs_access != CFE_SUCCESS)
    {
        // Can't exactly send a EVS message if this doesn't work. Not sure what else to do.
        printf("PROXY failed to find PEVS\n");
    }

    PROXY_ResetCounters();

    // Fork / Exec the actual process
    // TODO: Event for fork / exec issues

    childPID = fork();
    if (childPID >= 0)
    { // Forked
        if (childPID == 0)
        { // Child process
            // printf("The child has forked %d\n", getpid());
            if (-1 == execlp(EXEC_INSTRUCTION, EXEC_ARGUMENTS, NULL))
            {
                // I don't know how to indicate that this has happened.
                // A child process can not write to the parent's memory (so HK telemetry won't work)
                // It can read the parent's data, such as proxy_evs_access, but can't call the the correct CFE_EVS_SendEventWithAppID
                perror("Exec error!!!\n");
                exit(0); // The child must exit
            }
        }
    }
    else
    {
        PROXY_HkTelemetryPkt.proxy_fork_error = errno;
        printf("Fork error\n");
    }

    // This code block is handy if the child process fails after the exec succeeds.
    // Just waits on the process, prints the reason is died.
    // Signal 11 (Seg Fault) may indicate you need to raise Proxy's stack size in the startup script
    /*
    int wstatus = 0;
    int w = waitpid(childPID, &wstatus, WUNTRACED | WCONTINUED);
    if (w == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
    if (WIFEXITED(wstatus)) {
        printf("exited, status=%d\n", WEXITSTATUS(wstatus));
    } else if (WIFSIGNALED(wstatus)) {
        printf("killed by signal %d\n", WTERMSIG(wstatus));
    } else if (WIFSTOPPED(wstatus)) {
        printf("stopped by signal %d\n", WSTOPSIG(wstatus));
    } else if (WIFCONTINUED(wstatus)) {
        printf("continued\n");
    } */

    // Flat Buff init
    flatcc_builder_init(&builder);

    // Wait for connection from actual application
    int rv;

    // TODO: Event for NNG issues / error checking
    if ((rv = nng_pair0_open(&sock)) != 0)
    {
        printf("nng_pair0_open: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    }
    // Listen doesn't timeout waiting for a connection.
    if ((rv = nng_listen(sock, IPC_PIPE_ADDRESS, NULL, 0)) !=0)
    {
        printf("nng_listen: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    } else {
        printf("(PROXY) listening on %s\n", IPC_PIPE_ADDRESS);
    }
    if ((rv = nng_setopt_ms(sock, NNG_OPT_RECVTIMEO, ACTUAL_NNG_TIMEOUT)) != 0)
    {
        printf("nng_setopt_ms: %d\n", rv);
        PROXY_HkTelemetryPkt.proxy_nng_error = rv;
    }

    CFE_EVS_SendEventWithAppID(PROXY_STARTUP_INF_EID, CFE_EVS_INFORMATION, proxy_evs_id,
                               "Pro Proxy Initialized. Version %d.%d.%d.%d",
                               PROXY_MAJOR_VERSION,
                               PROXY_MINOR_VERSION,
                               PROXY_REVISION,
                               PROXY_MISSION_REV);
} /* End of PROXY_Init() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  PROXY_ProcessCommandPacket                                         */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the PROXY     */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void PROXY_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t  MsgId;

    MsgId = CFE_SB_GetMsgId(PROXY_MsgPtr);

    switch (MsgId)
    {
        case PROXY_CMD_MID:
            PROXY_ProcessGroundCommand();
            break;

        case PROXY_SEND_HK_MID:
            PROXY_ReportHousekeeping();
            break;

        default:
            PROXY_HkTelemetryPkt.proxy_command_error_count++;
            CFE_EVS_SendEventWithAppID(PROXY_COMMAND_ERR_EID,CFE_EVS_ERROR, proxy_evs_id,
            "PROXY: invalid command packet, MID = 0x%x", MsgId);
            break;
    }

    return;

} /* End PROXY_ProcessCommandPacket */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* PROXY_ProcessGroundCommand() -- PROXY ground commands                      */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void PROXY_ProcessGroundCommand(void)
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode(PROXY_MsgPtr);

    /* Process "known" PROXY ground commands */
    switch (CommandCode)
    {
        case PROXY_NOOP_CC:
            PROXY_HkTelemetryPkt.proxy_command_count++;
            CFE_EVS_SendEventWithAppID(PROXY_COMMANDNOP_INF_EID, CFE_EVS_INFORMATION, proxy_evs_id,
            "PROXY: NOOP command");
            break;

        case PROXY_RESET_COUNTERS_CC:
            PROXY_ResetCounters();
            break;

        /* default case already found during FC vs length test */
        default:
            break;
    }
    return;

} /* End of PROXY_ProcessGroundCommand() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  PROXY_ReportHousekeeping                                           */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void PROXY_ReportHousekeeping(void)
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &PROXY_HkTelemetryPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &PROXY_HkTelemetryPkt);
    return;
} /* End of PROXY_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  PROXY_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void PROXY_ResetCounters(void)
{
    /* Status of commands processed by the Proxy */
    PROXY_HkTelemetryPkt.proxy_command_count       = 0;
    PROXY_HkTelemetryPkt.proxy_command_error_count = 0;

    CFE_EVS_SendEventWithAppID(PROXY_COMMANDRST_INF_EID, CFE_EVS_INFORMATION, proxy_evs_id,
                      "PROXY: RESET command");
    return;
} /* End of PROXY_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* PROXY_VerifyCmdLength() -- Verify command packet length                    */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
boolean PROXY_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength)
{
    boolean result = TRUE;

    uint16 ActualLength = CFE_SB_GetTotalMsgLength(msg);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_SB_MsgId_t MessageID   = CFE_SB_GetMsgId(msg);
        uint16         CommandCode = CFE_SB_GetCmdCode(msg);

        CFE_EVS_SendEventWithAppID(PROXY_LEN_ERR_EID, CFE_EVS_ERROR, proxy_evs_id,
                          "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
                          MessageID, CommandCode, ActualLength, ExpectedLength);
        result = FALSE;
        PROXY_HkTelemetryPkt.proxy_command_error_count++;
    }

    return(result);

} /* End of PROXY_VerifyCmdLength() */
