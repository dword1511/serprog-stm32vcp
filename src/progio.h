#ifndef _PROGIO_H_
#define _PROGIO_H_

extern uint32_t spi_conf(uint32_t speed_hz);
extern void spi_bulk_write(uint32_t size);
extern void spi_bulk_read(uint32_t size);

extern void usb_sync(void);
extern void usb_putc(char data);
extern char usb_getc(void);
extern uint32_t usb_getu24(void);
extern uint32_t usb_getu32(void);
extern void usb_putu32(uint32_t ww);

#endif /* _PROGIO_H_ */
