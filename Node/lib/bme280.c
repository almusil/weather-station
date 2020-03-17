#include <bme280.h>

struct compensation_t {
    uint16_t t1;
    int16_t t2;
    int16_t t3;
    uint16_t p1;
    int16_t p2;
    int16_t p3;
    int16_t p4;
    int16_t p5;
    int16_t p6;
    int16_t p7;
    int16_t p8;
    int16_t p9;
    uint8_t h1;
    int16_t h2;
    uint8_t h3;
    int16_t h4;
    int16_t h5;
    int8_t h6;
    int32_t t_fine;
};

static struct compensation_t compensation = {0};

static void write_register(uint8_t reg, uint8_t val);

static uint8_t read_register(uint8_t reg);

static void read_compensation_data(void);

static int32_t compensate_temp(int32_t adc_t);

static uint32_t compensate_press(int32_t adc_p);

static uint32_t compensate_humid(int32_t adc_h);

void bme_setup() {
    read_compensation_data();
    // Configure all measurements for 1x oversampling
    write_register(REG_CTRL_HUM, OVERSAMPLING_1X);
    write_register(REG_CTRL_MEASURE, (OVERSAMPLING_1X << 5 | OVERSAMPLING_1X << 2));
}

void bme_start_measurement() {
    uint8_t ctrl = read_register(REG_CTRL_MEASURE);
    write_register(REG_CTRL_MEASURE, ctrl | FORCED_MODE);
}

bool bme_measure_done() {
    uint8_t status = read_register(REG_STATUS);
    return (status & STATUS_MEASURING) == 0;
}

int16_t bme_get_temp() {
    uint8_t read[REG_TEMP_LEN] = {0};
    uint8_t write[1] = {REG_TEMP};
    i2c_transfer(BME_ADDR, write, 1, read, REG_TEMP_LEN);
    int32_t adc_temp = (int32_t)(read[0] << 12 | read[1] << 4 | read[2] >> 4);
    int32_t temp = compensate_temp(adc_temp);
    return (int16_t) temp;
}

uint32_t bme_get_pressure() {
    uint8_t read[REG_PRESS_LEN] = {0};
    uint8_t write[1] = {REG_PRESS};
    i2c_transfer(BME_ADDR, write, 1, read, REG_PRESS_LEN);
    int32_t adc_press = (int32_t)(read[0] << 12 | read[1] << 4 | read[2] >> 4);
    float press = compensate_press(adc_press) >> 8;
    return (uint32_t) press;
}

uint16_t bme_get_humidity() {
    uint8_t read[REG_HUMIDITY_LEN] = {0};
    uint8_t write[1] = {REG_HUMIDITY};
    i2c_transfer(BME_ADDR, write, 1, read, REG_HUMIDITY_LEN);
    int32_t adc_hum = (int32_t)(read[0] << 8 | read[1]);
    float humid = (compensate_humid(adc_hum) >> 10) * 100.f;
    return (uint16_t) humid;
}

static void write_register(uint8_t reg, uint8_t val) {
    uint8_t write[2] = {reg, val};
    i2c_transfer(BME_ADDR, write, 2, 0, 0);
}

static uint8_t read_register(uint8_t reg) {
    uint8_t read[1] = {0};
    uint8_t write[1] = {reg};
    i2c_transfer(BME_ADDR, write, 1, read, 1);
    return read[0];
}

static void read_compensation_data() {
    uint8_t read[REG_COMP1_LEN + REG_COMP2_LEN + REG_COMP3_LEN] = {0};
    uint8_t write[1] = {REG_COMP1};
    i2c_transfer(BME_ADDR, write, 1, read, REG_COMP1_LEN);
    write[0] = REG_COMP2;
    i2c_transfer(BME_ADDR, write, 1, read + REG_COMP1_LEN, REG_COMP2_LEN);
    write[0] = REG_COMP3;
    i2c_transfer(BME_ADDR, write, 1, read + REG_COMP1_LEN + REG_COMP2_LEN, REG_COMP3_LEN);

    compensation.t1 = (uint16_t)(read[1] << 8 | read[0]);
    compensation.t2 = (int16_t)(read[3] << 8 | read[2]);
    compensation.t3 = (int16_t)(read[5] << 8 | read[4]);

    compensation.p1 = (uint16_t)(read[7] << 8 | read[6]);
    compensation.p2 = (int16_t)(read[9] << 8 | read[8]);
    compensation.p3 = (int16_t)(read[11] << 8 | read[10]);
    compensation.p4 = (int16_t)(read[13] << 8 | read[12]);
    compensation.p5 = (int16_t)(read[15] << 8 | read[14]);
    compensation.p6 = (int16_t)(read[17] << 8 | read[16]);
    compensation.p7 = (int16_t)(read[19] << 8 | read[18]);
    compensation.p8 = (int16_t)(read[21] << 8 | read[20]);
    compensation.p9 = (int16_t)(read[23] << 8 | read[22]);

    compensation.h1 = (uint8_t) read[24];
    compensation.h2 = (int16_t)(read[26] << 8 | read[25]);
    compensation.h3 = (uint8_t) read[27];
    compensation.h4 = (int16_t)(read[28] << 4 | (read[29] & 0x0f));
    compensation.h5 = (int16_t)(read[30] << 4 | (read[29] >> 4));
    compensation.h6 = (uint8_t) read[31];
}

static int32_t compensate_temp(int32_t adc_t) {
    int32_t var1, var2, t;
    var1 = ((((adc_t >> 3) - ((int32_t) compensation.t1 << 1))) * ((int32_t) compensation.t2)) >> 11;
    var2 = (((((adc_t >> 4) - ((int32_t) compensation.t1)) * ((adc_t >> 4) - ((int32_t) compensation.t1)))
            >> 12) *
            ((int32_t) compensation.t3)) >> 14;
    compensation.t_fine = var1 + var2;
    t = (compensation.t_fine * 5 + 128) >> 8;
    return t;
}

static uint32_t compensate_press(int32_t adc_p) {
    int64_t var1, var2, p;
    var1 = ((int64_t) compensation.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t) compensation.p6;
    var2 = var2 + ((var1 * (int64_t) compensation.p5) << 17);
    var2 = var2 + (((int64_t) compensation.p4) << 35);
    var1 = ((var1 * var1 * (int64_t) compensation.p3) >> 8) + ((var1 * (int64_t) compensation.p2) << 12);
    var1 = (((((int64_t) 1) << 47) + var1)) * ((int64_t) compensation.p1) >> 33;
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - adc_p;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t) compensation.p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t) compensation.p8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t) compensation.p7) << 4);
    return (uint32_t) p;
}

static uint32_t compensate_humid(int32_t adc_h) {
    int32_t v_x1_u32r;
    v_x1_u32r = (compensation.t_fine - ((int32_t) 76800));
    v_x1_u32r = (((((adc_h << 14) - (((int32_t) compensation.h4) << 20) - (((int32_t) compensation.h5) *
                                                                           v_x1_u32r)) + ((int32_t) 16384))
            >> 15) * (
                         ((((((v_x1_u32r * ((int32_t) compensation.h6)) >> 10) *
                             (((v_x1_u32r * ((int32_t) compensation.h3)) >> 11) + ((int32_t) 32768))) >> 10) +
                           ((int32_t) 2097152)) * ((int32_t) compensation.h2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r -
                 (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t) compensation.h1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t)(v_x1_u32r >> 12);
}
