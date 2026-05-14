// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (c) 2026 IBM Corp.
 */

#include <skiboot.h>
#include <stdbool.h>

#include "astbmc.h"
#include "device.h"
#include "ipmi.h"

static bool fuji_probe(void)
{
	if (!dt_node_is_compatible(dt_root, "ibm,fuji"))
		return false;

	astbmc_early_init();

	/* Setup UART for use by OPAL (Linux hvc) */
	uart_set_console_policy(UART_CONSOLE_OPAL);

	return true;
}

DECLARE_PLATFORM(fuji) = {
	.name			= "Fuji",
	.probe			= fuji_probe,
	.init			= astbmc_init,
	.start_preload_resource	= flash_start_preload_resource,
	.resource_loaded	= flash_resource_loaded,
	.bmc			= &bmc_plat_ast2600_openbmc,
	.cec_power_down         = astbmc_ipmi_power_down,
	.cec_reboot             = astbmc_ipmi_reboot,
	.elog_commit		= ipmi_elog_commit,
	.exit			= astbmc_exit,
	.terminate		= ipmi_terminate,
};
