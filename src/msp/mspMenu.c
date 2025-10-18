#include <string.h>
#include "main.h"

#include "mspMenu.h"
#include "vtx_msp.h"
#include "canvas_char.h"
#include "rf_pa.h"

#define OSD_MENU_TOP                2
#define OSD_MENU_TEXT_LEFT          2
#define OSD_MENU_VALUE_LEFT         16

osdState_e osdState = OSD_MSP;

uint8_t tempChannel;
uint8_t tempBand;
uint8_t tempDisplayport;

uint16_t rcChannel[4] = {1500};
uint8_t stickPos = 0;
uint8_t fcArmed = 0;

extern CCMRAM_DATA bool show_logo;

void printMenuValue(uint8_t x, uint8_t y, uint8_t idx);
void changeChannel(ButtonEvent_e btn, uint8_t idx);
void changePower(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void changePit(ButtonEvent_e btn, uint8_t idx);
void changeDisplayport(ButtonEvent_e btn, uint8_t idx);

osdEntry_t osdMenue[] = { {"BAND",        (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeChannel},
                          {"CHANNEL",     (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeChannel},
                          {"FREQUENCY",   (osdPrintFuncPtr)printMenuValue,    NULL},
                          {"POWER",       (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changePower},
                          {"PIT MODE",    (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changePit},
                          //{"DISPLAYPORT", (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeDisplayport},
                          {"EXIT",        NULL,                               (osdKeyFuncPtr)exitVtxMenu},
                          {"SAVE+EXIT",   NULL,                               (osdKeyFuncPtr)exitVtxMenu}};

#define MENUE_SIZE        (sizeof(osdMenue) / sizeof(osdMenue[0]))

void printMenuValue(uint8_t x, uint8_t y, uint8_t idx) {
  char buffer[20] = {0};

  switch (idx) {
    case 0:
      memcpy(buffer, vtx_get_band_name(tempBand), 8);
      break;
    case 1:
      sprintf(buffer, "%1i", tempChannel + 1);
      break;
    case 2:
      sprintf(buffer, "%1i",vtx_get_frequency(tempBand, tempChannel) );
      break;
    case 3:
      sprintf(buffer, "%i MW  ",vtx_get_power_mw() );
      break;
    case 4:
      if (vtx_get_config()->pitmode)
        sprintf(buffer, "ON ");
      else
        sprintf(buffer, "OFF");
      break;
    case 5:
      if (tempDisplayport)
        sprintf(buffer, "ON ");
      else
        sprintf(buffer, "OFF");
      break;
    default:
      break;
  }
  canvas_print(x, y, buffer);
  canvas_char_draw_complete();
}

void changeChannel(ButtonEvent_e btn, uint8_t idx) {
  switch (idx) {
    case 0:
       if (btn == BTN_RIGHT)
        tempBand = (tempBand + 1) % vtx_get_band_count();
      else
        tempBand = (vtx_get_band_count() + tempBand - 1) % vtx_get_band_count();
      break;
    case 1:
      if (btn == BTN_RIGHT)
        tempChannel = (tempChannel + 1) % 8;
      else
        tempChannel = (8 + tempChannel - 1) % 8;
      break;
    default:
      break;
  }
}

void changePower(ButtonEvent_e btn, uint8_t __attribute__((unused)) idx) {
  uint8_t power;

  if (btn == BTN_RIGHT)
    power = (vtx_get_config()->power + 1) % rf_pa_power_count();
  else
    power = (rf_pa_power_count() + vtx_get_config()->power - 1) % rf_pa_power_count();
  vtx_set_power(power + 1);
}

void changePit(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  
  vtx_set_pitmode(1 - vtx_get_config()->pitmode);
  TRACE_INFO("pitmode %i\r",vtx_get_config()->pitmode);
}

void changeDisplayport(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  /*if (tempDisplayport) {
    tempDisplayport = 0;
    setSyncMode(INTERNAL);
  } else {
    tempDisplayport = 1;
    setSyncMode(AUTOMATIC);
  }*/
}

void exitVtxMenu(ButtonEvent_e btn, uint8_t idx) {
  if (btn == BTN_RIGHT) {
    osdState = OSD_EXIT_VTX;
    if (idx == MENUE_SIZE - 1) {
      vtx_set_band_channel(tempBand, tempChannel);
    }
  }
}


void msp_menu(void) {
  static ButtonEvent_e btn = BTN_INVALID;
  static ButtonEvent_e btnLast = BTN_INVALID;

  if      (stickPos == 0x20)            btn = BTN_ENTER;
  else if (stickPos == 0x10)            btn = BTN_EXIT;
  else if (stickPos == 0x65)            btn = BTN_ENTER_VTX;
  else if ((stickPos & 0x0f) == 0x00)   btn = BTN_MID;
  else if ((stickPos & 0x0f) == 0x01)   btn = BTN_LEFT;
  else if ((stickPos & 0x0f) == 0x02)   btn = BTN_RIGHT;
  else if ((stickPos & 0x0f) == 0x04)   btn = BTN_DOWN;
  else if ((stickPos & 0x0f) == 0x08)   btn = BTN_UP;
  else                                  btn = BTN_INVALID;

  static uint8_t selectedEntry = 0;

  if ((osdState == OSD_MSP) && (btn == BTN_ENTER_VTX)) {
    osdState = OSD_VTX;
    selectedEntry = 0;
    btnLast = BTN_INVALID;
    tempChannel = vtx_get_config()->channel;
    tempBand = vtx_get_config()->band;
    tempDisplayport = 1;
    show_logo = false;

    TRACE_INFO("osdState = OSD_VTX %i\n", osdState);

    canvas_char_clean();
    for (uint8_t i = 0; i < MENUE_SIZE; i++) {
      canvas_print(OSD_MENU_TEXT_LEFT, OSD_MENU_TOP + i, osdMenue[i].text);
      if (i == selectedEntry)
        canvas_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + i, ">");
      if (osdMenue[i].printFunc != NULL) {
        osdMenue[i].printFunc(OSD_MENU_VALUE_LEFT ,OSD_MENU_TOP + i, i);
        if (osdMenue[i].keyFunc != NULL) {
          canvas_print(OSD_MENU_VALUE_LEFT - 2, OSD_MENU_TOP + i, "<");
          canvas_print(OSD_MENU_VALUE_LEFT + 9, OSD_MENU_TOP + i, ">");
        }
      }
    }
    canvas_char_draw_complete();
    
  }

  if ((osdState == OSD_VTX && fcArmed) || (osdState == OSD_EXIT_VTX)) {
    TRACE_INFO("osdState = OSD_MSP\n");
    canvas_char_clean();
    canvas_char_draw_complete();
    osdState = OSD_MSP;
    /*if (myEEPROM.displayport)
      setSyncMode(AUTOMATIC);
    else
      setSyncMode(OFF);
    */
  }

  if (osdState != OSD_VTX) {
    btnLast = btn;
    return;
  }

  if (btnLast == BTN_MID && (btn == BTN_LEFT || btn == BTN_RIGHT)) {
    if (osdMenue[selectedEntry].keyFunc != NULL) {
      osdMenue[selectedEntry].keyFunc(btn, selectedEntry);
      for (uint8_t i = 0; i < MENUE_SIZE; i++) {
        if (osdState == OSD_VTX && osdMenue[i].printFunc != NULL)
          osdMenue[i].printFunc(OSD_MENU_VALUE_LEFT ,OSD_MENU_TOP + i, i);
      }
    }
  }

  if (btnLast == BTN_MID && (btn == BTN_DOWN || btn == BTN_UP)) {
    canvas_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + selectedEntry, " ");
    if (btn == BTN_DOWN)
      selectedEntry = (selectedEntry + 1) % MENUE_SIZE;
    else
      selectedEntry = (MENUE_SIZE + selectedEntry - 1) % MENUE_SIZE;
    canvas_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + selectedEntry, ">");
    canvas_char_draw_complete();
  }

  btnLast = btn;
}