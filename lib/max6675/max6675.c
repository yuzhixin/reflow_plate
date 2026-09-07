#include "max6675.h"
#include "app_config.h"

#ifndef PIO_UNIT_TESTING
#include "hw_config.h"
static void delay_us(uint32_t us) {
    // Simple delay loop for bit-banging SPI
    // GD32E230 runs at 72MHz usually, so a few NOPs are enough for MAX6675 which supports up to 4.3MHz SPI.
    uint32_t count = us * (72 / 4);
    while(count--) {
        __asm("nop");
    }
}
#endif

void max6675_init(void) {
#ifndef PIO_UNIT_TESTING
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(MAX_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MAX_CS_PIN);
    gpio_output_options_set(MAX_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, MAX_CS_PIN);
    gpio_bit_set(MAX_CS_PORT, MAX_CS_PIN);
    // SPI SCK, MISO, MOSI are initialized in st7735_init as Hardware SPI
#endif
}

Max6675Status max6675_decode(uint16_t raw, float *out_temp) {
    if (out_temp != 0) *out_temp = APP_SENSOR_INVALID_TEMP_C;

    if (raw == 0xFFFFU) {
        return MAX6675_FAULT_ALL_ONES;
    }
    if ((raw & 0x0004U) != 0U) {
        return MAX6675_FAULT_OPEN;
    }
    if ((raw & 0x8000U) != 0U) {
        return MAX6675_FAULT_SHORT_FRAME;
    }

    if (out_temp != 0) {
        *out_temp = (float)((raw >> 3) & 0x0FFFU) * 0.25f;
    }
    return MAX6675_OK;
}

bool max6675_decode_raw(uint16_t raw, float *out_temp) {
    return max6675_decode(raw, out_temp) == MAX6675_OK;
}

Max6675Reading max6675_read(void) {
    Max6675Reading reading;
    reading.status = MAX6675_FAULT_SHORT_FRAME;
    reading.temp_c = APP_SENSOR_INVALID_TEMP_C;
    reading.raw = 0;

#ifndef PIO_UNIT_TESTING
    uint16_t raw_data = 0;
    
    // Slow down SPI for MAX6675 (4.3MHz max) -> 72MHz/32 = 2.25MHz
    spi_disable(SPI0);
    SPI_CTL0(SPI0) = (SPI_CTL0(SPI0) & ~SPI_CTL0_PSC) | SPI_PSC_32;
    spi_enable(SPI0);

    // Pull CS low
    gpio_bit_reset(MAX_CS_PORT, MAX_CS_PIN);
    delay_us(1); // Wait a tiny bit for the first bit to become ready

    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI0, 0xFF);
    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
    raw_data = spi_i2s_data_receive(SPI0) << 8;
    
    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI0, 0xFF);
    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
    raw_data |= spi_i2s_data_receive(SPI0);

    // Pull CS high
    gpio_bit_set(MAX_CS_PORT, MAX_CS_PIN);
    
    // Restore SPI speed for TFT (72MHz/4 = 18MHz)
    spi_disable(SPI0);
    SPI_CTL0(SPI0) = (SPI_CTL0(SPI0) & ~SPI_CTL0_PSC) | SPI_PSC_4;
    spi_enable(SPI0);

    reading.raw = raw_data;
    reading.status = max6675_decode(raw_data, &reading.temp_c);
#endif
    return reading;
}

float max6675_read_temp(void) {
    Max6675Reading reading = max6675_read();
    if (reading.status != MAX6675_OK) {
        return APP_SENSOR_INVALID_TEMP_C;
    }
    return reading.temp_c;
}
