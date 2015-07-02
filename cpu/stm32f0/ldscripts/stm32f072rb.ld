/*
 * Copyright (C) 2015 Lari Lehtomäki
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @addtogroup      cpu_stm32f0
 * @{
 *
 * @file
 * @brief           Memory definitions for the STM32F072RB
 *
 * @author          Lari Lehtomäki <lari@lehtomaki.fi>
 *
 * @}
 */

MEMORY
{
    rom (rx)    : ORIGIN = 0x08000000, LENGTH = 128K
    ram (rwx)   : ORIGIN = 0x20000000, LENGTH = 16K
}

INCLUDE cortexm_base.ld
