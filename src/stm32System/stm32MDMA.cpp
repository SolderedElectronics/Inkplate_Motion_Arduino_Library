// Include main header file of this library.
#include "stm32MDMA.h"

void stm32MDMAConfigure(MDMA_HandleTypeDef *_mdma, uint32_t _sourceAddress, uint32_t _destinationAddress, uint32_t _blockSize)
{
    /* Change MDMA peripheral state */
    _mdma->State = HAL_MDMA_STATE_BUSY;

    /* Initialize the error code */
    _mdma->ErrorCode = HAL_MDMA_ERROR_NONE;

    /* Disable the peripheral */
    __HAL_MDMA_DISABLE(_mdma);

    /* Configure the MDMA Channel data length */
    MODIFY_REG(_mdma->Instance->CBNDTR, MDMA_CBNDTR_BNDT, (_blockSize & MDMA_CBNDTR_BNDT));

    /* Configure the MDMA block repeat count */
    MODIFY_REG(_mdma->Instance->CBNDTR , MDMA_CBNDTR_BRC, ((1 - 1U) << MDMA_CBNDTR_BRC_Pos) & MDMA_CBNDTR_BRC);

    /* Clear all interrupt flags */
    __HAL_MDMA_CLEAR_FLAG(_mdma, MDMA_FLAG_TE | MDMA_FLAG_CTC | MDMA_CISR_BRTIF | MDMA_CISR_BTIF | MDMA_CISR_TCIF);

    /* Configure MDMA Channel destination address */
    _mdma->Instance->CDAR = (uint32_t)_destinationAddress;

    /* Configure MDMA Channel Source address */
    _mdma->Instance->CSAR = (uint32_t)_sourceAddress;

    uint32_t addressMask = (uint32_t)_sourceAddress & 0xFF000000U;
    if((addressMask == 0x20000000U) || (addressMask == 0x00000000U))
    {
        /*The AHBSbus is used as source (read operation) on channel x */
        _mdma->Instance->CTBR |= MDMA_CTBR_SBUS;
    }
    else
    {
        /*The AXI bus is used as source (read operation) on channel x */
        _mdma->Instance->CTBR &= (~MDMA_CTBR_SBUS);
    }
    
    addressMask = _destinationAddress & 0xFF000000U;
    if((addressMask == 0x20000000U) || (addressMask == 0x00000000U))
    {
        /*The AHB bus is used as destination (write operation) on channel x */
        _mdma->Instance->CTBR |= MDMA_CTBR_DBUS;
    }
    else
    {
        /*The AXI bus is used as destination (write operation) on channel x */
        _mdma->Instance->CTBR &= (~MDMA_CTBR_DBUS);
    }

    /* Set the linked list register to the first node of the list */
    _mdma->Instance->CLAR = (uint32_t)_mdma->FirstLinkedListNodeAddress;

    __HAL_MDMA_ENABLE(_mdma);
}

void stm32MDMAStart(MDMA_HandleTypeDef *_mdma)
{
    _mdma->Instance->CCR |=  MDMA_CCR_SWRQ;
}

void stm32MDMAModifyAddress(MDMA_HandleTypeDef *_mdma, uint32_t _source, uint32_t _dest)
{
    __HAL_MDMA_DISABLE(_mdma);

    /* Configure MDMA Channel destination address */
    _mdma->Instance->CDAR = (uint32_t)_dest;

    /* Configure MDMA Channel Source address */
    _mdma->Instance->CSAR = (uint32_t)_source;

    __HAL_MDMA_ENABLE(_mdma);
}

bool stm32MDMATransferComplete(MDMA_HandleTypeDef *_mdma)
{
    return (__HAL_MDMA_GET_FLAG(_mdma, MDMA_FLAG_CTC));
}

bool stm32MDMAClearFlag(MDMA_HandleTypeDef *_mdma)
{
    __HAL_MDMA_CLEAR_FLAG(_mdma, (MDMA_FLAG_BFTC | MDMA_FLAG_BT));
}