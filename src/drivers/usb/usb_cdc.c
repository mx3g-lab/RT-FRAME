#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb_cdc, LOG_LEVEL_INF);

USBD_DEVICE_DEFINE(rtframe_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_RTFRAME_USB_CDC_VID, CONFIG_RTFRAME_USB_CDC_PID);

USBD_DESC_LANG_DEFINE(rtframe_lang);
USBD_DESC_MANUFACTURER_DEFINE(rtframe_mfr, CONFIG_RTFRAME_USB_CDC_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(rtframe_product, CONFIG_RTFRAME_USB_CDC_PRODUCT);

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(rtframe_fs_config, 0,
			   CONFIG_RTFRAME_USB_CDC_MAX_POWER, &fs_cfg_desc);

USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");
USBD_CONFIGURATION_DEFINE(rtframe_hs_config, 0,
			   CONFIG_RTFRAME_USB_CDC_MAX_POWER, &hs_cfg_desc);

static void usbd_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx)) {
				LOG_ERR("Failed to enable USB");
			}
		}
		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			usbd_disable(ctx);
		}
	}
}

static int usb_cdc_init(void)
{
	int err;

	err = usbd_add_descriptor(&rtframe_usbd, &rtframe_lang)   ||
	      usbd_add_descriptor(&rtframe_usbd, &rtframe_mfr)    ||
	      usbd_add_descriptor(&rtframe_usbd, &rtframe_product);
	if (err) {
		LOG_ERR("Failed to add descriptors: %d", err);
		return err;
	}

	err = usbd_add_configuration(&rtframe_usbd, USBD_SPEED_FS, &rtframe_fs_config);
	if (err) { return err; }

	err = usbd_register_all_classes(&rtframe_usbd, USBD_SPEED_FS, 1, NULL);
	if (err) { return err; }

	usbd_device_set_code_triple(&rtframe_usbd, USBD_SPEED_FS,
				    USB_BCC_MISCELLANEOUS, 0x02, 0x01);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&rtframe_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&rtframe_usbd, USBD_SPEED_HS, &rtframe_hs_config);
		if (err) { return err; }

		err = usbd_register_all_classes(&rtframe_usbd, USBD_SPEED_HS, 1, NULL);
		if (err) { return err; }

		usbd_device_set_code_triple(&rtframe_usbd, USBD_SPEED_HS,
					    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	}

	err = usbd_msg_register_cb(&rtframe_usbd, usbd_msg_cb);
	if (err) { return err; }

	err = usbd_init(&rtframe_usbd);
	if (err) {
		LOG_ERR("usbd_init failed: %d", err);
		return err;
	}

	if (!usbd_can_detect_vbus(&rtframe_usbd)) {
		err = usbd_enable(&rtframe_usbd);
		if (err) {
			LOG_ERR("usbd_enable failed: %d", err);
			return err;
		}
	}

	LOG_INF("ready");
	return 0;
}

SYS_INIT(usb_cdc_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
