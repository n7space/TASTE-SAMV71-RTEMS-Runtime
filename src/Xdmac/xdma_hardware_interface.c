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

/** \addtogroup dmad_module
 *
 * \section DmaHw Dma Hardware Interface Usage
 * <ul>
 * <li> The DMA controller can handle the transfer between peripherals and memory
 * and so receives the triggers from the peripherals. The hardware interface
 * number are getting from DMAIF_Get_ChannelNumber().</li>

 * <li> DMAIF_IsValidatedPeripherOnDma() helps to check if the given DMAC has
 * associated peripheral identifier coded by the given  peripheral.</li>
 *
 * </ul>
 */
/*@{*/
/*@}*/

/** \file */
/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/
#include "xdma_hardware_interface.h"
#include "xdmad.h"
#include <Pmc/Pmc.h>
/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/
/** Array of DMA Channel definition for SAMv7 chip*/
static const XdmaHardwareInterface xdmaHwIf[] = {
    /* xdmac, peripheral,  T/R, HW interface number*/
    { 0, Pmc_PeripheralId_Hsmci, 0, 0 },   { 0, Pmc_PeripheralId_Hsmci, 1, 0 },   { 0, Pmc_PeripheralId_Spi0, 0, 1 },
    { 0, Pmc_PeripheralId_Spi0, 1, 2 },    { 0, Pmc_PeripheralId_Spi1, 0, 3 },    { 0, Pmc_PeripheralId_Spi1, 1, 4 },
    { 0, Pmc_PeripheralId_Qspi, 0, 5 },    { 0, Pmc_PeripheralId_Qspi, 1, 6 },    { 0, Pmc_PeripheralId_Usart0, 0, 7 },
    { 0, Pmc_PeripheralId_Usart0, 1, 8 },  { 0, Pmc_PeripheralId_Usart1, 0, 9 },  { 0, Pmc_PeripheralId_Usart1, 1, 10 },
    { 0, Pmc_PeripheralId_Usart2, 0, 11 }, { 0, Pmc_PeripheralId_Usart2, 1, 12 }, { 0, Pmc_PeripheralId_Pwm0, 0, 13 },
    { 0, Pmc_PeripheralId_Twihs0, 0, 14 }, { 0, Pmc_PeripheralId_Twihs0, 1, 15 }, { 0, Pmc_PeripheralId_Twihs1, 0, 16 },
    { 0, Pmc_PeripheralId_Twihs1, 1, 17 }, { 0, Pmc_PeripheralId_Twihs2, 0, 18 }, { 0, Pmc_PeripheralId_Twihs2, 1, 19 },
    { 0, Pmc_PeripheralId_Uart0, 0, 20 },  { 0, Pmc_PeripheralId_Uart0, 1, 21 },  { 0, Pmc_PeripheralId_Uart1, 0, 22 },
    { 0, Pmc_PeripheralId_Uart1, 1, 23 },  { 0, Pmc_PeripheralId_Uart2, 0, 24 },  { 0, Pmc_PeripheralId_Uart2, 1, 25 },
    { 0, Pmc_PeripheralId_Uart3, 0, 26 },  { 0, Pmc_PeripheralId_Uart3, 1, 27 },  { 0, Pmc_PeripheralId_Uart4, 0, 28 },
    { 0, Pmc_PeripheralId_Uart4, 1, 29 },  { 0, Pmc_PeripheralId_Dacc, 0, 30 },   { 0, Pmc_PeripheralId_Dacc, 0, 31 },
    { 0, Pmc_PeripheralId_Ssc, 0, 32 },    { 0, Pmc_PeripheralId_Ssc, 1, 33 },    { 0, Pmc_PeripheralId_PioA, 1, 34 },
    { 0, Pmc_PeripheralId_Afec0, 1, 35 },  { 0, Pmc_PeripheralId_Afec1, 1, 36 },  { 0, Pmc_PeripheralId_Aes, 0, 37 },
    { 0, Pmc_PeripheralId_Aes, 1, 38 },    { 0, Pmc_PeripheralId_Pwm1, 0, 39 },   { 0, Pmc_PeripheralId_Tc0Ch0, 1, 40 },
    { 0, Pmc_PeripheralId_Tc0Ch1, 1, 41 }, { 0, Pmc_PeripheralId_Tc0Ch2, 1, 42 }, { 0, Pmc_PeripheralId_Tc1Ch0, 1, 43 },
};

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Get peripheral identifier coded for hardware handshaking interface
 *
 * \param bDmac      DMA Controller number.
 * \param bPeriphID  Peripheral ID.
 * \param bTransfer  Transfer type 0: Tx, 1 :Rx.
 * \return 0-15 peripheral identifier coded.
 *         0xff : no associated peripheral identifier coded.
 */
uint8_t
XDMAIF_Get_ChannelNumber(uint8_t bPeriphID, uint8_t bTransfer)
{
    uint8_t i;
    uint8_t NumOfPeripheral = ((XDMAC_GetType(XDMAC) & 0x00FF0000) >> 16);
    for(i = 0; i <= NumOfPeripheral; i++) {
        if((xdmaHwIf[i].bPeriphID == bPeriphID) && (xdmaHwIf[i].bTransfer == bTransfer)) {
            return xdmaHwIf[i].bIfID;
        }
    }
    return 0xff;
}

/**
 * \brief Check if the given DMAC has associated peripheral identifier coded by
 * the given  peripheral.
 *
 * \param bDmac      DMA Controller number.
 * \param bPeriphID  Peripheral ID (0xff : memory only).
 * \return 1:  Is a validated peripheral. 0: no associated peripheral identifier
 * coded.
 */
uint8_t
XDMAIF_IsValidatedPeripherOnDma(uint8_t bPeriphID)
{
    uint8_t i;
    uint8_t NumOfPeripheral = ((XDMAC_GetType(XDMAC) & 0x00FF0000) >> 16);
    /* It is always validated when transfer to memory */
    if(bPeriphID == 0xFF) {
        return 1;
    }

    for(i = 0; i <= NumOfPeripheral; i++) {
        if((xdmaHwIf[i].bPeriphID == bPeriphID)) {
            return 1;
        }
    }
    return 0;
}
