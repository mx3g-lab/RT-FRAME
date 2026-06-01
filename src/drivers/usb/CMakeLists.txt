if(CONFIG_RTFRAME_USB_CDC)
    zephyr_sources_ifdef(CONFIG_RTFRAME_USB_CDC usb_cdc.c)
endif()
