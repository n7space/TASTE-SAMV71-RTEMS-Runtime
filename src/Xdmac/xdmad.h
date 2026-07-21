/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2013, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

#ifndef _XDMAD_H
#define _XDMAD_H

/*----------------------------------------------------------------------------
 *        Includes
 *----------------------------------------------------------------------------*/

#include "xdmac.h"
#include <assert.h>

/** \addtogroup dmad_defines DMA Driver Defines
        @{*/
/*----------------------------------------------------------------------------
 *        Consts
 *----------------------------------------------------------------------------*/
#define XDMAD_TRANSFER_MEMORY 0xFF /**< DMA transfer from or to memory */
#define XDMAD_ALLOC_FAILED 0xFFFF /**< Channel allocate failed */

#define XDMAD_TRANSFER_TX 0
#define XDMAD_TRANSFER_RX 1

/* XDMA_MBR_UBC */
#define XDMA_UBC_NDE (0x1u << 24)
#define XDMA_UBC_NDE_FETCH_DIS (0x0u << 24)
#define XDMA_UBC_NDE_FETCH_EN (0x1u << 24)
#define XDMA_UBC_NSEN (0x1u << 25)
#define XDMA_UBC_NSEN_UNCHANGED (0x0u << 25)
#define XDMA_UBC_NSEN_UPDATED (0x1u << 25)
#define XDMA_UBC_NDEN (0x1u << 26)
#define XDMA_UBC_NDEN_UNCHANGED (0x0u << 26)
#define XDMA_UBC_NDEN_UPDATED (0x1u << 26)
#define XDMA_UBC_NVIEW_Pos 27
#define XDMA_UBC_NVIEW_Msk (0x3u << XDMA_UBC_NVIEW_Pos)
#define XDMA_UBC_NVIEW_NDV0 (0x0u << XDMA_UBC_NVIEW_Pos)
#define XDMA_UBC_NVIEW_NDV1 (0x1u << XDMA_UBC_NVIEW_Pos)
#define XDMA_UBC_NVIEW_NDV2 (0x2u << XDMA_UBC_NVIEW_Pos)
#define XDMA_UBC_NVIEW_NDV3 (0x3u << XDMA_UBC_NVIEW_Pos)

/*----------------------------------------------------------------------------
 *        MACRO
 *----------------------------------------------------------------------------*/

/**     @}*/

/*----------------------------------------------------------------------------
 *        Types
 *----------------------------------------------------------------------------*/
/** \addtogroup dmad_structs DMA Driver Structs
        @{*/

/** @brief DMA status or return code. */
typedef enum _XdmadStatus {
	XDMAD_OK = 0, /**< Operation is successful */
	XDMAD_PARTIAL_DONE,
	XDMAD_DONE,
	XDMAD_BUSY, /**< Channel occupied or transfer not finished */
	XDMAD_ERROR, /**< Operation failed */
	XDMAD_CANCELED /**< Operation cancelled */
} eXdmadStatus,
	eXdmadRC;

/** @brief Lifecycle state of one DMA channel. */
typedef enum _XdmadState {
	XDMAD_STATE_FREE = 0, /**< Free channel */
	XDMAD_STATE_ALLOCATED, /**< Allocated to some peripheral */
	XDMAD_STATE_START, /**< DMA started */
	XDMAD_STATE_IN_XFR, /**< DMA in transferring */
	XDMAD_STATE_DONE, /**< DMA transfer done */
	XDMAD_STATE_HALTED, /**< DMA transfer stopped */
} eXdmadState;

/** @brief Programming mode used for a DMA channel transfer. */
typedef enum _XdmadProgState {
	XDMAD_SINGLE = 0,
	XDMAD_MULTI,
	XDMAD_LLI,
} eXdmadProgState;

/** @brief Callback invoked when a DMA transfer completes or changes state. */
typedef void (*XdmadTransferCallback)(uint32_t iChannel, void *pArg);

/** @brief Per-channel bookkeeping used by the XDMAC driver instance. */
typedef struct _XdmadChannel {
	XdmadTransferCallback fCallback; /**< Completion callback. */
	void *pArg; /**< Optional callback argument. */
	uint8_t bIrqOwner; /**< True when the DMA handler owns the interrupt. */
	uint8_t bSrcPeriphID; /**< Source peripheral ID. */
	uint8_t bDstPeriphID; /**< Destination peripheral ID. */
	uint8_t bSrcTxIfID; /**< Source Tx handshake interface ID. */
	uint8_t bSrcRxIfID; /**< Source Rx handshake interface ID. */
	uint8_t bDstTxIfID; /**< Destination Tx handshake interface ID. */
	uint8_t bDstRxIfID; /**< Destination Rx handshake interface ID. */
	volatile uint8_t state; /**< Channel state. */
} sXdmadChannel;

/** @brief DMA driver instance that owns the controller and its channels. */
typedef struct _Xdmad {
	Xdmac *pXdmacs;
	sXdmadChannel XdmaChannels[XDMACCHID_NUMBER];
	uint8_t numControllers;
	uint8_t numChannels;
	uint8_t pollingMode;
	uint8_t pollingTimeout;
	uint8_t xdmaMutex;
} sXdmad;

typedef struct _XdmadCfg {
	/** Microblock control register value. */
	uint32_t mbr_ubc;
	/** Source address register value. */
	uint32_t mbr_sa;
	/** Destination address register value. */
	uint32_t mbr_da;
	/** Channel configuration register value. */
	uint32_t mbr_cfg;
	/** Block control register value. */
	uint32_t mbr_bc;
	/** Data stride register value. */
	uint32_t mbr_ds;
	/** Source microblock stride register value. */
	uint32_t mbr_sus;
	/** Destination microblock stride register value. */
	uint32_t mbr_dus;
} sXdmadCfg;

/**
 * @brief Linked-list descriptor format 0 used by XDMAC master transfers.
 */
typedef struct _LinkedListDescriptorView0 {
	/** Next descriptor address. */
	uint32_t mbr_nda;
	/** Microblock control value. */
	uint32_t mbr_ubc;
	/** Transfer address. */
	uint32_t mbr_ta;
} LinkedListDescriptorView0;

/**
 * @brief Linked-list descriptor format 1 used by XDMAC master transfers.
 */
typedef struct _LinkedListDescriptorView1 {
	/** Next descriptor address. */
	uint32_t mbr_nda;
	/** Microblock control value. */
	uint32_t mbr_ubc;
	/** Source address. */
	uint32_t mbr_sa;
	/** Destination address. */
	uint32_t mbr_da;
} LinkedListDescriptorView1;

/**
 * @brief Linked-list descriptor format 2 used by XDMAC master transfers.
 */
typedef struct _LinkedListDescriptorView2 {
	/** Next descriptor address. */
	uint32_t mbr_nda;
	/** Microblock control value. */
	uint32_t mbr_ubc;
	/** Source address. */
	uint32_t mbr_sa;
	/** Destination address. */
	uint32_t mbr_da;
	/** Channel configuration register value. */
	uint32_t mbr_cfg;
} LinkedListDescriptorView2;

/**
 * @brief Linked-list descriptor format 3 used by XDMAC master transfers.
 */
typedef struct _LinkedListDescriptorView3 {
	/** Next descriptor address. */
	uint32_t mbr_nda;
	/** Microblock control value. */
	uint32_t mbr_ubc;
	/** Source address. */
	uint32_t mbr_sa;
	/** Destination address. */
	uint32_t mbr_da;
	/** Channel configuration register value. */
	uint32_t mbr_cfg;
	/** Block control value. */
	uint32_t mbr_bc;
	/** Data stride value. */
	uint32_t mbr_ds;
	/** Source microblock stride value. */
	uint32_t mbr_sus;
	/** Destination microblock stride value. */
	uint32_t mbr_dus;
} LinkedListDescriptorView3;

/**     @}*/

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/
/** \addtogroup dmad_functions DMA Driver Functions
        @{*/
extern void XDMAD_Initialize(sXdmad *pXdmad, uint8_t bPollingMode);

extern void XDMAD_Handler(sXdmad *pDmad);

extern uint32_t XDMAD_AllocateChannel(sXdmad *pXdmad, uint8_t bSrcID,
				      uint8_t bDstID);
extern eXdmadRC XDMAD_FreeChannel(sXdmad *pXdmad, uint32_t dwChannel);

extern eXdmadRC XDMAD_ConfigureTransfer(sXdmad *pXdmad, uint32_t dwChannel,
					sXdmadCfg *pXdmaParam,
					uint32_t dwXdmaDescCfg,
					uint32_t dwXdmaDescAddr,
					uint32_t dwXdmaIntEn);

extern eXdmadRC XDMAD_PrepareChannel(sXdmad *pXdmad, uint32_t dwChannel);

extern eXdmadRC XDMAD_IsTransferDone(sXdmad *pXdmad, uint32_t dwChannel);

extern eXdmadRC XDMAD_StartTransfer(sXdmad *pXdmad, uint32_t dwChannel);

extern eXdmadRC XDMAD_SetCallback(sXdmad *pXdmad, uint32_t dwChannel,
				  XdmadTransferCallback fCallback, void *pArg);

extern eXdmadRC XDMAD_StopTransfer(sXdmad *pXdmad, uint32_t dwChannel);
/**     @}*/
/**@}*/
#endif //#ifndef _XDMAD_H
