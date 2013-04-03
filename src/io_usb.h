#ifndef __IO_USB_H__
#define __IO_USB_H__

extern void     usb_putc(char data);
extern char     usb_getc(void);
extern uint32_t usb_getu24(void);
extern uint32_t usb_getu32(void);
extern void     usb_putu32(uint32_t ww);
extern void     usb_sync(void);

#endif /* __IO_USB_H__ */
