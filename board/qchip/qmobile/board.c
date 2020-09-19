/*
 * (C) Copyright 2015 Linaro
 * Peter Griffin <peter.griffin@linaro.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <dm.h>
#include <dm/platform_data/serial_pl01x.h>
#include <errno.h>
#include <malloc.h>
#include <netdev.h>
#include <asm/io.h>
#include <usb.h>
#include <asm-generic/gpio.h>
#include <asm/arch/dwmmc.h>
#include <asm/arch/gpio.h>
#include <asm/arch/periph.h>
#include <asm/arch/pinmux.h>
#include <asm/armv8/mmu.h>

/*TODO drop this table in favour of device tree */

DECLARE_GLOBAL_DATA_PTR;

#if !CONFIG_IS_ENABLED(OF_CONTROL)

static const struct pl01x_serial_platdata serial_platdata = {
	.base = 0x12000000,
	.type = TYPE_PL011,
	.clock = 19200000
};

U_BOOT_DEVICE(qserver_seriala) = {
	.name = "serial_pl01x",
	.platdata = &serial_platdata,
};
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
	return 0;
}
#endif



#define EYE_PATTERN	0x70533483

int board_usb_init(int index, enum usb_init_type init)
{
	return 0;
}


int misc_init_r(void)
{
	return 0;
}

int board_init(void)
{
	return 0;
}

#ifdef CONFIG_MMC

static int init_dwmmc(void)
{
	return 0;
}

/* setup board specific PMIC */
int power_init_board(void)
{
	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int ret;

	/* add the eMMC and sd ports */
	ret = init_dwmmc();

	if (ret)
		debug("init_dwmmc failed\n");

	return ret;
}
#endif

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

int dram_init_banksize(void)
{
	/*
	 * Reserve regions below from DT memory node (which gets generated
	 * by U-Boot from the dram banks in arch_fixup_fdt() before booting
	 * the kernel. This will then match the kernel hikey dts memory node.
	 *
	 *  0x05e0,0000 - 0x05ef,ffff: MCU firmware runtime using
	 *  0x05f0,1000 - 0x05f0,1fff: Reboot reason
	 *  0x06df,f000 - 0x06df,ffff: Mailbox message data
	 *  0x0740,f000 - 0x0740,ffff: MCU firmware section
	 *  0x21f0,0000 - 0x21ff,ffff: pstore/ramoops buffer
	 *  0x3e00,0000 - 0x3fff,ffff: OP-TEE
	*/

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = CONFIG_SYS_SDRAM_SIZE;

	/*gd->bd->bi_dram[1].start = 0x05f00000;
	gd->bd->bi_dram[1].size = 0x00001000;

	gd->bd->bi_dram[2].start = 0x05f02000;
	gd->bd->bi_dram[2].size = 0x00efd000;

	gd->bd->bi_dram[3].start = 0x06e00000;
	gd->bd->bi_dram[3].size = 0x0060f000;

	gd->bd->bi_dram[4].start = 0x07410000;
	gd->bd->bi_dram[4].size = 0x1aaf0000;

	gd->bd->bi_dram[5].start = 0x22000000;
	gd->bd->bi_dram[5].size = 0x1c000000;*/

	return 0;
}

