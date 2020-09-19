/*
 * (C) Copyright 2001
 * Raymond Lo, lo@routefree.com
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Support for harddisk partitions.
 *
 * To be compatible with LinuxPPC and Apple we use the standard Apple
 * SCSI disk partitioning scheme. For more information see:
 * http://developer.apple.com/techpubs/mac/Devices/Devices-126.html#MARKER-14-92
 */

#include <common.h>
#include <command.h>
#include <ide.h>
#include <memalign.h>
#include <mmc.h>

#include <dm/device.h>
#include <dm/ofnode.h>
#include <fdtdec.h>

#ifdef HAVE_BLOCK_DEVICE

#define PART_DEFAULT_SECTOR 512

#if 0
static void print_one_part(dos_partition_t *p, lbaint_t ext_part_sector,
			   int part_num, unsigned int disksig)
{
	lbaint_t lba_start = ext_part_sector + le32_to_int (p->start4);
	lbaint_t lba_size  = le32_to_int (p->size4);

	printf("%3d\t%-10" LBAFlength "u\t%-10" LBAFlength
		"u\t%08x-%02x\t%02x%s%s\n",
		part_num, lba_start, lba_size, disksig, part_num, p->sys_ind,
		(is_extended(p->sys_ind) ? " Extd" : ""),
		(is_bootable(p) ? " Boot" : ""));
}
#endif


void part_print_of(struct blk_desc *dev_desc)
{
	printf("Part\tStart Sector\tNum Sectors\tUUID\t\tType\n");
	printf("NOT SUPPORT\n");
}

int part_get_info_of(struct blk_desc *dev_desc, int part,
		      disk_partition_t *info)
{
	int subnode, i;
	struct udevice *dev = dev_get_parent(dev_desc->bdev);
	ofnode partnode = ofnode_find_subnode(dev->node, "partitions");
	fdt_addr_t reg, size;
	const char *name;

	
	if (!ofnode_valid(partnode)) {
		error("Cannot find partitions sub node in %s\n", dev->name);
		return -ENODEV;
	}
	
	i = 0;
	fdt_for_each_subnode(subnode, gd->fdt_blob, ofnode_to_offset(partnode)) {
		if (i == part)
			break;
		i++;
	}
	reg = fdtdec_get_addr_size_auto_noparent(gd->fdt_blob, subnode, "reg", 0, &size, 0);
	name = ofnode_read_string(offset_to_ofnode(subnode), "label");
	info->start = reg;
	info->size = size;
	info->blksz = PART_DEFAULT_SECTOR;
	info->bootable = 0;
	strncpy(info->name, name, 32);
	return 0;
#if 0
	na = fdt_address_cells(fdt, partnode);
	ns = fdt_size_cells(fdt, partnode);
	reg = fdt_getprop(gd->fdt_blob, ofnode_to_offset(subnode), "reg", &len);
	if (!reg) {
		error("Failed to get reg prop in %s\n", subnode);
		return -ENODEV;
	}
	
	end = reg + len / sizeof(*reg);
	
	while (ptr + na + ns <= end) {
		if (i == index) {
			res->start = res->end = fdtdec_get_number(ptr, na);
			res->end += fdtdec_get_number(&ptr[na], ns) - 1;
			return 0;
		}
	 
		ptr += na + ns;
		i++;
	}

	info->start = 0;
	info->size = dev_desc->lba;
	info->blksz = DOS_PART_DEFAULT_SECTOR;
	info->bootable = 0;
	return part_get_info_extended(dev_desc, 0, 0, 1, part, info, 0);
#endif
}

static int part_test_of(struct blk_desc *dev_desc) 
{
	struct udevice *dev = dev_get_parent(dev_desc->bdev);
	ofnode partnode = ofnode_find_subnode(dev->node, "partitions");
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	struct mmc_config *cfg = mmc->cfg;
	
	if (cfg->part_type != PART_TYPE_OF)
		return -1;
	return 0;
}
U_BOOT_PART_TYPE(of) = {
	.name		= "OF",
	.part_type	= PART_TYPE_OF,
	.max_entries	= 8,
	.get_info	= part_get_info_ptr(part_get_info_of),
	.print		= part_print_ptr(part_print_of),
	.test		= part_test_of,
};

#endif
