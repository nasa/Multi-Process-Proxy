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

#ifndef proxy_msg_h
#define proxy_msg_h

/*
** Proxy command codes
*/
#define PROXY_NOOP_CC                 0
#define PROXY_RESET_COUNTERS_CC       1

/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

} PROXY_NoArgsCmd_t;

// TODO: Command to send HK? How does the proxy recieve commands to start with?

/*************************************************************************/
/*
** Type definition (Proxy housekeeping)
*/
typedef struct
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint8              proxy_command_error_count;
    uint8              proxy_command_count;

    // Data about the proxy
    int32              proxy_pevs_access;    // retrun code from CFE_ES_GetAppIDByName
    int32              proxy_fork_error;     // errno after failed fork()
    int32              proxy_nng_error;      // return code from nng library call

    // Data about the actual application
    int32              actual_run_state;
    int32              actual_registered;
    uint32             actual_func_calls;
    uint32             actual_reset_count;
    uint32             actual_ms_last_msg;
}   OS_PACK proxy_hk_tlm_t  ;

#define PROXY_HK_TLM_LNGTH   sizeof ( proxy_hk_tlm_t )

#endif /* proxy_msg_h */
