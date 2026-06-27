#include "bmp280.h"

#include "main.h"
#include <stdio.h>

#define BMP280_REG_CALIB_START       0x88U
#define BME280_REG_H1                0xA1U
#define BME280_REG_HUM_CALIB_START   0xE1U
#define BMP280_REG_CHIP_ID           0xD0U
#define BMP280_REG_DATA_START        0xF7U
#define BMP280_REG_CTRL_HUM          0xF2U
#define BMP280_REG_CTRL_MEAS         0xF4U
#define BMP280_REG_CONFIG            0xF5U

#define BMP280_CALIB_LENGTH          24U
#define BMP280_DATA_LENGTH           6U
#define BME280_DATA_LENGTH           8U
#define BME280_HUM_CALIB_LENGTH      7U
#define BMP280_I2C_TIMEOUT_MS        100U

/* Temperature x2, pressure x16, normal mode. */
#define BMP280_CTRL_MEAS_VALUE       0x57U
#define BME280_CTRL_HUM_VALUE        0x01U
/* 1000 ms standby, filter x4, 3-wire SPI disabled. */
#define BMP280_CONFIG_VALUE          0xA8U

typedef struct
{
  uint16_t dig_t1;
  int16_t dig_t2;
  int16_t dig_t3;
  uint16_t dig_p1;
  int16_t dig_p2;
  int16_t dig_p3;
  int16_t dig_p4;
  int16_t dig_p5;
  int16_t dig_p6;
  int16_t dig_p7;
  int16_t dig_p8;
  int16_t dig_p9;
  uint8_t dig_h1;
  int16_t dig_h2;
  uint8_t dig_h3;
  int16_t dig_h4;
  int16_t dig_h5;
  int8_t dig_h6;
  int32_t t_fine;
  uint8_t chip_id;
  uint8_t humidity_calibrated;
  uint8_t initialized;
} BMP280_Calibration_t;

extern I2C_HandleTypeDef hi2c2;

static BMP280_Calibration_t bmp280_calibration;

static uint8_t BMP280_ReadRegisters(uint8_t reg, uint8_t *data, uint16_t length)
{
  return (HAL_I2C_Mem_Read(&hi2c2,
                           (uint16_t)BMP280_ADDR_HAL,
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           data,
                           length,
                           BMP280_I2C_TIMEOUT_MS) == HAL_OK) ? 1U : 0U;
}

static uint8_t BMP280_WriteRegister(uint8_t reg, uint8_t value)
{
  return (HAL_I2C_Mem_Write(&hi2c2,
                            (uint16_t)BMP280_ADDR_HAL,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            &value,
                            1U,
                            BMP280_I2C_TIMEOUT_MS) == HAL_OK) ? 1U : 0U;
}

static uint16_t BMP280_ReadU16LE(const uint8_t *data)
{
  return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static int16_t BMP280_ReadS16LE(const uint8_t *data)
{
  return (int16_t)BMP280_ReadU16LE(data);
}

static int16_t BME280_SignExtend12(uint16_t value)
{
  value &= 0x0FFFU;
  if ((value & 0x0800U) != 0U)
  {
    value |= 0xF000U;
  }

  return (int16_t)value;
}

static int32_t BMP280_CompensateTemperature(int32_t adc_temperature)
{
  int32_t var1;
  int32_t var2;

  var1 = (int32_t)((((adc_temperature >> 3) -
                     ((int32_t)bmp280_calibration.dig_t1 * 2)) *
                    (int32_t)bmp280_calibration.dig_t2) >> 11);
  var2 = (int32_t)(((((((adc_temperature >> 4) -
                         (int32_t)bmp280_calibration.dig_t1) *
                        ((adc_temperature >> 4) -
                         (int32_t)bmp280_calibration.dig_t1)) >> 12) *
                      (int32_t)bmp280_calibration.dig_t3)) >> 14);
  bmp280_calibration.t_fine = var1 + var2;

  return (bmp280_calibration.t_fine * 5 + 128) >> 8;
}

static uint8_t BMP280_CompensatePressure(int32_t adc_pressure, uint32_t *pressure_q24_8)
{
  int64_t var1;
  int64_t var2;
  int64_t pressure;

  if (pressure_q24_8 == NULL)
  {
    return 0U;
  }

  var1 = (int64_t)bmp280_calibration.t_fine - 128000LL;
  var2 = var1 * var1 * (int64_t)bmp280_calibration.dig_p6;
  var2 += var1 * (int64_t)bmp280_calibration.dig_p5 * 131072LL;
  var2 += (int64_t)bmp280_calibration.dig_p4 * 34359738368LL;
  var1 = ((var1 * var1 * (int64_t)bmp280_calibration.dig_p3) >> 8) +
         (var1 * (int64_t)bmp280_calibration.dig_p2 * 4096LL);
  var1 = ((140737488355328LL + var1) *
          (int64_t)bmp280_calibration.dig_p1) >> 33;

  if (var1 == 0LL)
  {
    return 0U;
  }

  pressure = 1048576LL - (int64_t)adc_pressure;
  pressure = (((pressure * 2147483648LL) - var2) * 3125LL) / var1;
  var1 = ((int64_t)bmp280_calibration.dig_p9 *
          (pressure >> 13) * (pressure >> 13)) >> 25;
  var2 = ((int64_t)bmp280_calibration.dig_p8 * pressure) >> 19;
  pressure = ((pressure + var1 + var2) >> 8) +
             ((int64_t)bmp280_calibration.dig_p7 * 16LL);

  if (pressure < 0LL)
  {
    return 0U;
  }

  *pressure_q24_8 = (uint32_t)pressure;
  return 1U;
}

static uint32_t BME280_CompensateHumidity(int32_t adc_humidity)
{
  int32_t value;

  value = bmp280_calibration.t_fine - 76800;
  value = (((((adc_humidity * 16384) -
              ((int32_t)bmp280_calibration.dig_h4 * 1048576) -
              ((int32_t)bmp280_calibration.dig_h5 * value)) + 16384) >> 15) *
           (((((((value * (int32_t)bmp280_calibration.dig_h6) >> 10) *
                (((value * (int32_t)bmp280_calibration.dig_h3) >> 11) + 32768)) >> 10) +
              2097152) * (int32_t)bmp280_calibration.dig_h2 + 8192) >> 14));
  value -= (((((value >> 15) * (value >> 15)) >> 7) *
             (int32_t)bmp280_calibration.dig_h1) >> 4);

  if (value < 0)
  {
    value = 0;
  }
  else if (value > 419430400)
  {
    value = 419430400;
  }

  return (uint32_t)(value >> 12);
}

uint8_t BMP280_Init(void)
{
  uint8_t chip_id;
  uint8_t calibration[BMP280_CALIB_LENGTH];
  uint8_t humidity_calibration[BME280_HUM_CALIB_LENGTH];

  bmp280_calibration.initialized = 0U;
  bmp280_calibration.chip_id = 0U;
  bmp280_calibration.humidity_calibrated = 0U;

  if (BMP280_ReadRegisters(BMP280_REG_CHIP_ID, &chip_id, 1U) == 0U)
  {
    printf("[BMP280] init failed: chip id read\r\n");
    return 0U;
  }

  if ((chip_id != BMP280_CHIP_ID) && (chip_id != BME280_CHIP_ID))
  {
    printf("[BMP/BME280] init failed: chip_id=0x%02X\r\n", chip_id);
    return 0U;
  }

  if (BMP280_ReadRegisters(BMP280_REG_CALIB_START,
                           calibration,
                           sizeof(calibration)) == 0U)
  {
    printf("[BMP280] init failed: calibration read\r\n");
    return 0U;
  }

  bmp280_calibration.dig_t1 = BMP280_ReadU16LE(&calibration[0]);
  bmp280_calibration.dig_t2 = BMP280_ReadS16LE(&calibration[2]);
  bmp280_calibration.dig_t3 = BMP280_ReadS16LE(&calibration[4]);
  bmp280_calibration.dig_p1 = BMP280_ReadU16LE(&calibration[6]);
  bmp280_calibration.dig_p2 = BMP280_ReadS16LE(&calibration[8]);
  bmp280_calibration.dig_p3 = BMP280_ReadS16LE(&calibration[10]);
  bmp280_calibration.dig_p4 = BMP280_ReadS16LE(&calibration[12]);
  bmp280_calibration.dig_p5 = BMP280_ReadS16LE(&calibration[14]);
  bmp280_calibration.dig_p6 = BMP280_ReadS16LE(&calibration[16]);
  bmp280_calibration.dig_p7 = BMP280_ReadS16LE(&calibration[18]);
  bmp280_calibration.dig_p8 = BMP280_ReadS16LE(&calibration[20]);
  bmp280_calibration.dig_p9 = BMP280_ReadS16LE(&calibration[22]);

  if (bmp280_calibration.dig_p1 == 0U)
  {
    printf("[BMP280] init failed: invalid calibration\r\n");
    return 0U;
  }

  if (chip_id == BME280_CHIP_ID)
  {
    if ((BMP280_ReadRegisters(BME280_REG_H1,
                              &bmp280_calibration.dig_h1,
                              1U) == 0U) ||
        (BMP280_ReadRegisters(BME280_REG_HUM_CALIB_START,
                              humidity_calibration,
                              sizeof(humidity_calibration)) == 0U))
    {
      printf("[BME280] init failed: humidity calibration read\r\n");
      return 0U;
    }

    bmp280_calibration.dig_h2 = BMP280_ReadS16LE(&humidity_calibration[0]);
    bmp280_calibration.dig_h3 = humidity_calibration[2];
    bmp280_calibration.dig_h4 = BME280_SignExtend12(
        ((uint16_t)humidity_calibration[3] << 4U) |
        ((uint16_t)humidity_calibration[4] & 0x0FU));
    bmp280_calibration.dig_h5 = BME280_SignExtend12(
        ((uint16_t)humidity_calibration[5] << 4U) |
        ((uint16_t)humidity_calibration[4] >> 4U));
    bmp280_calibration.dig_h6 = (int8_t)humidity_calibration[6];
    bmp280_calibration.humidity_calibrated = 1U;
  }

  if (BMP280_WriteRegister(BMP280_REG_CONFIG, BMP280_CONFIG_VALUE) == 0U)
  {
    printf("[BMP280] init failed: configuration write\r\n");
    return 0U;
  }

  if ((chip_id == BME280_CHIP_ID) &&
      (BMP280_WriteRegister(BMP280_REG_CTRL_HUM, BME280_CTRL_HUM_VALUE) == 0U))
  {
    printf("[BME280] init failed: ctrl_hum write\r\n");
    return 0U;
  }

  if (BMP280_WriteRegister(BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_VALUE) == 0U)
  {
    printf("[BMP280] init failed: ctrl_meas write\r\n");
    return 0U;
  }

  bmp280_calibration.chip_id = chip_id;
  bmp280_calibration.initialized = 1U;
  if (chip_id == BME280_CHIP_ID)
  {
    printf("[BME280] init ok: addr=0x%02X chip_id=0x%02X\r\n",
           BMP280_ADDR_7BIT,
           chip_id);
  }
  else
  {
    printf("[BMP280] init ok: addr=0x%02X chip_id=0x%02X\r\n",
           BMP280_ADDR_7BIT,
           chip_id);
  }
  return 1U;
}

uint8_t BMP280_Read(float *temperature_c, float *pressure_hpa)
{
  uint8_t data[BMP280_DATA_LENGTH];
  int32_t adc_temperature;
  int32_t adc_pressure;
  int32_t temperature_x100;
  uint32_t pressure_q24_8;

  if ((temperature_c == NULL) ||
      (pressure_hpa == NULL) ||
      (bmp280_calibration.initialized == 0U))
  {
    return 0U;
  }

  if (BMP280_ReadRegisters(BMP280_REG_DATA_START, data, sizeof(data)) == 0U)
  {
    return 0U;
  }

  adc_pressure = (int32_t)(((uint32_t)data[0] << 12U) |
                           ((uint32_t)data[1] << 4U) |
                           ((uint32_t)data[2] >> 4U));
  adc_temperature = (int32_t)(((uint32_t)data[3] << 12U) |
                              ((uint32_t)data[4] << 4U) |
                              ((uint32_t)data[5] >> 4U));

  if ((adc_temperature == 0x80000L) || (adc_pressure == 0x80000L))
  {
    return 0U;
  }

  temperature_x100 = BMP280_CompensateTemperature(adc_temperature);
  if (BMP280_CompensatePressure(adc_pressure, &pressure_q24_8) == 0U)
  {
    return 0U;
  }

  *temperature_c = (float)temperature_x100 / 100.0f;
  *pressure_hpa = (float)pressure_q24_8 / 25600.0f;
  return 1U;
}

uint8_t BMP280_IsBME280(void)
{
  return ((bmp280_calibration.initialized != 0U) &&
          (bmp280_calibration.chip_id == BME280_CHIP_ID) &&
          (bmp280_calibration.humidity_calibrated != 0U)) ? 1U : 0U;
}

uint8_t BME280_ReadHumidity(float *humidity_percent)
{
  uint8_t data[BME280_DATA_LENGTH];
  int32_t adc_temperature;
  int32_t adc_humidity;
  uint32_t humidity_q22_10;

  if ((humidity_percent == NULL) || (BMP280_IsBME280() == 0U))
  {
    return 0U;
  }

  if (BMP280_ReadRegisters(BMP280_REG_DATA_START, data, sizeof(data)) == 0U)
  {
    return 0U;
  }

  adc_temperature = (int32_t)(((uint32_t)data[3] << 12U) |
                              ((uint32_t)data[4] << 4U) |
                              ((uint32_t)data[5] >> 4U));
  adc_humidity = (int32_t)(((uint32_t)data[6] << 8U) |
                           (uint32_t)data[7]);

  if ((adc_temperature == 0x80000L) || (adc_humidity == 0x8000L))
  {
    return 0U;
  }

  (void)BMP280_CompensateTemperature(adc_temperature);
  humidity_q22_10 = BME280_CompensateHumidity(adc_humidity);
  *humidity_percent = (float)humidity_q22_10 / 1024.0f;
  return 1U;
}
