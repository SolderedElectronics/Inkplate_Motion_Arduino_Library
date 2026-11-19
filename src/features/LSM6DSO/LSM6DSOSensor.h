/**
 ******************************************************************************
 * @file    LSM6DSOSensor.h
 * @author  SRA
 * @version V1.0.0
 * @date    February 2019
 * @brief   Abstract Class of an LSM6DSO Inertial Measurement Unit (IMU) 3 axes
 *          sensor.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2019 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */


/* Prevent recursive inclusion -----------------------------------------------*/

#ifndef __LSM6DSOSensor_H__
#define __LSM6DSOSensor_H__


/* Includes ------------------------------------------------------------------*/

#include "Wire.h"
#include "lsm6dso_reg.h"

/* Defines -------------------------------------------------------------------*/

#define LSM6DSO_ACC_SENSITIVITY_FS_2G   0.061f
#define LSM6DSO_ACC_SENSITIVITY_FS_4G   0.122f
#define LSM6DSO_ACC_SENSITIVITY_FS_8G   0.244f
#define LSM6DSO_ACC_SENSITIVITY_FS_16G  0.488f
#define LSM6DSO_ACC_SENSITIVITY_FS_32G  0.976f

#define LSM6DSO_GYRO_SENSITIVITY_FS_125DPS    4.375f
#define LSM6DSO_GYRO_SENSITIVITY_FS_250DPS    8.750f
#define LSM6DSO_GYRO_SENSITIVITY_FS_500DPS   17.500f
#define LSM6DSO_GYRO_SENSITIVITY_FS_1000DPS  35.000f
#define LSM6DSO_GYRO_SENSITIVITY_FS_2000DPS  70.000f


/* Typedefs ------------------------------------------------------------------*/

typedef enum
{
  LSM6DSO_OK = 0,
  LSM6DSO_ERROR =-1
} LSM6DSOStatusTypeDef;

typedef enum
{
  LSM6DSO_INT1_PIN,
  LSM6DSO_INT2_PIN,
} LSM6DSO_SensorIntPin_t;

typedef enum
{
  LSM6DSO_ACC_HIGH_PERFORMANCE_MODE,
  LSM6DSO_ACC_LOW_POWER_NORMAL_MODE,
  LSM6DSO_ACC_ULTRA_LOW_POWER_MODE
} LSM6DSO_ACC_Operating_Mode_t;

typedef enum
{
  LSM6DSO_GYRO_HIGH_PERFORMANCE_MODE,
  LSM6DSO_GYRO_LOW_POWER_NORMAL_MODE
} LSM6DSO_GYRO_Operating_Mode_t;

typedef enum
{
  LSM6DSO_VARIANT_LSM6DSO,
  LSM6DSO_VARIANT_LSM6DSO32
} LSM6DSO_Variant_t;

typedef struct
{
  unsigned int FreeFallStatus : 1;
  unsigned int TapStatus : 1;
  unsigned int DoubleTapStatus : 1;
  unsigned int WakeUpStatus : 1;
  unsigned int StepStatus : 1;
  unsigned int TiltStatus : 1;
  unsigned int D6DOrientationStatus : 1;
  unsigned int SleepStatus : 1;
} LSM6DSO_Event_Status_t;


/* Class Declaration ---------------------------------------------------------*/

/**
 * Abstract class of an LSM6DSO Inertial Measurement Unit (IMU) 3 axes
 * sensor.
 */
class LSM6DSOSensor
{
  public:
    LSM6DSOSensor(TwoWire *i2c, uint8_t address=LSM6DSO_I2C_ADD_H);
    LSM6DSOStatusTypeDef begin();
    LSM6DSOStatusTypeDef begin(LSM6DSO_Variant_t variant);
    LSM6DSOStatusTypeDef end();
    LSM6DSOStatusTypeDef readId(uint8_t *id);
    LSM6DSOStatusTypeDef detectVariant(LSM6DSO_Variant_t *variant);
    LSM6DSO_Variant_t getVariant() { return device_variant; }
    LSM6DSOStatusTypeDef enableAccelerator();
    LSM6DSOStatusTypeDef disableAccelerator();
    LSM6DSOStatusTypeDef getAcceleratorSensitivity(float *sensitivity);
    LSM6DSOStatusTypeDef getAcceleratorOdr(float *odr);
    LSM6DSOStatusTypeDef setAcceleratorOdr(float odr);
    LSM6DSOStatusTypeDef setAcceleratorOdrWithMode(float odr, LSM6DSO_ACC_Operating_Mode_t mode);
    LSM6DSOStatusTypeDef getAcceleratorFullScale(int32_t *fullScale);
    LSM6DSOStatusTypeDef setAcceleratorFullScale(int32_t fullScale);
    LSM6DSOStatusTypeDef getAcceleratorAxesRaw(int16_t *value);
    LSM6DSOStatusTypeDef getAcceleratorAxes(int32_t *acceleration);

    LSM6DSOStatusTypeDef enableGyro();
    LSM6DSOStatusTypeDef disableGyro();
    LSM6DSOStatusTypeDef getGyroSensitivity(float *sensitivity);
    LSM6DSOStatusTypeDef getGyroOdr(float *odr);
    LSM6DSOStatusTypeDef setGyroOdr(float odr);
    LSM6DSOStatusTypeDef setGyroOdrWithMode(float odr, LSM6DSO_GYRO_Operating_Mode_t mode);
    LSM6DSOStatusTypeDef getGyroFullScale(int32_t *fullScale);
    LSM6DSOStatusTypeDef setGyroFullScale(int32_t fullScale);
    LSM6DSOStatusTypeDef getGyroAxesRaw(int16_t *value);
    LSM6DSOStatusTypeDef getGyroAxes(int32_t *angularRate);

    LSM6DSOStatusTypeDef readRegister(uint8_t reg, uint8_t *data);
    LSM6DSOStatusTypeDef writeRegister(uint8_t reg, uint8_t data);
    LSM6DSOStatusTypeDef setInterruptLatch(uint8_t status);
    LSM6DSOStatusTypeDef setInterruptPolarity(uint8_t status);
    LSM6DSOStatusTypeDef setInterruptPinMode(uint8_t status);

    LSM6DSOStatusTypeDef enableFreeFallDetection(LSM6DSO_SensorIntPin_t intPin);
    LSM6DSOStatusTypeDef disableFreeFallDetection();
    LSM6DSOStatusTypeDef setFreeFallThreshold(uint8_t threshold);
    LSM6DSOStatusTypeDef setFreeFallDuration(uint8_t duration);

    LSM6DSOStatusTypeDef enablePedometer();
    LSM6DSOStatusTypeDef disablePedometer();
    LSM6DSOStatusTypeDef getStepCounter(uint16_t *stepCount);
    LSM6DSOStatusTypeDef resetStepCounter();

    LSM6DSOStatusTypeDef enableTiltDetection(LSM6DSO_SensorIntPin_t intPin);
    LSM6DSOStatusTypeDef disableTiltDetection();

    LSM6DSOStatusTypeDef enableWakeUpDetection(LSM6DSO_SensorIntPin_t intPin);
    LSM6DSOStatusTypeDef disableWakeUpDetection();
    LSM6DSOStatusTypeDef setWakeUpThreshold(uint8_t threshold);
    LSM6DSOStatusTypeDef setWakeUpDuration(uint8_t duration);

    LSM6DSOStatusTypeDef enableSingleTapDetection(LSM6DSO_SensorIntPin_t intPin);
    LSM6DSOStatusTypeDef disableSingleTapDetection();
    LSM6DSOStatusTypeDef enableDoubleTapDetection(LSM6DSO_SensorIntPin_t intPin);
    LSM6DSOStatusTypeDef disableDoubleTapDetection();
    LSM6DSOStatusTypeDef setTapThreshold(uint8_t threshold);
    LSM6DSOStatusTypeDef setTapShockTime(uint8_t time);
    LSM6DSOStatusTypeDef setTapQuietTime(uint8_t time);
    LSM6DSOStatusTypeDef setTapDurationTime(uint8_t time);

    LSM6DSOStatusTypeDef enable6dOrientation(LSM6DSO_SensorIntPin_t intPin);
    LSM6DSOStatusTypeDef disable6dOrientation();
    LSM6DSOStatusTypeDef set6dOrientationThreshold(uint8_t threshold);
    LSM6DSOStatusTypeDef get6dOrientationXl(uint8_t *xl);
    LSM6DSOStatusTypeDef get6dOrientationXh(uint8_t *xh);
    LSM6DSOStatusTypeDef get6dOrientationYl(uint8_t *yl);
    LSM6DSOStatusTypeDef get6dOrientationYh(uint8_t *yh);
    LSM6DSOStatusTypeDef get6dOrientationZl(uint8_t *zl);
    LSM6DSOStatusTypeDef get6dOrientationZh(uint8_t *zh);

    LSM6DSOStatusTypeDef getAcceleratorDataReadyStatus(uint8_t *status);
    LSM6DSOStatusTypeDef getEventStatus(LSM6DSO_Event_Status_t *status);
    LSM6DSOStatusTypeDef setAcceleratorSelfTest(uint8_t val);

    LSM6DSOStatusTypeDef getGyroDataReadyStatus(uint8_t *status);
    LSM6DSOStatusTypeDef setGyroSelfTest(uint8_t val);

    LSM6DSOStatusTypeDef getFifoNumSamples(uint16_t *numSamples);
    LSM6DSOStatusTypeDef getFifoFullStatus(uint8_t *status);
    LSM6DSOStatusTypeDef setFifoInt1FifoFull(uint8_t status);
    LSM6DSOStatusTypeDef setFifoWatermarkLevel(uint16_t watermark);
    LSM6DSOStatusTypeDef setFifoStopOnFth(uint8_t status);
    LSM6DSOStatusTypeDef setFifoMode(uint8_t mode);
    LSM6DSOStatusTypeDef getFifoTag(uint8_t *tag);
    LSM6DSOStatusTypeDef getFifoData(uint8_t *data);
    LSM6DSOStatusTypeDef getFifoAcceleratorAxes(int32_t *acceleration);
    LSM6DSOStatusTypeDef setFifoAcceleratorBdr(float bdr);
    LSM6DSOStatusTypeDef getFifoGyroAxes(int32_t *angularVelocity);
    LSM6DSOStatusTypeDef setFifoGyroBdr(float bdr);

    /**
     * @brief Utility function to read data.
     * @param  pBuffer: pointer to data to be read.
     * @param  RegisterAddr: specifies internal address register to be read.
     * @param  NumByteToRead: number of bytes to be read.
     * @retval 0 if ok, an error code otherwise.
     */
    uint8_t ioRead(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToRead)
    {
      if (dev_i2c) {
        dev_i2c->beginTransmission(((uint8_t)(((address) >> 1) & 0x7F)));
        dev_i2c->write(RegisterAddr);
        dev_i2c->endTransmission(false);

        dev_i2c->requestFrom(((uint8_t)(((address) >> 1) & 0x7F)), (uint8_t) NumByteToRead);

        int i=0;
        while (dev_i2c->available()) {
          pBuffer[i] = dev_i2c->read();
          i++;
        }

        return 0;
      }

      return 1;
    }

    /**
     * @brief Utility function to write data.
     * @param  pBuffer: pointer to data to be written.
     * @param  RegisterAddr: specifies internal address register to be written.
     * @param  NumByteToWrite: number of bytes to write.
     * @retval 0 if ok, an error code otherwise.
     */
    uint8_t ioWrite(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToWrite)
    {
      if (dev_i2c) {
        dev_i2c->beginTransmission(((uint8_t)(((address) >> 1) & 0x7F)));

        dev_i2c->write(RegisterAddr);
        for (uint16_t i = 0 ; i < NumByteToWrite ; i++) {
          dev_i2c->write(pBuffer[i]);
        }

        dev_i2c->endTransmission(true);

        return 0;
      }

      return 1;
    }

  private:

    LSM6DSOStatusTypeDef setAcceleratorOdrWhenEnabled(float odr);
    LSM6DSOStatusTypeDef setAcceleratorOdrWhenDisabled(float odr);
    LSM6DSOStatusTypeDef setGyroOdrWhenEnabled(float odr);
    LSM6DSOStatusTypeDef setGyroOdrWhenDisabled(float odr);

    /* Helper classes. */
    TwoWire *dev_i2c;

    /* Configuration */
    uint8_t address;
    LSM6DSO_Variant_t device_variant;
    bool variant_forced;

    lsm6dso_odr_xl_t acc_odr;
    lsm6dso_odr_g_t gyro_odr;

    uint8_t acc_is_enabled;
    uint8_t gyro_is_enabled;

    lsm6dso_ctx_t reg_ctx;

};

#ifdef __cplusplus
 extern "C" {
#endif
int32_t LSM6DSO_io_write( void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite );
int32_t LSM6DSO_io_read( void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead );
#ifdef __cplusplus
  }
#endif

#endif
