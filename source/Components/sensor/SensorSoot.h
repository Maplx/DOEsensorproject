/******************************************************************************
*  Filename:       SensorSoot.h
*
*  Description:    Thin wrapper that plugs the soot-sensor resistance module
*                  (MCP4728 DAC + ADS1115 ADC, soot_resistance.c) into the
*                  SensorTag firmware's shared-I2C sensor framework.
*
*  Hardware:       Both chips sit on I2C bus 0 (SDA=DIO_5, SCL=DIO_6), reached
*                  through the Debug DevPack GROVE3 I2C connector.
*                  ADS1115 address 0x48, MCP4728 address 0x60.
******************************************************************************/
#ifndef SENSOR_SOOT_H
#define SENSOR_SOOT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint32_t resistance_ohms;   /* Clamp-to-clamp resistance in ohms. 0 = invalid */
  uint8_t  status;            /* SootResistance_Status value, 0 = OK           */
} SENSOR_SootData_t;

/* Program the MCP4728 bias and verify the ADS1115 answers. Call once from a
 * Task context after SensorI2C_open(). Returns true on success. */
bool SensorSoot_init(void);

/* Measure the clamp resistance. Always fills *data; returns false only when
 * the I2C bus could not be acquired or the module was never initialized. */
bool SensorSoot_oneShotRead(SENSOR_SootData_t *data);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_SOOT_H */
