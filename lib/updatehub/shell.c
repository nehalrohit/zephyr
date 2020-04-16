/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <drivers/flash.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include "include/updatehub.h"
#include "updatehub_firmware.h"
#include "updatehub_device.h"

#if defined(CONFIG_UPDATEHUB_CE)
#define UPDATEHUB_SERVER CONFIG_UPDATEHUB_SERVER
#else
#define UPDATEHUB_SERVER "coap.updatehub.io"
#endif

extern struct k_mbox uhu_mbox;
extern const k_tid_t uhu_tmain;

static int cmd_run(const struct shell *shell, size_t argc,
		   char **argv)
{
	int ret = -1;
	struct k_mbox_msg uhu_s;
	struct k_mbox_msg uhu_r;
	u32_t uhu_ret;

	shell_fprintf(shell, SHELL_INFO, "Starting UpdateHub run...\n");

	uhu_s.info = 101;
	uhu_s.size = 0;
	uhu_s.tx_data = NULL;
	uhu_s.tx_block.data = NULL;
	uhu_s.tx_target_thread = uhu_tmain;

	uhu_r.info = 101;
	uhu_r.size = 1;
	uhu_r.rx_source_thread = uhu_tmain;

	k_mbox_put(&uhu_mbox, &uhu_s, K_FOREVER);
	k_mbox_get(&uhu_mbox, &uhu_r, &uhu_ret, K_FOREVER);

	if (uhu_s.info == uhu_r.info) {
		ret = 0;
		shell_fprintf(shell, SHELL_INFO, "Valor: %d\n", uhu_ret);
	} else {
		shell_fprintf(shell, SHELL_INFO, "Error: %d\n", ret);
	}
/*
	switch (updatehub_probe()) {
	case UPDATEHUB_HAS_UPDATE:
		switch (updatehub_update()) {
		case UPDATEHUB_OK:
			ret = 0;
			break;
		default:
			shell_fprintf(shell, SHELL_ERROR, "Error installing update.\n");
			break;
		}
		break;

	case UPDATEHUB_NO_UPDATE:
		shell_fprintf(shell, SHELL_INFO, "No update found\n");
		ret = 0;
		break;

	default:
		shell_fprintf(shell, SHELL_ERROR, "Invalid response\n");
		break;
	}
*/
	return ret;
}

static int cmd_info(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	char *device_id = k_malloc(DEVICE_ID_HEX_MAX_SIZE);
	char *firmware_version = k_malloc(BOOT_IMG_VER_STRLEN_MAX);

	updatehub_get_device_identity(device_id, DEVICE_ID_HEX_MAX_SIZE);
	updatehub_get_firmware_version(firmware_version, BOOT_IMG_VER_STRLEN_MAX);

	shell_fprintf(shell, SHELL_NORMAL, "Unique device id: %s\n",
		      device_id);
	shell_fprintf(shell, SHELL_NORMAL, "Firmware Version: %s\n",
		      firmware_version);
	shell_fprintf(shell, SHELL_NORMAL, "Product uid: %s\n",
		      CONFIG_UPDATEHUB_PRODUCT_UID);
	shell_fprintf(shell, SHELL_NORMAL, "UpdateHub Server: %s\n",
		      UPDATEHUB_SERVER);

	k_free(device_id);
	k_free(firmware_version);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_updatehub, SHELL_CMD(info, NULL, "Dump UpdateHub information",
							cmd_info),
			       SHELL_CMD(run, NULL, "Trigger an UpdateHub update run", cmd_run),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(updatehub, &sub_updatehub, "UpdateHub commands", NULL);
