#include "main.h"
#include <stdbool.h>
#include <string.h>
#include "rtc6705.h"
#include "rf_pa.h"
#include "vtx_msp.h"

powerTable_t powerTable[] = { {0,  {' ', ' ', '0'}, RTC6705_PA_3dBm,  { 5650, 5700, 5750, 5800, 5850, 5900, 5950 },
                                                                      {    0,    0,    0,    0,    0,    0,    0 }},
                              {1,  {' ', ' ', '1'}, RTC6705_PA_3dBm,  {    1,    1,    1,    1,    1,    1,    1 },
                                                                      {    0,    0,    0,    0,    0,    0,    0 }},
                              {10, {' ', '1', '0'}, RTC6705_PA_7dBm,  {    1,    1,    1,    1,    1,    1,    1 }, 
                                                                      {    0,    0,    0,    0,    0,    0,    0 }}, 
                              {25, {' ', '2', '5'}, RTC6705_PA_7dBm,  {    1,    1,    1,    1,    1,    1,    1 }, 
                                                                      {    0,    0,    0,    0,    0,    0,    0 }}, 
                              {50, {' ', '5', '0'}, RTC6705_PA_11dBm, {    1,    1,    1,    1,    1,    1,    1 }, 
                                                                      {    0,    0,    0,    0,    0,    0,    0 }},
                            };

/* VTX bands table (letter + 8-char name + 8 channel freqs (MHz)).
 * These are standard bands used in Betaflight and iNav.
 * You can add custom bands here if needed. */
const vtx_band_t g_bands[] = {
    /* Band A (Boscam A) */
    { 'A', { 'B','O','S','C','A','M',' ','A' },
      { 5865,5845,5825,5805,5785,5765,5745,5725 } },

    /* Band B (Boscam B) */
    { 'B', { 'B','O','S','C','A','M',' ','B' },
      { 5733,5752,5771,5790,5809,5828,5847,5866 } },

    /* Band F (FatShark) */
    { 'F', { 'F','A','T','S','H','A','R','K' },
      { 5740,5760,5780,5800,5820,5840,5860,5880 } },

    /* Band R (Raceband) */
    { 'R', { 'R','A','C','E','B','A','N','D' },
      { 5658,5695,5732,5769,5806,5843,5880,5917 } },
};

#define NUM_PWR             (sizeof(powerTable)/sizeof(powerTable[0]) - 1)
#define NUM_BANDS           (sizeof(g_bands)/sizeof(g_bands[0]))

uint8_t vtx_get_band_count(void)
{
    return NUM_BANDS;
}

uint8_t rf_pa_power_count(void)
{
    return NUM_PWR;
}
