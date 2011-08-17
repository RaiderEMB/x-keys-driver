#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "libxdo/xdo.h"
#include <libusb-1.0/libusb.h>

#define VENDOR_ID   0x05f3
#define PRODUCT_ID  0x0241

unsigned char shadow[14];
xdo_t* xdo;
long dvswitch_window;

static int find_dvswitch() {
	Window *list;
	xdo_search_t search;
	int nwindows;
	int i;

	memset(&search, 0, sizeof(xdo_search_t));
	search.max_depth = -1;
	search.require = SEARCH_ANY;
	search.searchmask |= SEARCH_NAME;
	search.searchmask |= SEARCH_ONLYVISIBLE;
	search.winname = "dvswitch";

	xdo_window_search(xdo, &search, &list, &nwindows);
	if (nwindows > 0) {
		dvswitch_window = list[0];
		printf("dvswitch window id: %ld\n", dvswitch_window);
	}
	free(list);

	return nwindows;
}

static void dvswitch(char *key) {
	if (!fork()) {
		char id[100];
		snprintf(id, 99, "%ld", dvswitch_window);
		execlp("xdotool", "xdotool", "key", "--window", id, key, NULL);
	}
}

int main(int argc, char **argv) {
	struct libusb_device_handle *usb_handle;
	int retval = 1;
	int i;
	char *display = getenv("DISPLAY");

	xdo = xdo_new(display);
	/* La XWin få roe nervene sine først */
	sleep(1);

	if (!find_dvswitch()) {
		fprintf(stderr, "Can't find dvswitch window\n");
		exit(1);
	}

	/* init usb */
	libusb_init(NULL);

	usb_handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if (usb_handle == NULL) {
		fprintf(stderr, "Device not found\n");
		exit(1);
	}

	libusb_detach_kernel_driver(usb_handle, 0);

	libusb_set_configuration(usb_handle, 1);

	printf("Claiming: %d\n", libusb_claim_interface(usb_handle, 0));

	{
		unsigned char data[14];
		int actual_length;
		int r;

		while (1) {
			r = libusb_interrupt_transfer(usb_handle, 0x81, data, sizeof(data), &actual_length, 0);
			if (r == 0 && actual_length == sizeof(data)) {
				printf("Data: %d %02X %02X %02d %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", (signed char)data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13]);

				if (data[3] & 0x01 && (shadow[3] & 0x01) == 0) {
					dvswitch("1");
				}
				if (data[4] & 0x01 && (shadow[4] & 0x01) == 0) {
					dvswitch("2");
				}
				if (data[5] & 0x01 && (shadow[5] & 0x01) == 0) {
					dvswitch("3");
				}
				if (data[6] & 0x01 && (shadow[6] & 0x01) == 0) {
					dvswitch("4");
				}

				memcpy(shadow, data, 14);
			} else {
				/* ignore for now */
			}
		}
	}
	retval = 0;

exit:
	libusb_release_interface(usb_handle, 0);
	libusb_close(usb_handle);
	return retval;
}

