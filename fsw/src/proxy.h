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

#ifndef proxy_h
#define proxy_h

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>

/***********************************************************************/

#define PROXY_PIPE_DEPTH                     32

/************************************************************************
** Type Definitions
*************************************************************************/

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (PROXY_Main), these
**       functions are not called from any other source module.
*/
void PROXY_Main(void);
void PROXY_Init(void);
void PROXY_ProcessCommandPacket(void);
void PROXY_ProcessGroundCommand(void);
void PROXY_ReportHousekeeping(void);
void PROXY_ResetCounters(void);

void cleanup_and_exit(uint32 RunStatus);

void incoming_message(void);
bool PROXY_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

#endif /* proxy_h */
