#ifndef _USB_MSC_H_
#define _USB_MSC_H_

// re-insert the medium on all LUNs, undoing a previous host eject
void usb_msc_reset_eject(void);

#endif	// _USB_MSC_H_
