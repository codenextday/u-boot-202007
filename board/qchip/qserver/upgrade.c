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
#include <part.h>
#include <mmc.h>
#include <blk.h>
#include <memalign.h>
#include <log.h>
#include <net.h>
#include <dfu.h>
#include <image.h>


#define MAX_LOAD_SIZE		0x10000000
#define STORAGE_INTERFACE	"mmc"
#define FITIMAGE_FILE_NAME	"image.itb"
#define BOOT_FILE_NAME		"bootloader.img"
#define KERNEL_FILE_NAME	"kernel.img"
#define ROOTFS_FILE_NAME	"rootfs.img"

/* set configuration defaults if needed */
#ifndef CONFIG_UPDATE_LOAD_ADDR
#define CONFIG_UPDATE_LOAD_ADDR		0x48000000
#endif

#ifndef CONFIG_UPDATE_TFTP_MSEC_MAX
#define CONFIG_UPDATE_TFTP_MSEC_MAX	100
#endif

#ifndef CONFIG_UPDATE_TFTP_CNT_MAX
#define CONFIG_UPDATE_TFTP_CNT_MAX	0
#endif

#define DFU_ALT_BUF_LEN SZ_1K
struct qchip_upgrade_data {
	unsigned long addr;
	char *image_name;
	char *part_name;
	char *src_interface;
	struct dfu_entity *sdfu;
	struct dfu_entity *ddfu;
};
struct qchip_upgrade_data *qchip_udata;
#if 0
static int get_storage_dev(char **interface, char **devstring)
{
	*interface = STORAGE_INTERFACE;
	ret = dfu_init_env_entities(interface, devstring);
	if (ret) {
		return -EINVAL;
	}
}
#endif
void set_dfu_alt_info_mmc(struct udevice *dev, char *buf)
{
	struct disk_partition info;
	int p, len, devnum;
	bool first = true;
	const char *name;
	struct mmc *mmc;
	struct blk_desc *desc;

	mmc = mmc_get_mmc_dev(dev);
	if (!mmc)
		return;

	if (mmc_init(mmc))
		return;

	desc = mmc_get_blk_desc(mmc);
	if (!desc)
		return;

	name = blk_get_if_type_name(desc->if_type);
	devnum = desc->devnum;
	len = strlen(buf);

	if (buf[0] != '\0')
		len += snprintf(buf + len,
			DFU_ALT_BUF_LEN - len, "&");
	len += snprintf(buf + len, DFU_ALT_BUF_LEN - len,
		"%s %d=", name, devnum);

	if (IS_MMC(mmc) && mmc->capacity_boot) {
		len += snprintf(buf + len, DFU_ALT_BUF_LEN - len,
			"bootloader0 raw 0x0 0x%llx mmcpart 1;",
			mmc->capacity_boot);
		len += snprintf(buf + len, DFU_ALT_BUF_LEN - len,
			"bootloader1 raw 0x0 0x%llx mmcpart 2",
			 mmc->capacity_boot);
		first = false;
	}

	for (p = 1; p < MAX_SEARCH_PARTITIONS; p++) {
		if (part_get_info(desc, p, &info))
			continue;
		/* DOS part type means that the mmc is a source device */
		if (desc->part_type == PART_TYPE_DOS) {
			len += snprintf(buf + len, DFU_ALT_BUF_LEN - len, "%s fat %d %d", FITIMAGE_FILE_NAME, devnum, p);
			len += snprintf(buf + len, DFU_ALT_BUF_LEN - len, "%s fat %d %d", BOOT_FILE_NAME, devnum, p);
			len += snprintf(buf + len, DFU_ALT_BUF_LEN - len, "%s fat %d %d", KERNEL_FILE_NAME, devnum, p);
			len += snprintf(buf + len, DFU_ALT_BUF_LEN - len, "%s fat %d %d", ROOTFS_FILE_NAME, devnum, p);
			break;
		}
		if (!first)
			len += snprintf(buf + len, DFU_ALT_BUF_LEN - len, ";");
		first = false;
		len += snprintf(buf + len, DFU_ALT_BUF_LEN - len,
			"%s part %d %d",
			info.name, devnum, p);
	}
	log_debug("qchip add alt:%s\n", buf);
}

void set_dfu_alt_info(char *interface, char *devstr)
{
	struct udevice *dev;
	/*struct mtd_info *mtd;*/

	ALLOC_CACHE_ALIGN_BUFFER(char, buf, DFU_ALT_BUF_LEN);

	if (env_get("dfu_alt_info"))
		return;

	memset(buf, 0, sizeof(buf));
#if 0
	snprintf(buf, DFU_ALT_BUF_LEN,
		"ram 0=%s", CONFIG_DFU_ALT_RAM0);
#endif
	if (!uclass_get_device(UCLASS_MMC, 0, &dev))
		set_dfu_alt_info_mmc(dev, buf);

	if (!uclass_get_device(UCLASS_MMC, 1, &dev))
		set_dfu_alt_info_mmc(dev, buf);
	if (strlen(buf) == 0) {
		log_err("No mmc found\n");
		return;
	}
	env_set("dfu_alt_info", buf);
	log_info("DFU alt info setting: done\n");
}

static char* image_to_part(char *image_name)
{
	char *part;
	if (!strcmp(image_name,  BOOT_FILE_NAME))
		part = "boot0";
	else if (!strcmp(image_name, KERNEL_FILE_NAME))
		part = "kernel";
	else if (!strcmp(image_name, ROOTFS_FILE_NAME))
		part = "rootfs"; 
	else
		part = NULL;
	return part;
}

static int upgrade_raw_images(struct qchip_upgrade_data *data, u64 size)
{
	int rlen, ret;
	int seq = 0;
	void *buf = (void *)data->addr;

	do {
		rlen = dfu_read(data->sdfu, buf, 0x1000000, seq);
		if (rlen < 0) {
			log_err("read error\n");
			return -EACCES;
		}
		ret = dfu_write_from_mem_addr(data->ddfu, buf, rlen);
		if (ret) {
			log_err("write error\n");
			return -EACCES;
		}
		seq++;
		size -= rlen;
	} while (size > 0);
	return 0;
}
#if 0
static int upgrade_fit_images_memory(struct dfu_entity *dfu, void *fit, int noffset)
{
	ulong update_addr, update_fladdr, update_size;
	char *part;
	char *fit_image_name;
	struct dfu_entity *dfu;
	int alt_num, ret = 0;
	if (update_fit_getparams(fit, noffset, &update_addr,
			&update_fladdr, &update_size)) {
		printf("Error: can't get update parameteres, "
			"aborting\n");
		goto out;
	}

	if (fit_image_check_type(fit, noffset,
			IH_TYPE_FIRMWARE)) {
		fit_image_name = (char *)fit_get_name(fit, noffset, NULL); 
		part = image_to_part(fit_image_name);
		if (!part)
			goto out;
		alt_num = dfu_get_alt(part);
		if (alt_num < 0) {
			log_err("Cannot found alt %s\n", part);
			goto out;
		}
		dfu = dfu_get_entity(alt_num);
		if (!dfu) {
			log_err("Cannot found DFU entity for alt %d\n", alt_num);
			goto out;
		}
		dfu_write_from_mem_addr(dfu, update_addr, update_size);
	}
	
out:
	return ret;
}
static in upgrade_fit_images(struct qchip_upgrade_data *data, u64 size)
{
	void *fit = buf;
	int images_noffset, ndepth, noffset, ret;
	char *fit_image_name;
	char *image_name = data->image_name;
	char *part_name = data->part_name;
	void *buf = data->addr;

	/* process updates */
	images_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);

	ndepth = 0;
	noffset = fdt_next_node(fit, images_noffset, &ndepth);
	while (noffset >= 0 && ndepth > 0) {
		if (ndepth != 1)
			goto next_node;

		fit_image_name = (char *)fit_get_name(fit, noffset, NULL);

		if (image_name && strcmp(image_name, fit_image_name))
			goto next_node;

		printf("Processing update '%s' :", fit_image_name);

		if (!fit_image_verify(fit, noffset)) {
			printf("Error: invalid update hash, aborting\n");
			ret = 1;
			goto next_node;
		}

		printf("\n");
		
		if (!data->sdfu) {
			upgrade_raw_images
		} else {
			ulong update_addr, update_fladdr, update_size;
			char *part;
			struct dfu_entity *dfu;
			int alt_num;
			if (update_fit_getparams(fit, noffset, &update_addr,
					&update_fladdr, &update_size)) {
				printf("Error: can't get update parameteres, "
					"aborting\n");
				ret = 1;
				goto next_node;
			}
	
			if (fit_image_check_type(fit, noffset,
					IH_TYPE_FIRMWARE)) {
				part = image_to_part(fit_image_name);
				if (!part)
					goto next_node; 
				alt_num = dfu_get_alt(part);
				if (alt_num < 0) {
					log_err("Cannot found alt %s\n", data->part_name);
					goto next_node;
				}
				dfu = dfu_get_entity(alt_num);
				if (!dfu) {
					log_err("Cannot found DFU entity for alt %d\n", alt_num);
					goto next_node;
				}
			}
		}
next_node:
		noffset = fdt_next_node(fit, noffset, &ndepth);
	}
}
#endif
static int upgrade_from_dfu(struct qchip_upgrade_data *data)
{
	int ret;
	u64 size;
	struct dfu_entity *sdfu = data->sdfu;
	void *buf = (void *)data->addr;
	void *fit;
	bool is_fit_image = true;

	ret = dfu_read(sdfu, buf, 0x400, 0);
	if (ret) {
		log_err("Read error\n");
		return -EACCES;
	}
	ret = sdfu->get_medium_size(sdfu, &size);
	fit = (void *)buf;

	if (!fit_check_format((void *)fit)) {
		printf("Raw image update\n");
		is_fit_image = false;
	}

	dfu_transaction_cleanup(sdfu);
	
	if (!is_fit_image) {
		upgrade_raw_images(data, size);
		return 0;
	}
	printf("Not support fit images\n");
	return 0;

}

static int upgrade_from_tftp(char *image_name, char *interface, char *devstring)
{
	env_set("updatefile", image_name);
	return update_tftp(0, interface, devstring);
}


static int set_src_dest_dfu(struct qchip_upgrade_data *data)
{
	int i, alt_num, ret = 0;
	struct udevice *dev;
	struct mmc *mmc;
	if (!strcmp(qchip_udata->src_interface, "sd")) {
		for (i = 0; i < 2; i++) {
			if (uclass_get_device(UCLASS_MMC, i, &dev))
				break;
			mmc = mmc_get_mmc_dev(dev);
			if (!mmc)
				return -EINVAL;
			if (IS_SD(mmc))
				break;
		}
		if (i == 2) {
			log_err("No sd found\n");
			return -ENODEV;
		}
	}
	alt_num = dfu_get_alt(data->image_name);
	if (alt_num < 0) {
		log_err("Cannot found alt %s\n", data->image_name);
		ret = -ENODEV;
		goto done;
	}
	data->sdfu = dfu_get_entity(alt_num);
	if (!data->sdfu) {
		log_err("Cannot found DFU entity for alt %d\n", alt_num);
		ret = -ENODEV;
		goto done;
	}

	if (!data->part_name) {
		data->ddfu = NULL;
		return 0;
	}
	alt_num = dfu_get_alt(data->part_name);
	if (alt_num < 0) {
		log_err("Cannot found alt %s\n", data->part_name);
		ret = -ENODEV;
		goto done;
	}
	data->ddfu = dfu_get_entity(alt_num);
	if (!data->ddfu) {
		log_err("Cannot found DFU entity for alt %d\n", alt_num);
		ret = -ENODEV;
		goto done;
	}
done:
	return ret;

}

static int do_upgrade(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	char *env_addr;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;
	qchip_udata = malloc(sizeof(*qchip_udata));
	if (!qchip_udata)
		return CMD_RET_USAGE;
	memset(qchip_udata, 0, sizeof(*qchip_udata));

	qchip_udata->part_name = argv[1];
	if (!strcmp(argv[1], "boot")) {
		qchip_udata->image_name = BOOT_FILE_NAME;
		qchip_udata->part_name = "boot0";
	} else if (!strcmp(argv[1], "kernel")) {
		qchip_udata->image_name = KERNEL_FILE_NAME;
	} else if (!strcmp(argv[1], "rootfs")) {
		qchip_udata->image_name = ROOTFS_FILE_NAME;
	} else if (!strcmp(argv[1], "all")) {
		qchip_udata->image_name = FITIMAGE_FILE_NAME;
		qchip_udata->part_name = NULL;
	} else if (!strcmp(argv[1], "fit")) {
		qchip_udata->image_name = FITIMAGE_FILE_NAME;
		qchip_udata->part_name = NULL;
	} else
		return CMD_RET_USAGE; 
	if (argc == 3) {
		qchip_udata->src_interface = argv[2];
	} else {
		qchip_udata->src_interface = "tftp";
	}

	ret = set_src_dest_dfu(qchip_udata);
	if (ret)
		return ret;
	/* get load address of downloaded update file */
	env_addr = env_get("loadaddr");
	if (env_addr)
		qchip_udata->addr = simple_strtoul(env_addr, NULL, 16);
	else
		qchip_udata->addr = CONFIG_UPDATE_LOAD_ADDR;
	ret = upgrade_from_dfu(qchip_udata);
	if (qchip_udata->sdfu)
		dfu_transaction_cleanup(qchip_udata->sdfu);
	if (qchip_udata->ddfu)
		dfu_transaction_cleanup(qchip_udata->ddfu);
	return ret;
}

U_BOOT_CMD(
    upgrade, 3, 1, do_upgrade,
    "Qchip Upgrade Firmware",
    "<image> <interface> <dev>\n"
    "    - update image from interface"
    ); 
