/*
 * (C) Copyright 2015 Linaro
 * Peter Griffin <peter.griffin@linaro.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#define DEBUG
#include <common.h>
#include <log.h>
#include <dm.h>
#include <dm/platform_data/serial_pl01x.h>
#include <errno.h>
#include <malloc.h>
#include <netdev.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <bitfield.h>
#include <env.h>
#include <usb.h>
#include <usb/dwc2_udc.h>
#include <asm-generic/gpio.h>
#include <asm/arch/dwmmc.h>
#include <asm/arch/gpio.h>
#include <asm/arch/periph.h>
#include <asm/arch/pinmux.h>
#include <asm/armv8/mmu.h>
#include <dwc3-uboot.h>
#include <ti-usb-phy-uboot.h>

DECLARE_GLOBAL_DATA_PTR;


enum board_type {
	QCHIP_B1,
	QCHIP_B2,
	QCHIP_B3,
	QCHIP_B4
};

enum qchip_modules {
	QCHIP_NAND,
	QCHIP_GMAC,
	QCHIP_MMC,
	QCHIP_SD,
	QCHIP_USB20,
	QCHIP_USB30,
	QCHIP_SFC,
	QCHIP_CAM_I2C,
	QCHIP_HDMI_PHY,
	QCHIP_XGMAC0,
	QCHIP_XGMAC1,
	QCHIP_XLGMAC0,
	QCHIP_XLGMAC1,
	QCHIP_MODULES_END
};

#define SYS_BOARD_INFO_REG      (0x10000004l)
#define BOARD_TYPE_MASK         (0xff)
#define NAND_BIT                BIT(8)
#define MAC_BIT                 BIT(9)
#define MMC_BIT                 BIT(10)
#define SD_BIT                  BIT(11)
#define USB_BIT                 BIT(12)
#define SFC_BIT                 BIT(13)
#define CAM_I2C_BIT             BIT(14)

#define HDMI_PHY_SHIFT          (16)
#define HDMI_PHY_WIDTH          (2)

#define USB3_BIT             BIT(18)


struct module_bitmap {
	char *name;
	char *of_alias;
	u8 shift;
	u8 width;
	u8 valid_sel;
};

#define MODULE_INFO(n, a, s, w) \
{ \
	.name = n, \
	.of_alias = a, \
	.shift = s, \
	.width = w, \
	.valid_sel = 0, \
}

#define MODULE_VALID_INFO(n, a, s)	MODULE_INFO(n, a, s, 1)
#define MODULE_SEL_INFO(n, a, s, w)	MODULE_INFO(n, a, s, w)

struct module_bitmap modules_info[] = {
	[QCHIP_NAND] = MODULE_VALID_INFO("nand", "nand0", 8),
	[QCHIP_GMAC] = MODULE_VALID_INFO("ethernet", "ethernet0", 9),
	[QCHIP_MMC] = MODULE_VALID_INFO("mmc", "mmc0", 10),
	[QCHIP_SD] = MODULE_VALID_INFO("sd", "mmc1", 11),
	[QCHIP_USB20] = MODULE_VALID_INFO("usb20", "usb0", 12),
	[QCHIP_SFC] = MODULE_VALID_INFO("spiflash", "spi0", 13),
	[QCHIP_CAM_I2C] = MODULE_VALID_INFO("spiflash", "spi0", 14),
	[QCHIP_XGMAC0] = MODULE_VALID_INFO("xgmac0", "ethernet1", 20),
	[QCHIP_XGMAC1] = MODULE_VALID_INFO("xgmac1", "ethernet2", 19),
	/*[QCHIP_XLGMAC0] = MODULE_VALID_INFO("xlgmac0", "ethernet3", 19),
	[QCHIP_XLGMAC1] = MODULE_VALID_INFO("xlgmac1", "ethernet4", 19),*/
	[QCHIP_HDMI_PHY] = MODULE_SEL_INFO("hdmi_phy", NULL, 16, 2),
	[QCHIP_USB30] = MODULE_VALID_INFO("usb30", "usb1", 18),
};

static int module_info_init(void)
{
	int cnt = ARRAY_SIZE(modules_info);
	int i;

	u32 val = readl(SYS_BOARD_INFO_REG);
	if (!val)
		val = 0xcb1;
	for (i = 0; i < cnt; i++)
		modules_info[i].valid_sel = bitfield_extract(val, modules_info[i].shift, modules_info[i].width);
	return 0;
}

static inline int module_valid_sel(enum qchip_modules module)
{
	int valid = 0;
	if (module >= QCHIP_MODULES_END)
		return -ENODEV;
	valid = modules_info[module].valid_sel;
	debug("%s is %s\n", modules_info[module].name, valid ? "valid" : "invalid");
	return modules_info[module].valid_sel;
}

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
	module_info_init();
	return 0;
}
#endif



#define EYE_PATTERN	0x70533483

#ifdef CONFIG_USB_GADGET
/*static int qserver_phy_control(int on)
{
	return 0;
}*/
#ifdef CONFIG_USB_GADGET_DWC2_OTG
struct dwc2_plat_otg_data qserver_otg_data = {
	.regs_otg       = CONFIG_USB_DWC2_REG_ADDR,
};
#endif
#ifdef CONFIG_USB_DWC3
static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = QCHIP_USB_DWC3_BASE,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
};


int usb_gadget_handle_interrupts(int index)
{
	dwc3_uboot_handle_interrupt(index);

	return 0;
}

#endif
int board_usb_init(int index, enum usb_init_type init)
{
	printf("usb init\n");
#ifdef CONFIG_USB_GADGET_DWC2_OTG
	/* usb2 first */
	if (module_valid_sel(QCHIP_USB20))
		return dwc2_udc_probe(&qserver_otg_data); 
#endif
	/* no usb */
	if (!module_valid_sel(QCHIP_USB30))
		return 0;
#ifdef CONFIG_USB_DWC3
	/* dwc3 */
	//ti_usb_phy_uboot_init(&usb_phy_device);
	dwc3_uboot_init(&dwc3_device_data);
#endif
	return 0;
}
#endif

int misc_init_r(void)
{
	return 0;
}

enum board_type get_board_type(void)
{
	unsigned int val = readl(SYS_BOARD_INFO_REG);
	enum board_type type;
	val &= BOARD_TYPE_MASK;
	switch (val) {
	case 0xb1:
		type = QCHIP_B1;
		break;
	case 0xb2:
		type = QCHIP_B2;
		break;
	case 0xb3:
		type = QCHIP_B3;
		break;
	default:
		type = QCHIP_B2;
		break;
	}
	return type;
}

static int qserver_fixup_fdt(void *blob)
{
	/* fixup modules */
	int cnt = ARRAY_SIZE(modules_info);
	int i;
	u32 val = readl(SYS_BOARD_INFO_REG);
	if (!val) {
		val = 0xcb1;
		writel(val, SYS_BOARD_INFO_REG);
	}

	printf("Fixup fdt for %x\n", readl(SYS_BOARD_INFO_REG));
	
	for (i = 0; i < cnt; i++) {
		if (modules_info[i].width > 1)
			continue;
		if (val & BIT(modules_info[i].shift))
			fdt_status_okay_by_alias(blob, modules_info[i].of_alias);
	}
	
	if (QCHIP_B3 == get_board_type()) {
		printf("Fixup mdio phy reg property for b3 board:phy %d\n", fdt_getprop_u32_default(blob, "/soc/ethernet@16000000/mdio/phy", "reg", 0xf));
		do_fixup_by_path_u32(blob, "/soc/ethernet@16000000/mdio/phy", "reg", 0, 0);
		printf("Fixup mdio phy reg property for b3 board:phy %d\n", fdt_getprop_u32_default(blob, "/soc/ethernet@16000000/mdio/phy", "reg", 0xf)); 
	}

	return 0;
}
#ifdef CONFIG_OF_BOARD_FIXUP
int board_fix_fdt(void *blob)
{
	qserver_fixup_fdt(blob);
	return 0;
}
#endif
int board_init(void)
{
	printf("%s, defalt value of 0x10000008: %x\n", __func__, readl(0x10000008l));
	//generic_set_bit(3, 0x10000008l);
	writel(3, 0x10000008l);
	printf("%s, set value of 0x10000008: %x\n", __func__, readl(0x10000008l));
	env_set("serial#", "b1");
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
	*/

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = CONFIG_SYS_SDRAM_SIZE;

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	qserver_fixup_fdt(blob);
	//printk("####usb3:switch to host mode");
	//clrsetbits_le32(0x20000000 + 0xc110, 0x3 << 12,  0x1 << 12);
	return 0;
}
#endif
