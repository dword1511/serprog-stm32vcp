#include <stm32/gpio.h>
#include <stm32/usb/lib.h>
#include "usb_istr.h"
#include "config.h"

/* This file handles USB interrupts. */

__IO uint16_t wIstr;                         /* ISTR register last read value */
__IO uint8_t  bIntPackSOF     = 0;           /* SOFs received between 2 consecutive packets */
__IO uint32_t bDeviceState    = UNCONNECTED; /* USB device status */
__IO bool     fSuspendEnabled = TRUE;        /* true when suspend is possible */

struct {
  __IO RESUME_STATE eState;
  __IO uint8_t      bESOFcnt;
} ResumeS;

void (*pEpInt_IN[7])(void) = {
  EP1_IN_Callback,
  EP2_IN_Callback,
  EP3_IN_Callback,
  EP4_IN_Callback,
  EP5_IN_Callback,
  EP6_IN_Callback,
  EP7_IN_Callback,
};

void (*pEpInt_OUT[7])(void) = {
  EP1_OUT_Callback,
  EP2_OUT_Callback,
  EP3_OUT_Callback,
  EP4_OUT_Callback,
  EP5_OUT_Callback,
  EP6_OUT_Callback,
  EP7_OUT_Callback,
};

RESULT PowerOn(void) {
  uint16_t wRegVal;

  /*** CNTR_PWDN = 0 ***/
  wRegVal = CNTR_FRES;
  _SetCNTR(wRegVal);

  /*** CNTR_FRES = 0 ***/
  wInterrupt_Mask = 0;
  _SetCNTR(wInterrupt_Mask);
  /*** Clear pending interrupts ***/
  _SetISTR(0);
  /*** Set interrupt mask ***/
  wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
  _SetCNTR(wInterrupt_Mask);

  return USB_SUCCESS;
}

RESULT PowerOff(void) {
  /* disable all interrupts and force USB reset */
  _SetCNTR(CNTR_FRES);
  /* clear interrupt status register */
  _SetISTR(0);
  /* switch-off device */
  _SetCNTR(CNTR_FRES + CNTR_PDWN);

  led_off();
  return USB_SUCCESS;
}

void Suspend(void) {
  uint16_t wCNTR;

  /* macrocell enters suspend mode */
  wCNTR = _GetCNTR();
  wCNTR |= CNTR_FSUSP;
  _SetCNTR(wCNTR);

  /* force low-power mode in the macrocell */
  wCNTR = _GetCNTR();
  wCNTR |= CNTR_LPMODE;
  _SetCNTR(wCNTR);

  led_off();
  bDeviceState = SUSPENDED;
}

void Resume_Init(void) {
  uint16_t wCNTR;

  /* CNTR_LPMODE = 0 */
  wCNTR = _GetCNTR();
  wCNTR &= (~CNTR_LPMODE);
  _SetCNTR(wCNTR);

  DEVICE_INFO *pInfo = &Device_Info;
  /* Set the device state to the correct state */
  /* Device configured */
  if (pInfo->Current_Configuration != 0) bDeviceState = CONFIGURED;
  else bDeviceState = ATTACHED;

  /* reset FSUSP bit */
  _SetCNTR(IMR_MSK);
}

void Resume(RESUME_STATE eResumeSetVal) {
  uint16_t wCNTR;

  if(eResumeSetVal != RESUME_ESOF) ResumeS.eState = eResumeSetVal;

  switch(ResumeS.eState) {
    case RESUME_EXTERNAL:
      Resume_Init();
      ResumeS.eState = RESUME_OFF;
      break;
    case RESUME_INTERNAL:
      Resume_Init();
      ResumeS.eState = RESUME_START;
      break;
    case RESUME_LATER:
      ResumeS.bESOFcnt = 2;
      ResumeS.eState = RESUME_WAIT;
      break;
    case RESUME_WAIT:
      ResumeS.bESOFcnt--;
      if(ResumeS.bESOFcnt == 0) ResumeS.eState = RESUME_START;
      break;
    case RESUME_START:
      wCNTR = _GetCNTR();
      wCNTR |= CNTR_RESUME;
      _SetCNTR(wCNTR);
      ResumeS.eState = RESUME_ON;
      ResumeS.bESOFcnt = 10;
      break;
    case RESUME_ON:
      ResumeS.bESOFcnt--;
      if(ResumeS.bESOFcnt == 0) {
        wCNTR = _GetCNTR();
        wCNTR &= (~CNTR_RESUME);
        _SetCNTR(wCNTR);
        ResumeS.eState = RESUME_OFF;
      }
      break;
    case RESUME_OFF:
    case RESUME_ESOF:
    default:
      ResumeS.eState = RESUME_OFF;
      break;
  }
}

void USB_Istr(void) {
  wIstr = _GetISTR();

#if (IMR_MSK & ISTR_SOF)
  if (wIstr & ISTR_SOF & wInterrupt_Mask) {
    _SetISTR((uint16_t)CLR_SOF);
    bIntPackSOF++;
    #ifdef SOF_CALLBACK
      SOF_Callback();
    #endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/  
#if (IMR_MSK & ISTR_CTR)
  if (wIstr & ISTR_CTR & wInterrupt_Mask) {
    /* servicing of the endpoint correct transfer interrupt */
    /* clear of the CTR flag into the sub */
    CTR_LP();
    #ifdef CTR_CALLBACK
      CTR_Callback();
    #endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/  
#if (IMR_MSK & ISTR_RESET)
  if (wIstr & ISTR_RESET & wInterrupt_Mask) {
    _SetISTR((uint16_t)CLR_RESET);
    Device_Property.Reset();
    #ifdef RESET_CALLBACK
      RESET_Callback();
    #endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_DOVR)
  if (wIstr & ISTR_DOVR & wInterrupt_Mask) {
    _SetISTR((uint16_t)CLR_DOVR);
    #ifdef DOVR_CALLBACK
      DOVR_Callback();
    #endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_ERR)
  if (wIstr & ISTR_ERR & wInterrupt_Mask) {
    _SetISTR((uint16_t)CLR_ERR);
    #ifdef ERR_CALLBACK
      ERR_Callback();
    #endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_WKUP)
  if (wIstr & ISTR_WKUP & wInterrupt_Mask) {
    _SetISTR((uint16_t)CLR_WKUP);
    Resume(RESUME_EXTERNAL);
    #ifdef WKUP_CALLBACK
      WKUP_Callback();
    #endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_SUSP)
  if(wIstr & ISTR_SUSP & wInterrupt_Mask) {
    /* check if SUSPEND is possible */
    if(fSuspendEnabled) Suspend();
    /* if not possible then resume after xx ms */
    else Resume(RESUME_LATER);
    /* clear of the ISTR bit must be done after setting of CNTR_FSUSP */
    _SetISTR((uint16_t)CLR_SUSP);
    #ifdef SUSP_CALLBACK
      SUSP_Callback();
    #endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_ESOF)
  if (wIstr & ISTR_ESOF & wInterrupt_Mask) {
    _SetISTR((uint16_t)CLR_ESOF);
    /* resume handling timing is made with ESOFs */
    Resume(RESUME_ESOF); /* request without change of the machine state */

    #ifdef ESOF_CALLBACK
      ESOF_Callback();
    #endif
  }
#endif

}
