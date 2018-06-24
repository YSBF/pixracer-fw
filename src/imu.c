#include <ch.h>
#include <hal.h>
#include <msgbus/msgbus.h>
#include <types/sensors.h>
#include <error/error.h>
#include "mpu9250.h"
#include "thread_prio.h"
#include "main.h"

static THD_WORKING_AREA(imu_thread, 1024);
static THD_FUNCTION(imu_thread_main, arg)
{
    (void) arg;
    static mpu9250_t imu;
    static msgbus_topic_t imu_topic;
    static imu_sample_t imu_topic_buf;

    chRegSetThreadName("IMU");

    msgbus_topic_create(&imu_topic, &bus, &imu_sample_type, &imu_topic_buf, "/imu");

    // power up sensors
    palSetPadMode(GPIOE, GPIOE_VDD_3V3_SENSORS_EN, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOE, GPIOE_VDD_3V3_SENSORS_EN);
    chThdSleepMilliseconds(500);
    palSetPad(GPIOE, GPIOE_VDD_3V3_SENSORS_EN);

    // init mosi miso sck cs, AF5
    palSetPadMode(GPIOA, GPIOA_MPU9250_SCK, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
    palSetPadMode(GPIOA, GPIOA_MPU9250_MISO, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST | PAL_STM32_PUPDR_PULLUP);
    palSetPadMode(GPIOA, GPIOA_MPU9250_MOSI, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
    palSetPadMode(GPIOC, GPIOC_MPU9250_CS, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palSetPadMode(GPIOD, GPIOD_MPU9250_DRDY, PAL_MODE_INPUT_PULLUP);
    palSetPad(GPIOC, GPIOC_MPU9250_CS);

    palSetPadMode(GPIOC, GPIOC_20608_CS, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palSetPad(GPIOC, GPIOC_20608_CS);
    palSetPadMode(GPIOD, GPIOD_BARO_CS, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palSetPad(GPIOD, GPIOD_BARO_CS);
    palSetPadMode(GPIOD, GPIOD_FRAM_CS, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palSetPad(GPIOD, GPIOD_FRAM_CS);

    /*
     * SPI1 configuration structure for MPU9250.
     * SPI1 is on APB2 @ 84MHz / 128 = 656.25kHz
     * CPHA=1, CPOL=1, 8bits frames, MSb transmitted first.
     */
    static SPIConfig spi_cfg = {
        .end_cb = NULL,
        .ssport = GPIOC,
        .sspad = GPIOC_MPU9250_CS,
        .cr1 = SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA
    };
    spiStart(&SPID1, &spi_cfg);

    mpu9250_init(&imu, &SPID1);

    mpu9250_reset(&imu);

    chThdSleepMilliseconds(100);

    if (!mpu9250_ping(&imu)) {
        ERROR("IMU ping");
    }

    mpu9250_configure(&imu);
    mpu9250_enable_magnetometer(&imu);

    /* speed up SPI for sensor register reads (max 20MHz)
     * APB2 @ 84MHz / 8 = 10.5MHz
     */
    spi_cfg.cr1 = SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA;
    spiStart(&SPID1, &spi_cfg);

    // check that the sensor still pings
    if (!mpu9250_ping(&imu)) {
        ERROR("IMU ping");
    }

    while (1) {
        static imu_sample_t sample;

        while(palReadPad(GPIOD, GPIOD_MPU9250_DRDY) != 1) {
            chThdSleepMilliseconds(1);
        }
        mpu9250_gyro_read(&imu, &sample.rate[0], &sample.rate[1], &sample.rate[2]);
        mpu9250_acc_read(&imu, &sample.acceleration[0], &sample.acceleration[1], &sample.acceleration[2]);
        mpu9250_interrupt_read_and_clear(&imu);

        sample.timestamp_us = TIME_I2US(chVTGetSystemTime());

        msgbus_topic_publish(&imu_topic, &sample);

    }
}

void imu_start(void)
{
    chThdCreateStatic(imu_thread, sizeof(imu_thread), THD_PRIO_IMU, imu_thread_main, NULL);
}
