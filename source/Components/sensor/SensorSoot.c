/******************************************************************************
*  Filename:       SensorSoot.c
*
*  Description:    Wrapper around soot_resistance.c so it follows the same
*                  select -> access -> deselect pattern as the other SensorTag
*                  sensor drivers and shares the existing I2C bus/mutex.
*
*  Why the wrapper is needed:
*    - soot_resistance.c wants a raw I2C_Handle; SensorI2C.c keeps that handle
*      private and guards the bus with a semaphore.
*    - The MPU9250 lives on a second set of I2C pins (interface 1). After an
*      MPU read the bus is still switched to those pins. SensorI2C_select()
*      with SENSOR_I2C_0 switches it back to DIO_5/DIO_6 where the ADS1115 and
*      MCP4728 are, so it must be called before every access.
******************************************************************************/

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
#include "Board.h"
#include "SensorI2C.h"
#include "SensorSoot.h"
#include "soot_resistance.h"

/* -----------------------------------------------------------------------------
*  Constants
* ------------------------------------------------------------------------------
*/
#define SOOT_ADS1115_ADDR   0x48   /* ADDR pin tied to GND                   */
#define SOOT_MCP4728_ADDR   0x60   /* Adafruit default; some A4 boards: 0x64 */

/* Number of ADC conversions averaged per reading. soot_resistance.c defaults
 * to 16, which takes roughly 300 ms per report. Lower this if the CoAP task
 * must not block that long. */
#define SOOT_SAMPLE_COUNT   4

/* The bus lock is taken with the ADS1115 address; soot_resistance.c passes the
 * real target address on every transfer itself, so this value is only used
 * to satisfy SensorI2C_select(). */
#define SENSOR_SELECT()     SensorI2C_select(SENSOR_I2C_0, SOOT_ADS1115_ADDR)
#define SENSOR_DESELECT()   SensorI2C_deselect()

/* -----------------------------------------------------------------------------
*  Local variables
* ------------------------------------------------------------------------------
*/
static SootResistance soot;
static bool sootReady = false;

/*******************************************************************************
* @fn          SensorSoot_init
*
* @brief       Configure the MCP4728 bias output and check the ADS1115 ACKs.
*
* @return      true if both chips answered and the bias was programmed
*/
bool SensorSoot_init(void)
{
  SootResistance_Config cfg;
  SootResistance_Status status;

  sootReady = false;

  if (!SENSOR_SELECT())
  {
    return false;
  }

  SootResistance_configDefaults(&cfg, SensorI2C_getHandle());
  cfg.mcp4728_address = SOOT_MCP4728_ADDR;
  cfg.ads1115_address = SOOT_ADS1115_ADDR;
  cfg.sample_count    = SOOT_SAMPLE_COUNT;

  status = SootResistance_init(&soot, &cfg);

  SENSOR_DESELECT();

  sootReady = (status == SOOT_RESISTANCE_STATUS_OK);
  return sootReady;
}

/*******************************************************************************
* @fn          SensorSoot_oneShotRead
*
* @brief       Run one averaged resistance measurement.
*
* @param       data - output: resistance in ohms (0 if invalid) and status code
*
* @return      true if the measurement was attempted, false if the bus could
*              not be acquired or the module is not initialized
*/
bool SensorSoot_oneShotRead(SENSOR_SootData_t *data)
{
  SootResistance_Measurement m;
  SootResistance_Status status;

  data->resistance_ohms = 0;
  data->status = (uint8_t)SOOT_RESISTANCE_STATUS_NOT_INITIALIZED;

  if (!sootReady)
  {
    return false;
  }

  if (!SENSOR_SELECT())
  {
    return false;
  }

  /* The handle can be re-opened by SensorI2C_select() when switching pins;
   * refresh it every time so soot_resistance.c always uses the live one. */
  soot.config.i2c = SensorI2C_getHandle();

  status = SootResistance_read(&soot, &m);

  SENSOR_DESELECT();

  data->status = (uint8_t)status;
  if (status == SOOT_RESISTANCE_STATUS_OK)
  {
    if (m.resistance_ohms < 0.0f)
    {
      data->resistance_ohms = 0;
    }
    else if (m.resistance_ohms > 4294967040.0f)
    {
      data->resistance_ohms = 0xFFFFFFFFu;
    }
    else
    {
      data->resistance_ohms = (uint32_t)(m.resistance_ohms + 0.5f);
    }
  }

  return true;
}
