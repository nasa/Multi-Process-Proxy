/*******************************************************************************
** File: sample_pro.h
**
** Purpose:
**   This file is main hdr file for the SAMPLE process application.
**
**
*******************************************************************************/

#ifndef _proxy_h_
#define _proxy_h_

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
boolean PROXY_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

#endif /* _proxy_h_ */
