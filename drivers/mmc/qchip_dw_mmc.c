/*
 * (C) Copyright 2017
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dwmmc.h>
#include <fdtdec.h>
#include <malloc.h>
#include <errno.h>
#include <dm.h>
#include <dm/device.h>
#include <dm/ofnode.h>

#define	DWMMC_MAX_CH_NUM		4
#define	DWMMC_MAX_FREQ			25000000
#define	DWMMC_MIN_FREQ			400000
/*#define	DWMMC_MMC0_SDR_TIMING_VAL	0x03030001
#define	DWMMC_MMC2_SDR_TIMING_VAL	0x03020001*/

#ifdef CONFIG_DM_MMC
#include <dm.h>
DECLARE_GLOBAL_DATA_PTR;

struct qserver_mmc_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};
#endif

/* private data */
struct qserver_dwmmc_priv {
#ifdef CONFIG_DM_MMC
	struct dwmci_host host;
#endif
	int fifo_depth;
	bool fifo_mode;
};

unsigned int qserver_dwmci_get_clk(struct dwmci_host *host, uint freq)
{
	return DWMMC_MAX_FREQ;
}

static int qserver_dwmmc_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct qserver_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;

	host->name = dev->name;
	host->ioaddr = (void *)devfdt_get_addr(dev);
	host->buswidth = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
					"bus-width", 4);
	host->get_mmc_clk = qserver_dwmci_get_clk;
	host->priv = dev;

	/* use non-removeable as sdcard and emmc as judgement */
	if (fdtdec_get_bool(gd->fdt_blob, dev_of_offset(dev), "non-removable"))
		host->dev_index = 0;
	else
		host->dev_index = 1;

	priv->fifo_depth = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
				    "fifo-depth", 0);
	if (priv->fifo_depth < 0)
		return -EINVAL;
	priv->fifo_mode = fdtdec_get_bool(gd->fdt_blob, dev_of_offset(dev),
					  "fifo-mode");
#endif
	return 0;
}


static int qserver_dwmci_core_init(struct dwmci_host *host)
{
/*	if (host->bus_hz)
		freq = host->bus_hz;
	else
		freq = DWMMC_MAX_FREQ;*/


	host->name = "QCHIP DWMMC";

	host->board_init = NULL;//qserver_dwmci_board_init;

	host->caps = MMC_MODE_HS_52MHz;
	host->clksel = NULL;//exynos_dwmci_clksel;
	host->get_mmc_clk = qserver_dwmci_get_clk;

#ifndef CONFIG_DM_MMC
	/* Add the mmc channel to be registered with mmc core */
	if (add_dwmci(host, DWMMC_MAX_FREQ, DWMMC_MIN_FREQ)) {
		printf("DWMMC%d registration failed\n", host->dev_index);
		return -1;
	}
#endif

	return 0;
}

//static struct dwmci_host dwmci_host[DWMMC_MAX_CH_NUM];

static int do_dwmci_init(struct dwmci_host *host)
{
	return qserver_dwmci_core_init(host);
}

static int qserver_dwmci_get_config(const void *blob, int node,
					struct dwmci_host *host)
{
	unsigned long base;
	struct qserver_dwmmc_priv *priv;

	priv = malloc(sizeof(*priv));
	if (!priv) {
		printf("qserver_dwmmc_priv malloc fail!\n");
		return -ENOMEM;
	}

	/* Extract device id for each mmc channel */
	host->dev_id = 0;
	host->dev_index = 0;

	/* Set the base address from the device node */
	base = fdtdec_get_addr(blob, node, "reg");
	if (!base) {
		printf("DWMMC%d: Can't get base address\n", host->dev_index);
		return -EINVAL;
	}
	host->ioaddr = (void *)base;


	host->fifoth_val = fdtdec_get_int(blob, node, "fifoth_val", 0);
	host->bus_hz = fdtdec_get_int(blob, node, "clock-frequency", 0);
	host->buswidth = fdtdec_get_int(blob, node, "bus-width", 0);
	/*host->div = fdtdec_get_int(blob, node, "div", 0);*/

	host->priv = priv;

	return 0;
}

#ifdef CONFIG_DM_MMC
static int qserver_dwmmc_probe(struct udevice *dev)
{
	struct qserver_mmc_plat *plat = dev_get_platdata(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct qserver_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;
	ofnode node;
	int err;
	printf("%s\n", __func__);
	err = qserver_dwmci_get_config(gd->fdt_blob, dev_of_offset(dev), host);
	if (err)
		return err;
	err = do_dwmci_init(host);
	if (err)
		return err;

	dwmci_setup_cfg(&plat->cfg, host, DWMMC_MAX_FREQ, DWMMC_MIN_FREQ);
	node = ofnode_find_subnode(dev->node, "partitions");
	if (ofnode_valid(node)) {
		plat->cfg.part_type = PART_TYPE_OF;
	}
	/*else
		plat->cfg.part_type = PART_TYPE_DOS;*/
	host->mmc = &plat->mmc;
	host->mmc->priv = &priv->host;
	host->priv = dev;
	upriv->mmc = host->mmc;

	return dwmci_probe(dev);
}

static int qserver_dwmmc_bind(struct udevice *dev)
{
	struct qserver_mmc_plat *plat = dev_get_platdata(dev);

	return dwmci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id qserver_dwmmc_ids[] = {
	{ .compatible = "qchip,qchip-dwmmc" },
	{ }
};

U_BOOT_DRIVER(qserver_dwmmc_ids) = {
	.name		= "qserver_dwmmc",
	.id		= UCLASS_MMC,
	.of_match	= qserver_dwmmc_ids,
	.ofdata_to_platdata = qserver_dwmmc_ofdata_to_platdata,
	.bind		= qserver_dwmmc_bind,
	.ops		= &dm_dwmci_ops,
	.probe		= qserver_dwmmc_probe,
	.priv_auto_alloc_size	= sizeof(struct qserver_dwmmc_priv),
	.platdata_auto_alloc_size = sizeof(struct qserver_mmc_plat),
};
#endif
