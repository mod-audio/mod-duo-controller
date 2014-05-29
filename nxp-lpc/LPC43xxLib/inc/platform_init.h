/**********************************************************************
* $Id$      platform_init.h         2012-03-16
*//**
* @file     platform_init.h
* @brief    LPC43xx Dual Core Interrupt configuration 
* @version  1.0
* @date     03. March. 2012
* @author   NXP MCU SW Application Team
*
* Copyright(C) 2012, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
**********************************************************************/
#ifndef __PLATFORM_CONFIG_H 
#define __PLATFORM_CONFIG_H

#include "stdint.h"
//#include "project_init.h"

/****************************************************/
/* USER CONFIGURATION SECTION                       */
/****************************************************/

/* choose the device you want to build against  */
#define DEVICE  LPC43xx

/* define which boot mode is used */
#define MASTER_BOOT_MODE    (OTHER_BOOT)

/* specify the filename used for the slave image */
#define SLAVE_IMAGE_FILE "CM0_image.c"

/* specify the maximum number of items which the buffer can hold before getting full */
/* messages might use 1 or multiple items in case of additional parameters */
#define MASTER_CMDBUF_SIZE      (10)

/* configure which priority the IPC interrupt should have on the host (M4) side */
/* uses cmsis definition, priority from 0 to 7  */
#define MASTER_QUEUE_PRIORITY       0//((0x01<<3)|0x01)

/* specify the maximum number of items which the buffer can hold before getting full */
/* messages might use 1 or multiple items in case of additional parameters */
#define SLAVE_MSGBUF_SIZE       (10)

/* configure which priority the IPC interrupt should have on the slave (M0) side */
/* uses cmsis definition, priority from 0 to 7 */
#define SLAVE_QUEUE_PRIORITY        0//((0x01<<3)|0x01)

/* these addresses specify where the IPC queues shall be located */
#define MASTER_CMD_BLOCK_START  0x20008000
#define SLAVE_MSG_BLOCK_START   0x2000A000

/* configure which priority the mailbox interrupt should have on the M4 side */
/* cmsis definition, priority from 0 to 7 */
#define MASTER_MAILBOX_PRIORITY (0)

/* configure which priority the mailbox interrupt should have on the M0 side */
/* cmsis definition, priority from 0 to 3 */
#define SLAVE_MAILBOX_PRIORITY  (0)

#define MASTER_INTERRUPT_PRIORITY (0)
#define SLAVE_INTERRUPT_PRIORITY  (0)

//#define IPC_MASTER
#define IPC_SLAVE


/****************************************************/
/* END OF USER CONFIGURATION                        */
/* DO NOT EDIT BELOW THIS LINE                      */
/****************************************************/


/* assign the roles for the devices */
#if (DEVICE==LPC43xx)

#include "LPC43xx.h"

/* these definitions are used in case the interrupt vector table */
/* gets relocated into internal ram */
/* total of system + user defined interrupts, 68 vectors in total */
#define LPC4300_SYSTEM_INTERRUPTS   (16)
#define LPC4300_USER_INTERRUPTS     (53)
#define LPC4300_TOTAL_INTERRUPTS    (LPC4300_SYSTEM_INTERRUPTS + LPC4300_USER_INTERRUPTS)

#define MASTER_CPU  CORE_M4
#define SLAVE_CPU   CORE_M0

#define MASTER_IRQn (M0CORE_IRQn)
#define SLAVE_IRQn  (M0_M4CORE_IRQn)

#define MASTER_TXEV_QUIT()  { LPC_CREG->M4TXEVENT = 0x0; }
#define SLAVE_TXEV_QUIT()   { LPC_CREG->M0TXEVENT = 0x0; }

#define SET_SLAVE_SHADOWREG(n) {LPC_CREG->M0APPMEMMAP = n; }

#define MASTER_IPC_TABLE    MASTER_MBX_START
#define SLAVE_IPC_TABLE     SLAVE_MBX_START

#endif

/****************************************************/
/* platform wise initialization function            */
/****************************************************/
void platformInit(void);



#endif /* __PLATFORM_CONFIG_H */
