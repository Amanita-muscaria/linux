/*
 * Copyright 2013-2015 Freescale Semiconductor, Inc.
 * Copyright 2016 reMarkable AS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/mfd/syscon.h>
#include <linux/mfd/syscon/imx6q-iomuxc-gpr.h>
#include <linux/gpio.h>
#include <linux/regmap.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#define WL_PWR_ON IMX_GPIO_NR(3, 30)
#define WL_REG_ON IMX_GPIO_NR(4, 0)


#include "common.h"
#include "hardware.h"
#include "cpuidle.h"

static void __init imx6sl_fec_clk_init(void)
{
	struct regmap *gpr;

	/* set FEC clock from internal PLL clock source */
	gpr = syscon_regmap_lookup_by_compatible("fsl,imx6sl-iomuxc-gpr");
	if (!IS_ERR(gpr)) {
		regmap_update_bits(gpr, IOMUXC_GPR1,
			IMX6SL_GPR1_FEC_CLOCK_MUX2_SEL_MASK, 0);
		regmap_update_bits(gpr, IOMUXC_GPR1,
			IMX6SL_GPR1_FEC_CLOCK_MUX1_SEL_MASK, 0);
	} else
		pr_err("failed to find fsl,imx6sl-iomux-gpr regmap\n");
}

static inline void imx6sl_fec_init(void)
{
	imx6sl_fec_clk_init();
	imx6_enet_mac_init("fsl,imx6sl-fec", "fsl,imx6sl-ocotp");
}
static bool init_wifi_gpio(void)
{
	int err;

	err = gpio_request_one(WL_PWR_ON, GPIOF_OUT_INIT_LOW, "wl_pwr_on");
	if (err) {
		pr_err("Failed to request power GPIO for wifi: %d\n", err);
		return false;
	}

	err = gpio_request_one(WL_REG_ON, GPIOF_OUT_INIT_LOW, "wl_reg_on");
	if (err) {
		pr_err("Failed to request regulator GPIO for wifi: %d\n", err);
		return false;
	}

	return true;
}

static void brcmfmac_power_off(void)
{
	gpio_set_value(WL_PWR_ON, 0);
	gpio_set_value(WL_REG_ON, 0);
}


static void brcmfmac_power_on(void)
{
	gpio_set_value(WL_PWR_ON, 1);
	gpio_set_value(WL_REG_ON, 1);
}


static void __init imx6sl_init_late(void)
{
	/* imx6sl reuses imx6q cpufreq driver */
	if (IS_ENABLED(CONFIG_ARM_IMX6Q_CPUFREQ))
		platform_device_register_simple("imx6q-cpufreq", -1, NULL, 0);

	imx6sl_cpuidle_init();
}

static void __init imx6sl_init_machine(void)
{
	struct device *parent;

	parent = imx_soc_device_init();
	if (parent == NULL)
		pr_warn("failed to initialize soc device\n");

	of_platform_populate(NULL, of_default_bus_match_table, NULL, parent);

	if (init_wifi_gpio()) {
		brcmfmac_power_on();
	} else {
		pr_warn("failed to power on wifi\n");
	}

	imx6sl_fec_init();
	imx_anatop_init();
	imx6sl_pm_init();
}

static void __init imx6sl_init_irq(void)
{
	imx_gpc_check_dt();
	imx_init_revision_from_anatop();
	imx_init_l2cache();
	imx_src_init();
	irqchip_init();
}

static void __init imx6sl_map_io(void)
{
	debug_ll_io_init();
	imx6_pm_map_io();
#ifdef CONFIG_IMX_BUSFREQ
	imx_busfreq_map_io();
#endif
}

static const char * const imx6sl_dt_compat[] __initconst = {
	"fsl,imx6sl",
	NULL,
};

DT_MACHINE_START(IMX6SL, "Freescale i.MX6 SoloLite (Device Tree)")
	.map_io		= imx6sl_map_io,
	.init_irq	= imx6sl_init_irq,
	.init_machine	= imx6sl_init_machine,
	.init_late      = imx6sl_init_late,
	.dt_compat	= imx6sl_dt_compat,
MACHINE_END
