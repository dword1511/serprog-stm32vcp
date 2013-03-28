#ifndef __USB_CONF_H
#define __USB_CONF_H

/* THIS FILE IS REUIRED BY LIMSTM32USB. */

/* EP_NUM */
/* defines how many endpoints are used by the device */
#define EP_NUM              (4)

/* buffer table base address */
#define BTABLE_ADDRESS      (0x00)

/* EP0  */
/* rx/tx buffer base address */
#define ENDP0_RXADDR        (0x40)
#define ENDP0_TXADDR        (0x80)

/* EP1  */
/* tx buffer base address */
#define ENDP1_TXADDR        (0xC0)

/* EP2  */
/* tx buffer base address */
#define ENDP2_TXADDR        (0x100)

/* EP3  */
/* rx buffer base address */
#define ENDP3_RXADDR        (0x110)

/* IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */
#define IMR_MSK (CNTR_CTRM  | CNTR_SOFM  | CNTR_RESETM )

//#define CTR_CALLBACK
//#define DOVR_CALLBACK
//#define ERR_CALLBACK
//#define WKUP_CALLBACK
//#define SUSP_CALLBACK
//#define RESET_CALLBACK
//#define SOF_CALLBACK
//#define ESOF_CALLBACK

/* CTR service routines */
/* associated to defined endpoints */
#define  EP1_IN_Callback   NOP_Process
#define  EP2_IN_Callback   NOP_Process
#define  EP3_IN_Callback   NOP_Process
#define  EP4_IN_Callback   NOP_Process
#define  EP5_IN_Callback   NOP_Process
#define  EP6_IN_Callback   NOP_Process
#define  EP7_IN_Callback   NOP_Process

#define  EP1_OUT_Callback   NOP_Process
#define  EP2_OUT_Callback   NOP_Process
#define  EP3_OUT_Callback   NOP_Process
#define  EP4_OUT_Callback   NOP_Process
#define  EP5_OUT_Callback   NOP_Process
#define  EP6_OUT_Callback   NOP_Process
#define  EP7_OUT_Callback   NOP_Process

#endif /* __USB_CONF_H */
