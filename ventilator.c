// [1] Air Flow Rate Sensor Code:

/* Flow Rate Sensor Code
Make sure i2c is turned on in raspi-config.
Also pigpio daemon should be off.
Uses sda1 and scl1.
stdio.h is only included for printf() and can
61
be removed if not using printf().
*/
#include <stdint.h>
// #include <pigpio.h>
#include <stdio.h>
#define FRSensor_ADDRESS 0x40
int main(void)
{
    uint16_t value, flow = 0;
    uint8_t direction;
    int handle;
    unsigned char data[2];
    unsigned char command[2];
    if (gpioInitialise() < 0)
        return 1;                             // Initialize pigpio and check for error
    handle = i2cOpen(1, FRSensor_ADDRESS, 0); // Get handle(id) for sensor
    command[0] = 0x10;                        // Set flow collection command,0x1000
    command[1] = 0x00;
    while (1)
    {
        i2cWriteDevice(handle, &command, 2); // Tell sensor to prepare data
        i2cReadDevice(handle, &data, 2);     // Read bytes into value array
        // Get flow rate
        value = (data[0] << 8) | (data[1] && 0xFF);
        if (value > 0x7D00) // check direction of flow

        {
            flow = ((value - 32000) / 140);
            direction = 1; // Forward direction
        }
        else if (value < 0x7D00)
        {
            flow = ((32000 - value) / 140);
            direction = 0; // Reverse direction
        }
        else
        {
            flow = 0;
            direction = 1;
        }
        printf("%d and %d \n", flow, direction);
        for (int i = 2850; i > 0; i--)
            ; // Delay before next read
    }
    i2cClose(handle);
    gpioTerminate();
}
// [2] BME280 Sensor Code:

/* Temp/Humid/Pressure Sensor Code
Make sure i2c is turned on in raspi-config.
Also pigpio daemon should be off.
Uses sda1 and scl1.
stdio.h is only included for printf() and can
be removed if not using printf().
63
Setting:
-Normal Mode (Continuous Measurements)
-Oversampling = 1 (All)
-No IIR Filter
-Standby time = 0.5ms
Max measure time = 9.3ms
Standby time = 0.5ms
Reading Rate = 1000/(9.3 + 0.5)= 102Hz
Chose rate of 50Hz
*/
#include <stdint.h>
#include <stdio.h>
// #include <pigpio.h>
#define Sensor_ADDR 0x76
#define DATA_ADDR 0xF7
#define COMP1_ADDR 0x88
#define COMP2_ADDR 0xE1
    int32_t
    Temp_Compens(int32_t adc_T);
uint32_t Pres_Compens(int32_t adc_P);
uint32_t Humi_Compens(int32_t adc_H);
void org_compens(unsigned char comp1[25], unsigned char comp2[7]);
int32_t t_fine;
unsigned char dig_H1, dig_H3;
uint16_t dig_T1, dig_P1;
int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5,
     dig_P6, dig_P7, dig_P8, dig_P9, dig_H2, dig_H4 = 0, dig_H5 = 0;
signed char dig_H6;
int main(void)
{
    int32_t un_temp, un_pres, un_humi, temp;
    uint32_t pres, humi;
    int32_t handle;
    unsigned char data[8], compens1[25], compens2[7], reg_addr, set[2];
    if (gpioInitialise() < 0)
        return 1;                        // Initialize pigpio and check for error
    handle = i2cOpen(1, Sensor_ADDR, 0); // Get handle(id) for sensor
    for (uint32_t i = 3000000; i > 0; i--)
        ; // start up delay (2ms)
    // Set Sensor Settings
    set[0] = 0xF5; // standby time register
    set[1] = 0;    // standby time = 0.5ms, filter off, spi disabled
    i2cWriteDevice(handle, &set, 2);
    set[0] = 0xF2; // humidity sampling register
    set[1] = 1;    // oversampling = 1
    i2cWriteDevice(handle, &set, 2);
    set[0] = 0xF4;       // Temp and Pres sampling, and mode register
    set[1] = 0b00100111; // oversampling = 1, normal mode
    i2cWriteDevice(handle, &set, 2);
    // Get compensation parameters
    reg_addr = COMP1_ADDR;
    i2cWriteDevice(handle, &reg_addr, 1); // Prepare comp first part
    i2cReadDevice(handle, &compens1, 25);    // Read bytes into compens1 array
    reg_addr = COMP2_ADDR;
    i2cWriteDevice(handle, &reg_addr, 1); // Prepare comp second part
    i2cReadDevice(handle, &compens2, 7);  // Read bytes into compens2 array
    org_compens(compens1, compens2);      // Organize compensation parameters
    while (1)
    {
        // Get uncompensated measurements
        reg_addr = DATA_ADDR;
        i2cWriteDevice(handle, &reg_addr, 1); // Prepare correct register
        i2cReadDevice(handle, &data, 8);      // Read bytes into data array
        // Organize received data
        un_pres = (data[0] << 12) | (data[1] << 4) | ((data[2] && 0xF0) >> 4);
        un_temp = (data[3] << 12) | (data[4] << 4) | ((data[5] && 0xFF) >> 4);
        un_humi = (data[6] << 8) | (data[7] && 0xFF);
        // Get compensated measurements
        temp = Temp_Compens(un_temp);        // in DegC*100
        pres = Pres_Compens(un_pres) / 256;  // in Pa
        humi = Humi_Compens(un_humi) / 1024; // in %RH
        // Print out measurements
        printf("T= %d and P= %d and H= %d \n", temp, pres, humi);
        for (int32_t j = 10; j > 0; j--)
        {
            for (uint32_t i = 3000000; i > 0; i--)
                ;
        }; // delay (20ms)
    }
    i2cClose(handle);
    gpioTerminate();
}
void org_compens(unsigned char comp1[25], unsigned char comp2[7])
{
    dig_T1 = (comp1[0] && 0xFF) | (comp1[1] << 8);
    dig_T2 = (comp1[2] && 0xFF) | (comp1[3] << 8);
    dig_T3 = (comp1[4] && 0xFF) | (comp1[5] << 8);
    dig_P1 = (comp1[6] && 0xFF) | (comp1[7] << 8);
    dig_P2 = (comp1[8] && 0xFF) | (comp1[9] << 8);
    dig_P3 = (comp1[10] && 0xFF) | (comp1[11] << 8);
    dig_P4 = (comp1[12] && 0xFF) | (comp1[13] << 8);
    dig_P5 = (comp1[14] && 0xFF) | (comp1[15] << 8);
    dig_P6 = (comp1[16] && 0xFF) | (comp1[17] << 8);
    dig_P7 = (comp1[18] && 0xFF) | (comp1[19] << 8);
    dig_P8 = (comp1[20] && 0xFF) | (comp1[21] << 8);
    dig_P9 = (comp1[22] && 0xFF) | (comp1[23] << 8);
    dig_H1 = comp1[24] && 0xFF;
    dig_H2 = (comp2[0] && 0xFF) | (comp2[1] << 8);
    dig_H3 = comp2[2] && 0xFF;
    dig_H4 = (comp2[3] << 4) | (comp2[4] && 0x0F);
    dig_H5 = ((comp2[4] && 0xF0) >> 4) | (comp2[5] << 4);
    dig_H6 = comp2[6] && 0xFF;
    return;
}
// Returns temperature in DegC, resolution is 0.01 DegC.
// Output value of “5123” equals 51.23 DegC.
int32_t Temp_Compens(int32_t adc_T)
{
     int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format
// (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t Pres_Compens(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)p;
}
// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format
// (22 integer and 10 fractional bits).
// Output value of “47445” represents 47445/1024 = 46.333 %RH
uint32_t Humi_Compens(int32_t adc_H)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r *
                                                                                                                                     ((int32_t)dig_H6)) >>
                                                                                                                                    10) *
                                                                                                                                   (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) +
                                                                                                                                    ((int32_t)32768))) >>
                                                                                                                                  10) +
                                                                                                                                 ((int32_t)2097152)) *
                                                                                                                                    ((int32_t)dig_H2) +
                                                                                                                                8192) >>
                                                                                                                               14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                               ((int32_t)dig_H1)) >>
                              4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t)(v_x1_u32r >> 12);
}
