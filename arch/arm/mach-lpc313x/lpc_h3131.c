/*  arch/arm/mach-lpc313x/lpc_h3131.c
 *
 *  Copyright (C) 2010 Petri Ahonen
 *  based on ea313x.c by Durgesh Pattamatta
 *
 *  Olimex LPC-H3131 board init routines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/device.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dm9000.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>

#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/sizes.h>

#include <asm/mach/map.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <mach/gpio.h>
#include <mach/i2c.h>
#include <mach/board.h>

#define GPIO_MMC_POWER GPIO_MI2STX_DATA0
#define GPIO_USB_VBUS_ENABLE GPIO_GPIO19

static int mci_init(u32 slot_id)
{
	gpio_request(GPIO_MMC_POWER, "lpc313x_mmc.power");
	gpio_direction_output(GPIO_MMC_POWER, 1);

	return 0;
}

static void mci_exit(u32 slot_id)
{
	gpio_free(GPIO_MMC_POWER);
}

static int mci_get_ocr(u32 slot_id)
{
	return MMC_VDD_32_33 | MMC_VDD_33_34;
}

static void mci_setpower(u32 slot_id, u32 volt)
{
	gpio_set_value(GPIO_MMC_POWER, volt ? 0 : 1);
}

static struct resource lpc313x_mci_resources[] = {
	[0] = {
	       .start = IO_SDMMC_PHYS,
	       .end = IO_SDMMC_PHYS + IO_SDMMC_SIZE,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_MCI,
	       .end = IRQ_MCI,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct lpc313x_mci_board lpc_h3131_mci_platform_data = {
	.num_slots = 1,
	.detect_delay_ms = 250,
	.init = mci_init,
	.exit = mci_exit,
	.get_ocr = mci_get_ocr,
	.setpower = mci_setpower,
	.slot = {
		[0] = {
			.bus_width = 4,
			.detect_pin = -1,
			.wp_pin = -1,
		},
	},
};

static u64 mci_dmamask = 0xffffffffUL;
static struct platform_device lpc313x_mci_device = {
	.name = "lpc313x_mmc",
	.num_resources = ARRAY_SIZE(lpc313x_mci_resources),
	.dev = {
		.dma_mask = &mci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &lpc_h3131_mci_platform_data,
		},
	.resource = lpc313x_mci_resources,
};

#if defined (CONFIG_MTD_NAND_LPC313X)
static struct resource lpc313x_nand_resources[] = {
	[0] = {
	       .start = IO_NAND_PHYS,
	       .end = IO_NAND_PHYS + IO_NAND_SIZE,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IO_NAND_BUF_PHYS,
	       .end = IO_NAND_BUF_PHYS + IO_NAND_BUF_SIZE,
	       .flags = IORESOURCE_MEM,
	       },
	[2] = {
	       .start = IRQ_NAND_FLASH,
	       .end = IRQ_NAND_FLASH,
	       .flags = IORESOURCE_IRQ,
	       }
};

#define BLK_SIZE (2048 * 64)
static struct mtd_partition lpc_h3131_nand0_partitions[] = {
	/* The LPC_H3131 board uses the following block scheme:
	   128K: Blocks 0   - 0    - LPC31xx info and bad block table
	   384K: Blocks 1   - 3    - loader image
	   256K: Blocks 4   - 5    - loader environment
	   4M:   Blocks 6   - 37   - kernel
	   16M:  Blocks 38  - 165  - ramdisk
	   ???:  Blocks 166 - end  - root filesystem / ubi */
	{
	 .name = "info",
	 .offset = (BLK_SIZE * 0),
	 .size = (BLK_SIZE * 1)},
	{
	 .name = "loader",
	 .offset = (BLK_SIZE * 1),
	 .size = (BLK_SIZE * 3)},
	{
	 .name = "environment",
	 .offset = (BLK_SIZE * 4),
	 .size = (BLK_SIZE * 2)},
	{
	 .name = "kernel",
	 .offset = (BLK_SIZE * 6),
	 .size = (BLK_SIZE * 32)},
	{
	 .name = "ramdisk",
	 .offset = (BLK_SIZE * 38),
	 .size = (BLK_SIZE * 128)},
	{
	 .name = "rootfs",
	 .offset = (BLK_SIZE * 166),
	 .size = MTDPART_SIZ_FULL},
};

static struct lpc313x_nand_timing lpc_h3131_nanddev_timing = {
	.ns_trsd = 36,
	.ns_tals = 36,
	.ns_talh = 12,
	.ns_tcls = 36,
	.ns_tclh = 12,
	.ns_tdrd = 36,
	.ns_tebidel = 12,
	.ns_tch = 12,
	.ns_tcs = 48,
	.ns_treh = 24,
	.ns_trp = 48,
	.ns_trw = 24,
	.ns_twp = 36
};

static struct lpc313x_nand_dev_info lpc_h3131_ndev[] = {
	{
	 .name = "nand0",
	 .nr_partitions = ARRAY_SIZE(lpc_h3131_nand0_partitions),
	 .partitions = lpc_h3131_nand0_partitions}
};

static struct lpc313x_nand_cfg lpc_h3131_plat_nand = {
	.nr_devices = ARRAY_SIZE(lpc_h3131_ndev),
	.devices = lpc_h3131_ndev,
	.timing = &lpc_h3131_nanddev_timing,
	.support_16bit = 0,
};

static u64 nand_dmamask = 0xffffffffUL;
static struct platform_device lpc313x_nand_device = {
	.name = "lpc313x_nand",
	.dev = {
		.dma_mask = &nand_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &lpc_h3131_plat_nand,
		},
	.num_resources = ARRAY_SIZE(lpc313x_nand_resources),
	.resource = lpc313x_nand_resources,
};
#endif

#if defined(CONFIG_SPI_LPC313X)
static struct resource lpc313x_spi_resources[] = {
	[0] = {
	       .start = SPI_PHYS,
	       .end = SPI_PHYS + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_SPI,
	       .end = IRQ_SPI,
	       .flags = IORESOURCE_IRQ,
	       },
};

static void spi_set_cs_state(int cs_num, int state)
{
	/* Only CS0 is supported, so no checks are needed */
	(void)cs_num;

	/* Set GPO state for CS0 */
	gpio_set_value(GPIO_SPI_CS_OUT0, state);
}

struct lpc313x_spics_cfg lpc313x_stdspics_cfg[] = {
	/* SPI CS0 */
	[0] = {
	       .spi_spo = 0,	/* Low clock between transfers */
	       .spi_sph = 0,	/* Data capture on first clock edge (high edge with spi_spo=0) */
	       .spi_cs_set = spi_set_cs_state,
	       },
};

struct lpc313x_spi_cfg lpc313x_spidata = {
	.num_cs = ARRAY_SIZE(lpc313x_stdspics_cfg),
	.spics_cfg = lpc313x_stdspics_cfg,
};

static u64 lpc313x_spi_dma_mask = 0xffffffffUL;
static struct platform_device lpc313x_spi_device = {
	.name = "spi_lpc313x",
	.id = 0,
	.dev = {
		.dma_mask = &lpc313x_spi_dma_mask,
		.coherent_dma_mask = 0xffffffffUL,
		.platform_data = &lpc313x_spidata,
		},
	.num_resources = ARRAY_SIZE(lpc313x_spi_resources),
	.resource = lpc313x_spi_resources,
};

/* If both SPIDEV and MTD data flash are enabled with the same chip select, only 1 will work */
#if defined(CONFIG_MTD_DATAFLASH)
/* MTD Data FLASH driver registration */
static int __init lpc313x_spimtd_register(void)
{
	struct spi_board_info info = {
		.modalias = "mtd_dataflash",
		.max_speed_hz = 30000000,
		.bus_num = 0,
		.chip_select = 0,
	};

	return spi_register_board_info(&info, 1);
}

arch_initcall(lpc313x_spimtd_register);
#else
#if defined(CONFIG_SPI_SPIDEV)
/* SPIDEV driver registration */
static int __init lpc313x_spidev_register(void)
{
	struct spi_board_info info = {
		.modalias = "spidev",
		.max_speed_hz = 1000000,
		.bus_num = 0,
		.chip_select = 0,
	};

	return spi_register_board_info(&info, 1);
}

arch_initcall(lpc313x_spidev_register);
#endif
#endif

#endif

#ifdef CONFIG_LEDS_GPIO_PLATFORM
static struct gpio_led leds[] = {
    {
	.name   = "led-1:yellow",
	.default_trigger = "mmc0",
	.gpio   = GPIO_GPIO17,
    }, {
	.name   = "led-2:green",
	.default_trigger = "heartbeat",
	.gpio   = GPIO_GPIO18,
    },
};

static struct gpio_led_platform_data leds_pdata = {
    .num_leds  = ARRAY_SIZE(leds),
    .leds      = leds,
};

static struct platform_device leds_device = {
    .name   = "leds-gpio",
    .id     = -1,
    .dev    = {
	.platform_data = &leds_pdata,
    },
};
#endif

static struct platform_device *devices[] __initdata = {
	&lpc313x_mci_device,
#if defined (CONFIG_MTD_NAND_LPC313X)
	&lpc313x_nand_device,
#endif
#if defined(CONFIG_SPI_LPC313X)
	&lpc313x_spi_device,
#endif
#if defined (CONFIG_LEDS_GPIO_PLATFORM)
        &leds_device,
#endif
};

static struct map_desc lpc_h3131_io_desc[] __initdata = {
	{
	 .virtual = io_p2v(EXT_SRAM0_PHYS),
	 .pfn = __phys_to_pfn(EXT_SRAM0_PHYS),
	 .length = SZ_4K,
	 .type = MT_DEVICE},
	{
	 .virtual = io_p2v(EXT_SRAM1_PHYS + 0x10000),
	 .pfn = __phys_to_pfn(EXT_SRAM1_PHYS + 0x10000),
	 .length = SZ_4K,
	 .type = MT_DEVICE},
	{
	 .virtual = io_p2v(IO_SDMMC_PHYS),
	 .pfn = __phys_to_pfn(IO_SDMMC_PHYS),
	 .length = IO_SDMMC_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = io_p2v(IO_USB_PHYS),
	 .pfn = __phys_to_pfn(IO_USB_PHYS),
	 .length = IO_USB_SIZE,
	 .type = MT_DEVICE},
};

static struct i2c_board_info lpc_h3131_i2c_devices[] __initdata = {
};

static void __init lpc_h3131_init(void)
{
	lpc313x_init();
	/* register i2cdevices */
	lpc313x_register_i2c_devices();

	platform_add_devices(devices, ARRAY_SIZE(devices));

	i2c_register_board_info(0, lpc_h3131_i2c_devices,
				ARRAY_SIZE(lpc_h3131_i2c_devices));

}

static void __init lpc_h3131_map_io(void)
{
	lpc313x_map_io();
	iotable_init(lpc_h3131_io_desc, ARRAY_SIZE(lpc_h3131_io_desc));
}

void lpc313x_vbus_power(int enable)
{
	if (enable) {
		gpio_set_value(GPIO_USB_VBUS_ENABLE, 0);
	} else {
		gpio_set_value(GPIO_USB_VBUS_ENABLE, 1);
	}
}

EXPORT_SYMBOL(lpc313x_vbus_power);

#if defined(CONFIG_MACH_LPC_H3131)
MACHINE_START(LPC_H3131, "Olimex LPC-H3131")
    /* Maintainer:  */
    .phys_io = IO_APB01_PHYS,.io_pg_offst =
    (io_p2v(IO_APB01_PHYS) >> 18) & 0xfffc,.boot_params = 0x30000100,.map_io =
    lpc_h3131_map_io,.init_irq = lpc313x_init_irq,.timer =
    &lpc313x_timer,.init_machine = lpc_h3131_init, MACHINE_END
#endif
