#include <string.h>
#include "main.h"

#include "settingsMenu.h"
#include "settings.h"
#include "vtx_msp.h"
#include "canvas_char.h"
#include "rf_pa.h"
#include "video_overlay.h"
#include "msp_displayport.h"
#include "msp_fc.h"

#define OSD_MENU_TOP                2
#define OSD_MENU_TEXT_LEFT          ((COLUMN_SIZE - 26) / 2)
#define OSD_MENU_VALUE_LEFT         (OSD_MENU_TEXT_LEFT + 14 )

uint8_t tempChannel;
uint8_t tempBand;
uint8_t tempVideoInput;

extern CCMRAM_DATA bool show_logo;

void printMenuValue(uint8_t x, uint8_t y, uint8_t idx);
void changeChannel(ButtonEvent_e btn, uint8_t idx);
void changePower(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void changePit(ButtonEvent_e btn, uint8_t idx);
void changeDisplayport(ButtonEvent_e btn, uint8_t idx);
void changeVideoIn(ButtonEvent_e btn, uint8_t idx);

osdEntry_t osdMenue[] = { {"BAND",        (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeChannel},
                          {"CHANNEL",     (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeChannel},
                          {"FREQUENCY",   (osdPrintFuncPtr)printMenuValue,    NULL},
                          {"POWER",       (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changePower},
                          {"PIT MODE",    (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changePit},
                          {"DISPLAYPORT", (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeDisplayport},
                          #if (VIDEO1_INPUT_ENABLED == true && VIDEO2_INPUT_ENABLED == true)
                          {"VIDEO INPUT", (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeVideoIn},
                          #endif
                          {"EXIT",        NULL,                               (osdKeyFuncPtr)exitVtxMenu},
                          {"SAVE+EXIT",   NULL,                               (osdKeyFuncPtr)exitVtxMenu}};

#define MENUE_SIZE        (sizeof(osdMenue) / sizeof(osdMenue[0]))

void printMenuValue(uint8_t x, uint8_t y, uint8_t idx) {
  char buffer[20] = {0};

  switch (idx) {
    case 0:
      if(tempBand) {
        memcpy(buffer, vtx_get_band_name(tempBand - 1), 8);
      } else {
        sprintf(buffer, "DIRECT F");
      }
      break;
    case 1:
      sprintf(buffer, "%1i   ", tempChannel);
      break;
    case 2:
      if(tempBand) {
        sprintf(buffer, "%1i",vtx_get_frequency(tempBand - 1, tempChannel - 1) );
      } else {
        sprintf(buffer, "%1i",vtx_get_config()->frequency);
      }
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
      if (settings.displayportEnabled)
        sprintf(buffer, "ON ");
      else
        sprintf(buffer, "OFF");
      break;
    case 6:
      if (tempVideoInput == 0)
        sprintf(buffer, "INPUT 1 ");
      else if (tempVideoInput == 1)
        sprintf(buffer, "INPUT 2 ");
      else
        sprintf(buffer, "CAM CTRL");
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
        tempBand = ((tempBand) % vtx_get_band_count()) + 1;
      else
        tempBand = ((vtx_get_band_count() + tempBand - 2 ) % vtx_get_band_count()) + 1;
      break;
    case 1:
      if (btn == BTN_RIGHT)
        tempChannel = ((tempChannel ) % 8) + 1;
      else
        tempChannel = ((8 + tempChannel - 2) % 8) + 1;
      break;
    default:
      break;
  }
}

void changePower(ButtonEvent_e btn, uint8_t __attribute__((unused)) idx) {
  uint8_t power;

  if (btn == BTN_RIGHT)
    power = ((vtx_get_config()->power) % rf_pa_power_count()) + 1;
  else
    power = ((rf_pa_power_count() + vtx_get_config()->power - 2) % rf_pa_power_count()) + 1;
  vtx_set_power(power);
}

void changePit(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  
  vtx_set_pitmode(1 - vtx_get_config()->pitmode);
  TRACE_INFO("pitmode %i\r",vtx_get_config()->pitmode);
}

void changeDisplayport(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  settings.displayportEnabled = !settings.displayportEnabled;
}

void changeVideoIn(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  if (btn == BTN_RIGHT)
    tempVideoInput = (tempVideoInput + 1) % 3;
  else
    tempVideoInput = (3 + tempVideoInput -1) % 3;
  
  if(tempVideoInput == 0) {
    set_video_input(0);
    settings.activeVideoInput = 0;
    settings.camswitchEnabled = false;
  } if(tempVideoInput == 1) {
    set_video_input(1);
    settings.activeVideoInput = 1;
    settings.camswitchEnabled = false;
  } if(tempVideoInput == 2) {
    set_video_input(fc.status.cameraControl);
    settings.activeVideoInput = 0;
    settings.camswitchEnabled = true;
  }
}

void exitVtxMenu(ButtonEvent_e btn, uint8_t idx) {
  if (btn == BTN_RIGHT) {
    osdState = OSD_EXIT_MENU;
    if (idx == MENUE_SIZE - 1) {
      vtx_set_band_channel(tempBand, tempChannel);
      settings_save();
    }
  }
}


void msp_menu(void) {
  static ButtonEvent_e btn = BTN_INVALID;
  static ButtonEvent_e btnLast = BTN_INVALID;

  if      (fc.stickPos == 0x20)            btn = BTN_ENTER;
  else if (fc.stickPos == 0x10)            btn = BTN_EXIT;
  else if (fc.stickPos == 0x65)            btn = BTN_ENTER_VTX;
  else if ((fc.stickPos & 0x0f) == 0x00)   btn = BTN_MID;
  else if ((fc.stickPos & 0x0f) == 0x01)   btn = BTN_LEFT;
  else if ((fc.stickPos & 0x0f) == 0x02)   btn = BTN_RIGHT;
  else if ((fc.stickPos & 0x0f) == 0x04)   btn = BTN_DOWN;
  else if ((fc.stickPos & 0x0f) == 0x08)   btn = BTN_UP;
  else                                  btn = BTN_INVALID;

  static uint8_t selectedEntry = 0;

  if ((osdState == OSD_MSP || osdState == OSD_OFF) && (btn == BTN_ENTER_VTX) && !fc.status.armed) {
    osdState = OSD_MENU;
    selectedEntry = 0;
    btnLast = BTN_INVALID;
    tempChannel = vtx_get_config()->channel;
    tempBand = vtx_get_config()->band;
    show_logo = false;

    if (settings.camswitchEnabled)
      tempVideoInput = 2;
    else
      tempVideoInput = settings.activeVideoInput;

    TRACE_INFO("osdState = OSD_VTX %i\n", osdState);

    setSyncMode(AUTOMATIC);
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

  if ((osdState == OSD_MENU && fc.status.armed) || (osdState == OSD_EXIT_MENU)) {
    TRACE_INFO("osdState = OSD_MSP\n");
    canvas_char_clean();
    canvas_char_draw_complete();
    
    if (settings.displayportEnabled) {
      setSyncMode(AUTOMATIC);
      osdState = OSD_MSP;
    } else {
      setSyncMode(OFF);
      osdState = OSD_OFF;
    }
  }

  if (osdState != OSD_MENU) {
    btnLast = btn;
    return;
  }

  if (btnLast == BTN_MID && (btn == BTN_LEFT || btn == BTN_RIGHT)) {
    if (osdMenue[selectedEntry].keyFunc != NULL) {
      osdMenue[selectedEntry].keyFunc(btn, selectedEntry);
      for (uint8_t i = 0; i < MENUE_SIZE; i++) {
        if (osdState == OSD_MENU && osdMenue[i].printFunc != NULL)
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