/*
 * (C) Copyright 2017 
 *
 * Yuan Haibo <iambobor@qq.com>
 *
 * Configuration for qserver. Parts were derived from other ARM
 * configurations.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __QSERVER_H
#define __QSERVER_H

#include <linux/sizes.h>

#define CPU_RELEASE_ADDR 0x1000c0
#define CONFIG_SYS_NONCACHED_MEMORY	(1 << 20)

#define CONFIG_SYS_BOOTM_LEN	(64 << 20)

/*#define QSERVER_TIMER_FREQ 10000000*/
/* Physical Memory Map */
#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_SDRAM_SIZE		0x40000000

#define CONFIG_SYS_INIT_RAM_SIZE	0x20000

#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SYS_SDRAM_BASE + 0x7fff0)
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x80000)


/* Generic Interrupt Controller Definitions */
/*#define CONFIG_GICV3*/
#ifdef CONFIG_GICV3
#define GICD_BASE			0x20000000
#define GICR_BASE			0x20060000
#else
#define GICD_BASE			0x38001000
#define GICC_BASE			0x38002000
#endif

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + SZ_16M)

/* Serial port PL010/PL011 through the device model */
#define CONFIG_SYS_BAUDRATE_TABLE \
	{ 4800, 9600, 19200, 38400, 57600, 115200 }


/* nand flash */
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_REGS_BASE	0x0a000000
#define CONFIG_SYS_NAND_DATA_BASE	0x0a100000

/* USB */
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_DWC2_REG_ADDR 0x28000000
#define CONFIG_DWC2_DFLT_SPEED_FULL
#define CONFIG_DWC2_ENABLE_DYNAMIC_FIFO

#define QCHIP_USB_DWC3_BASE	0x20000000
#endif

#undef CONFIG_IPADDR
#define CONFIG_IPADDR                   192.168.0.115
#define CONFIG_NETMASK                  255.255.255.0
#undef CONFIG_SERVERIP
#define CONFIG_SERVERIP                 192.168.0.108
#undef CONFIG_GATEWAYIP
#define CONFIG_GATEWAYIP				192.168.0.1

#define SF_BOOTCMD		\
		"sf probe" \
		" && sf read 0x40080000 0x05200000 0x1000000"	\
		" && sf read 0x48000000 0x06100000 0x10000"	\
		" && booti 0x40080000 - 0x48000000"

#define RAM_BOOTCMD		\
		"cp.b 0x05000000 0x40080000 0xa000000" \
		" && cp.b 0x04100000 0x48000000 0x10000"	\
		" && booti 0x40080000 - 0x48000000"
#if 0
#define CONFIG_BOOTCOMMAND	SF_BOOTCMD
#endif

#define CONFIG_EXTRA_ENV_SETTINGS	\
				"kernel_name=Image\0"	\
				"kernel_addr_r=0x00080000\0" \
				"fdtfile=qserver.dtb\0" \
				"fdt_addr_r=0x22000000\0" \
				"fdt_high=0xffffffffffffffff\0" \
				"initrd_high=0xffffffffffffffff\0" \
				"ramboot=" RAM_BOOTCMD "\0"	\
				"sfboot=" SF_BOOTCMD


/* env in ram */

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_MAXARGS		64	/* max command args */
#endif /* __QSERVER_H */
