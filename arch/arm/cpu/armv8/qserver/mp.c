/*
 * (C) Copyright 2014 - 2015 Xilinx, Inc.
 * Michal Simek <michal.simek@xilinx.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>


int is_core_valid(unsigned int core)
{
	return 0;
}

int cpu_reset(int nr)
{
	puts("Feature is not implemented.\n");
	return 0;
}

int cpu_disable(int nr)
{
	printf("Unsupported\n");
}

int cpu_status(int nr)
{
	printf("Unsupported\n");
	return 0;
}

int cpu_release(int nr, int argc, char * const argv[])
{
	printf("Unsupported\n");
	return 0;
}
