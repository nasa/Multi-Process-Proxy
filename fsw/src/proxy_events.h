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

#ifndef proxy_events_h
#define proxy_events_h

#define PROXY_RESERVED_EID              0
#define PROXY_STARTUP_INF_EID           1
#define PROXY_COMMAND_ERR_EID           2
#define PROXY_COMMANDNOP_INF_EID        3
#define PROXY_COMMANDRST_INF_EID        4
#define PROXY_INVALID_MSGID_ERR_EID     5
#define PROXY_LEN_ERR_EID               6
#define PROXY_SHUTDOWN_INF_EID          7
#define PROXY_NNG_ERR_EID               8
#define PROXY_UNIMPLEMENTED_ERR_EID     9

#endif /* proxy_events_h */
