/******************************************************************************
  文 件 名   : balance.h
  版 本 号   : 初稿
  作    者   : sgeeood
  生成日期   : 2022年7月5日
  最近修改   :
  功能描述   : balance.c 的头文件
  
  修改历史   :
  1.日    期   : 2022年7月5日
    作    者   : sgeeood
    修改内容   : 创建文件

******************************************************************************/

#ifndef __BALANCE_H__
#define __BALANCE_H__


#include <rtthread.h>   //内核头文件
#include <rtdevice.h>   //设备头文件
#include <board.h>      //硬件头文件


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#if PKG_USING_BALANCE
//定义大小端模式
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#endif
//选择要使用的设备通道
#define BALANCE_USING_CH0
//#define BALANCE_USING_CH1
//#define BALANCE_USING_CH2

//ADC滤波缓存字节数
#define BALANCE_ADC_BUFFSZ 10
//数据有效位
#define VALID_BIT (unsigned short int)1000
//定义数据小数位
#define DECIMAL_PLACES ((double)0.1)
//保留小数位计算魔术字
#define DECIMAL_PLACES_CALC (int)(1.0/DECIMAL_PLACES)
//校准系数放大倍数
#define FACTOR_AMPLIFICATION (unsigned int)1000000


//pin定义
#ifdef BALANCE_USING_CH0
#define __BALANCE_DOUT_PIN_CH0             GET_PIN(D,10)
#define __BALANCE_SCLK_PIN_CH0             GET_PIN(D,11)
#endif
#ifdef BALANCE_USING_CH1
#define __BALANCE_DOUT_PIN_CH1             GET_PIN(port,pin)
#define __BALANCE_SCLK_PIN_CH1             GET_PIN(port,pin)
#endif
#ifdef BALANCE_USING_CH2
#define __BALANCE_DOUT_PIN_CH2             GET_PIN(port,pin)
#define __BALANCE_SCLK_PIN_CH2             GET_PIN(port,pin)
#endif



#define BALANCE_PIN_MODE_CONFIG(PIN,MODE)   rt_pin_mode(PIN,MODE)
#define BALANCE_SCLK_SET(__DEV__,LEVEL)     rt_pin_write(__DEV__,LEVEL)
#define BALANCE_DOUT_GET(__DEV__)           ((rt_pin_read(__DEV__))?0:1)
#define BALANCE_DELAY_US(n)                 rt_hw_us_delay(n)




enum balance_e
{
    balance_busy = ((int32_t)0x80000000),
    
};
typedef enum balance_e balance_enum_t;

enum __ret_e
{
    __ok        = 0,
    __finish    = __ok,
    __success   = __ok,
    __error     =1,
    __timeout   =2,
    __full      =3,
    __empty     =4,
    __busy      =5,
    __ioerr     =6,
    __invalid   =7,
    __exception = -1,

};
typedef enum __ret_e __ret_enum_t;




struct balance_s
{
    uint8_t index;
    const char* name;
    struct
    {
        uint8_t dout;
        uint8_t sclk;
    }pin;
    struct
    {
        int32_t current_weight;
        int32_t set_weight;
        uint32_t calibration_weight;
        int32_t remaining_weight;
        double calibration_factor;
    }data;
    struct 
    {
        int32_t current;
        int32_t last;
    	int32_t reference_zero;
    	int32_t physical_zero;
    	int32_t buff[BALANCE_ADC_BUFFSZ];
    }adc;
    union
    {
        unsigned char byte;
        struct
        {
            //byte:          byte1
            //bit:  b7 b6 b5 b4 b3 b2 b1 b0
		#if __LITTLE_ENDIAN
            //little endian
            unsigned char boot_init:1;
            unsigned char balance_peel:1;
            unsigned char balance_calibration:1;
            unsigned char factor_read:1;
            unsigned char bit4:1;
            unsigned char bit5:1;
            unsigned char bit6:1;
            unsigned char bit7:1;
		#else
            //big endian
            unsigned char bit7:1;
            unsigned char bit6:1;
            unsigned char bit5:1;
            unsigned char bit4:1;
            unsigned char bit3:1;
            unsigned char bit2:1;
            unsigned char bit1:1;
            unsigned char bit0:1;
		#endif
        };
        unsigned char arr[1];
    }sings;

    
    
};
typedef struct balance_s balance_struct_t;
typedef struct balance_s * balance_struct_t_p;



extern balance_struct_t balance[3];


extern char balance_load(void);
extern char balance_factor_write(balance_struct_t_p dev,uint32_t weight);
extern char balance_peel(balance_struct_t_p dev);
extern char balance_the_set_quantity_is_reached(balance_struct_t_p dev);
extern int balance_remaining_weight_get(balance_struct_t_p dev);


#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __BALANCE_H__ */
