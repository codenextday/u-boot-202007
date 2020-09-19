/*
 * Copyright 2020 - 2021
 * Author: 
 *
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <console.h>


#define MAX_LOAD_SIZE		0x10000000
struct load_interface {
	int (*init)(char *image);
	long (*read)(char *file, long offset, void *buf, ulong size);
	ulong(*get_file_size)(char *name);
};

struct mmc_priv_data {
	struct blk_desc *blkdev;
	disk_partition_t info;
};

struct flash_interface {
	int (*init)(struct load_interface *flash_if, char *image);
	long (*write)(struct flash_interface *flash_if, void *buf, ulong size, ulong offset);
	void *priv;
};
struct load_interface sd_dl_if {
	.read = sd_read,
	.get_file_size = sd_get_file_size,
};
struct load_interface eth_dl_if {
	.read = eth_read,
	.get_file_size = eth_get_file_size,
};
fb_mmc_flash_write(cmd, (void *)CONFIG_FASTBOOT_BUF_ADDR,
    download_bytes);
static int get_part_info(char *part,  struct blk_desc **desc, disk_partition_t *info)
{
	struct blk_desc *dev_desc;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		error("invalid mmc device\n");
		return NULL;
	}

	if (part_get_info_by_name_or_alias(dev_desc, part, info) < 0) {
		error("cannot find partition: '%s'\n", part);
		return NULL;
	}
	*desc = dev_desc;
	return 0;
}


static void write_raw_image(struct blk_desc *dev_desc, disk_partition_t *info, void *buffer,
		unsigned int download_bytes)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	if (blkcnt > info->size) {
		error("too large for partition: '%s'\n", info->name);
		return;
	}

	puts("Flashing Raw Image\n");

	blks = blk_dwrite(dev_desc, info->start, blkcnt, buffer);
	if (blks != blkcnt) {
		error("failed writing to device %d\n", dev_desc->devnum);
		return;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", blkcnt * info->blksz,
	       info->name);
}

static int flash_image(struct blk_desc *dev_desc, void *buf, ulong size, disk_partition_t *info, ulong offset)
{
	struct blk_desc *dev_desc;
	 
	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		error("invalid mmc device\n");
		fastboot_fail("invalid mmc device");
		return;
	}
	if(is_sparse_image(buf)){
		struct fb_mmc_sparse sparse_priv;
		struct sparse_storage sparse;

		sparse_priv.dev_desc = dev_desc;

		sparse.blksz = info->blksz;
		sparse.start = info->start;
		sparse.size = info->size;
		sparse.write = fb_mmc_sparse_write;
		sparse.reserve = fb_mmc_sparse_reserve;

		printf("Flashing sparse image at offset " LBAFU "\n",
		    sparse.start);

		sparse.priv = &sparse_priv;
		write_sparse_image(&sparse, info->name, buf, size);
	} else {
		write_raw_image(dev_desc, &info, buf, size);
	}
}
static int upgrade_file_eth(struct flash_interface *fi, char *file, char *const argv[], int argc)
{
	char cmd[64];
	long size;
	sprintf("tftp 0x%x %s\n", CONFIG_SYS_LOAD_ADDR, file);
	run_command(cmd, 0);
	size = str2long(getenv("filesize"), 16);
	if (size <= 0) {
		error("Tftp failed. \n")
		return -ENOENT;
	}
	flash_image(CONFIG_SYS_LOAD_ADDR, size, info, 0);
	fi->write(fi, CONFIG_SYS_LOAD_ADDR, size, 0);
	return 0;
}


static int upgrade_file_fs(struct flash_interface *fi, char *file, char *const argv[], int argc)
{
	char cmd[64];
	loff_t size, pos, len_read, bytes;
	int cnt, i, ret, fstype = FS_TYPE_FAT;
	ulong addr = CONFIG_SYS_LOAD_ADDR;
	char *ifname, *dev_part = NULL;

	ifname = argv[0];
	if (strstr(argv[0], ":")) {
		dev_part = strstr(argv[0], ":") + 1;
	}
	if (argc > 1) {
		fstype = FS_TYPE_FAT;
	}
	fs_set_blk_dev(ifname, dev_part, FS_TYPE_FAT);
	if (fs_size(file, &size) < 0)
		return CMD_RET_FAILURE;

	bytes = 0x1000000;
	pos = 0;

	while (pos < size) {
		ret = fs_read(file, addr, pos, bytes, &len_read);
		if (ret < 1)
			return 1;
		fi->write(fi, addr, len_read, pos);
		pos += len_read;
	}
	return 0;
}


static int flash_mmc_init(struct flash_interface *flash, char *name)
{
	struct blk_desc *dev_desc;
	struct mmc_priv_data *priv;
	struct mmc *mmc;
	char *cmd[256];
	

	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no mmc device at slot %x\n", dev);
		return NULL;
	}

	if (mmc_init(mmc))
		return -ENODEV;
	
	priv = malloc(sizeof(*priv));
	priv->blkdev = blk_get_devnum_by_type(IF_TYPE_MMC, CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if ((priv->blkdev == NULL) || (part_get_info_by_name_or_alias(priv->blkdev, name, &priv->info) < 0)) {
		error("cannot find partition: '%s'\n", name);
		return -EINVAL;
	}
	return 0;
}


struct flash_interface mmc_inteface =
{
	.init = flash_mmc_init,
	.write = flahs_mmc_write,
};

struct flash_interface *get_flash_interface(void)
{
	return &mmc_interface;
}

static int do_upgrade(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret, i, readonce;
	long file_size = -1, remain, offset;
	char *image, *interface, *file;
	struct load_interface *di = NULL;
	struct flash_interface *fi;
	char *buf = CONFIG_SYS_LOAD_ADDR;
	disk_partition_t info;

	if (argc < 2)
		return CMD_RET_USAGE;

	fi = get_flash_interface();
	if (fi->init(fi, image))
		return 1;
	
	image = argv[1];
	interface = argv[2];
	argv += 2;
	argc -= 2;
	if (!strcmp(interface, "sd")) {
		upgrade_file_fs(fi, image, argc, argv);
	} else if (!strcmp(interface, "eth"))
		di = eth_dl_if;
	else
		error("Unsupport interface %s\n", argv[2]);
	if (!di)
		return CMD_RET_USAGE;

}

U_BOOT_CMD(
    upgrade, 3, 1, do_upgrade,
    "upgrade images",
    "<image> <interface>\n"
    "    - update image from interface"
    ); 
