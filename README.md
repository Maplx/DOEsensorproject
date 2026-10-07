# DOE Sensor Project – 6TiSCH testbed with soot-sensor integration

Firmware and gateway software for a CC2650 SensorTag based 6TiSCH wireless
sensor network, extended with an external soot-resistance sensor
(MCP4728 DAC + ADS1115 ADC on the shared I2C bus).

## Layout

| Path | Contents |
|---|---|
| `source/Projects/tsch/coap-cc26xx/` | IAR project for the node firmware (`CoAP.eww`, configuration **sensortag**) |
| `source/Components/` | Node source: `sensor/` drivers, `apps/coap`, `apps/webmsg` (packet builder), `tsch/`, `uip/`, `hal/` |
| `source/Components/sensor/soot_resistance.c` | Soot sensor driver (MCP4728 + ADS1115), from <https://github.com/Polar-Slater/soot_sensor> |
| `source/Components/sensor/SensorSoot.c` | Wrapper that plugs the soot driver into the firmware's shared-I2C framework |
| `source/Projects/tsch/lib/tsch-int-cc26xx.a` | Prebuilt TSCH MAC library (built from `source/Components/tsch` by the `lib-gen` project) |
| `bin/hex/coap_cc26xx/CoAP-sensortag.hex` | Last built firmware image, flash with IAR or TI UniFlash via the Debug DevPack |
| `bin/webapp/` | Node.js gateway / web front-end (`app.js`) |

## Toolchain (must match exactly)

* IAR Embedded Workbench for ARM **8.22.1**
* TI-RTOS for CC13xx/CC26xx **2.21.00.06** installed to `c:/ti/tirtos_cc13xx_cc26xx_2_21_00_06`
* XDCtools **3.32.00.06** installed to `c:/ti/xdctools_3_32_00_06_core`

The installers are not in this repository (GitHub file-size limit); download them
from ti.com. Paths are set in `source/Projects/tsch/coap-cc26xx/CoAP.custom_argvars`.

Only flashing a prebuilt `.hex`? You need neither IAR nor the SDKs: use TI UniFlash
with the SensorTag Debug DevPack (XDS110).

## Soot sensor data path

`SensorSoot_init()` (called from `coap_task()` at boot) programs the MCP4728 bias and
checks the ADS1115. Each periodic report in `webmsg_readSensor()` calls
`SensorSoot_oneShotRead()` and appends two TLVs to the CoAP `sensors` payload:

| TLV type | Length | Meaning |
|---|---|---|
| `0x24` | 4 | clamp resistance, uint32 ohms (0 = invalid) |
| `0x25` | 1 | status code, 0 = OK (see `soot_resistance.h`) |

The gateway parses both in `obs_sensor_parse_handler()` (`bin/webapp/app.js`) and
stores them as `soot_ohms` / `soot_status`.

## Web front-end setup

Google Maps keys are not committed. Copy
`bin/webapp/iot/lib/maps_key.example.js` to `maps_key.js` in the same folder and
fill in your own keys. Keep that file out of git (it is already in `.gitignore`).
