/******************************************************************************
 文 件 名   : balance.c
 版 本 号   : 初稿
 作    者   : sgeeood
 生成日期   : 2022年7月6日
 最近修改   :
 功能描述   :
 实现称重测量
 1.所用芯片hx71x
 2.称重压力传感器

 修改历史   :
 1.日    期   : 2022年7月6日
 作    者   : sgeeood
 修改内容   : 创建文件

 ******************************************************************************/

#define DBG_TAG "balance"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>


#include <balance.h>


#if PKG_USING_BALANCE

balance_struct_t balance[3] =
        {
#ifdef BALANCE_USING_CH0
                { .index = 0, .name = "balance0", .pin.dout = __BALANCE_DOUT_PIN_CH0,
                        .pin.sclk = __BALANCE_SCLK_PIN_CH0, .data.calibration_weight = 500, .sings.boot_init = 1 },
#else
                {   .index = 0, .name = "default_0"},
#endif
#ifdef BALANCE_USING_CH1
                {   .index = 1, .name = "balance1", .pin.dout = __BALANCE_DOUT_PIN_CH1, .pin.sclk = __BALANCE_SCLK_PIN_CH1, .data.calibration_weight = 500,.sings.boot_init=1},
#else
                { .index = 1, .name = "default_1" },
#endif
#ifdef BALANCE_USING_CH2
                {   .index = 2, .name = "balance2", .pin.dout = __BALANCE_DOUT_PIN_CH2, .pin.sclk = __BALANCE_SCLK_PIN_CH2, .data.calibration_weight = 500,.sings.boot_init=1},
#else
                { .index = 2, .name = "default_2" },
#endif
        };


//先进先出
char int32_t_fifo(int*buff, unsigned short int len)
{
    if (len > 0xFFFE)
    {
        return __error;
    }
    for (unsigned short int i = 0; i < len; i++)
    {
        *(buff + (len - 1) - i) = *(buff + (len - 1) - i - 1);
    }
    return __ok;
}

//冒泡法
static unsigned char uint32_bubble_sort_rise_order(unsigned int *dat_pointer, unsigned short int qty)
{
    unsigned int tmp;
    unsigned short int i, j;
    if (qty > 512)
    {
        return 1;
    }
	
    for (i = 0; i < qty; i++)
    {
        for (j = 0; j < qty - i - 1; j++)
        {
            if (*(dat_pointer + j) > *(dat_pointer + j + 1))
            {
                tmp = *(dat_pointer + j) ;
                *(dat_pointer + j) = *(dat_pointer + j + 1);
                *(dat_pointer + j + 1) = tmp;
            }
        }
    }
    return 0;
}



/*****************************************************************************
 函 数 名  : balance_adc_read
 功能描述  : 获取hx71x ADC值
 输入参数  : balance_struct_t_p dev
 int32_t *ret
 输出参数  : *ret 返回测量值
 返 回 值  : static char
 调用函数  :
 被调函数  :

 修改历史      :
 1.日    期   : 2022年7月6日
 作    者   : sgeeood
 修改内容   : 新生成函数

 *****************************************************************************/
static char balance_adc_read(balance_struct_t_p dev)
{
    static int32_t val = 0;
    if (strcmp(dev->name, "balance0") && strcmp(dev->name, "balance1") && strcmp(dev->name, "balance2"))
    {
        LOG_E("%s is not defined in the lister", dev->name);
        return __exception;
    }
    BALANCE_SCLK_SET(dev->pin.sclk, PIN_LOW);
    if (BALANCE_DOUT_GET(dev->pin.dout))
    {
        val = 0;
        BALANCE_DELAY_US(1);
        for (uint8_t i = 0; i < 24; i++)
        {
            BALANCE_SCLK_SET(dev->pin.sclk, PIN_HIGH);
            val = val << 1;
            BALANCE_DELAY_US(1);
            BALANCE_SCLK_SET(dev->pin.sclk, PIN_LOW);
            if (!BALANCE_DOUT_GET(dev->pin.dout))
            {
                val++;
            }
            BALANCE_DELAY_US(1);
        }
        BALANCE_SCLK_SET(dev->pin.sclk, PIN_HIGH);
        BALANCE_DELAY_US(1);
        BALANCE_SCLK_SET(dev->pin.sclk, PIN_LOW);
        val = val ^ 0x00800000;
        BALANCE_DELAY_US(1);
        dev->adc.last = dev->adc.current;
        dev->adc.current = val / VALID_BIT;
        int32_t_fifo((int *) &dev->adc.buff[0], BALANCE_ADC_BUFFSZ);
        dev->adc.buff[0] = dev->adc.current;
        return __ok;
    }
    return __busy;
}


//上电读取参考0值
char balance_boot_init(balance_struct_t_p dev)
{
    static uint8_t boot_init_counter = 0;
    static int32_t tmp = 0;
    if (dev->sings.boot_init)
    {
        if (balance_adc_read(&balance[dev->index]) == __ok)
        {
            boot_init_counter++;
            if (boot_init_counter >= 20)
            {
                uint32_bubble_sort_rise_order((int *) &balance[dev->index].adc.buff[0], BALANCE_ADC_BUFFSZ);
                for (boot_init_counter = 2; boot_init_counter < BALANCE_ADC_BUFFSZ - 2; boot_init_counter++)
                {
                    tmp += balance[dev->index].adc.buff[boot_init_counter];
                }
                tmp = (tmp * 10) / 6 / 10;
                balance[dev->index].adc.reference_zero = tmp;
                LOG_I("balance[%d].adc.reference_zero = %d", dev->index, balance[dev->index].adc.reference_zero);
                dev->sings.boot_init = 0;
                return __ok;
            }
            else
            {
                return __busy;
            }
        }
        else
        {
            return __busy;
        }
    }
    else
    {
        return __ok;
    }

}


//去皮
char balance_peel(balance_struct_t_p dev)
{
    static uint8_t peel_counter = 0;
    static int32_t tmp = 0;

    if (balance_adc_read(&balance[dev->index]) == __ok)
    {
        peel_counter++;
        if (peel_counter >= BALANCE_ADC_BUFFSZ)
        {
            uint32_bubble_sort_rise_order((int *) &balance[dev->index].adc.buff[0], BALANCE_ADC_BUFFSZ);
            tmp = 0;
            for (peel_counter = 2; peel_counter < BALANCE_ADC_BUFFSZ - 2; peel_counter++)
            {
                tmp += balance[dev->index].adc.buff[peel_counter];
            }
            tmp = (tmp * 10) / 6 / 10;
            balance[dev->index].adc.reference_zero = tmp;
            peel_counter = 0;
            LOG_W("balance[%d].adc.reference_zero = %d",dev->index,balance[dev->index].adc.reference_zero);
            return __ok;
        }
        else
        {
            return __busy;
        }
    }
    else
    {
        return __busy;
    }

}


//设置校准重量
char balance_calibration_weight_set(balance_struct_t_p dev, uint32_t weight)
{
    dev->data.calibration_weight = weight;
    return __ok;
}


//读取校准系数
char balance_factor_read(balance_struct_t_p dev)
{
	uint32_t flash_val =0;

    //此处添加读取FLASH代码(读取校准系数)
	//add user code here
	//ex:  flash_val = flash_read();

    dev->data.calibration_factor = (double)((double)flash_val / (double)FACTOR_AMPLIFICATION);

    dev->sings.factor_read = 1;
    device_sign->magic_word_finish = dev->sings.factor_read;
    return __ok;
}


//保存校准系数
char balance_factor_write(balance_struct_t_p dev,uint32_t weight)
{
    static uint8_t calibration_counter = 0;
    static int32_t tmp = 0;
    balance_calibration_weight_set(dev,weight);
    if (balance_adc_read(&balance[dev->index]) == __ok)
    {
        calibration_counter++;
        if (calibration_counter >= BALANCE_ADC_BUFFSZ)
        {
            uint32_bubble_sort_rise_order((int *) &balance[dev->index].adc.buff[0], BALANCE_ADC_BUFFSZ);
            tmp = 0;
            for (calibration_counter = 2; calibration_counter < BALANCE_ADC_BUFFSZ - 2; calibration_counter++)
            {
                tmp += balance[dev->index].adc.buff[calibration_counter];
            }
            tmp = (tmp * 10) / 6 / 10;
            dev->data.calibration_factor = (double) dev->data.calibration_weight
                    / (double) (tmp - dev->adc.reference_zero);
            //此处添加存FLASH代码(用来存储校准系数)
			//add user code here

            calibration_counter = 0;
            return __ok;
        }
        else
        {
            return __busy;
        }
    }
    else
    {
        return __busy;
    }

}




//称重
static char balance_weight(balance_struct_t_p dev)
{
    static int32_t tmp = 0;
    int32_t tmp_calc =0;
    if (balance_adc_read(dev) == __ok)
    {

        tmp = 0;
        for (uint8_t i = 0; i < 10; i++)
        {
            tmp += balance[0].adc.buff[i];
        }
        tmp = tmp / 10;
        tmp_calc = (tmp - balance[0].adc.reference_zero)*DECIMAL_PLACES_CALC * balance[0].data.calibration_factor;
        if ((tmp_calc % 10) >=6)
        {
            tmp_calc /= 10;
            tmp_calc +=1;
        }
        else
        {
            tmp_calc /= 10;
        }
        balance[0].data.current_weight = tmp_calc;
        return __ok;
    }
    else
    {
        return __busy;
    }
}

//判断设定量是否到达
char balance_the_set_quantity_is_reached(balance_struct_t_p dev)
{
    if ((dev->data.current_weight - dev->data.set_weight) >= 0)
    {
        return __ok;
    }
    else
    {
        return __busy;
    }
}

//计算剩余量    负数为剩余量，正数为超出量
int balance_remaining_weight_get(balance_struct_t_p dev)
{
    return (dev->data.current_weight - dev->data.set_weight);
}



//--------------------------------------------------------------------------------------------------------

//变量及参数初始化
static char var_list_register(void)
{
    return __ok;
}
//引脚初始化
static char pin_list_register(void)
{
#ifdef BALANCE_USING_CH0
    BALANCE_PIN_MODE_CONFIG(balance[0].pin.dout, PIN_MODE_INPUT);
    BALANCE_PIN_MODE_CONFIG(balance[0].pin.sclk, PIN_MODE_OUTPUT);
    BALANCE_SCLK_SET(balance[0].pin.sclk, PIN_LOW);
#endif
#ifdef BALANCE_USING_CH1
    BALANCE_PIN_MODE_CONFIG(balance[1].pin.dout, PIN_MODE_INPUT);
    BALANCE_PIN_MODE_CONFIG(balance[1].pin.sclk, PIN_MODE_OUTPUT);
    BALANCE_SCLK_SET(balance[1].pin.sclk,PIN_LOW);
#endif
#ifdef BALANCE_USING_CH2
    BALANCE_PIN_MODE_CONFIG(balance[2].pin.dout, PIN_MODE_INPUT);
    BALANCE_PIN_MODE_CONFIG(balance[2].pin.sclk, PIN_MODE_OUTPUT);
    BALANCE_SCLK_SET(balance[2].pin.sclk,PIN_LOW);
#endif

    return __ok;
}
//所用资源注册
static char res_register(void)
{
    var_list_register();
    pin_list_register();
    return __ok;
}

//加载后自动更新
#if 0
static char auto_lister(void)
{
    return __ok;
}
#endif

//查找设备
static char dev_mount(void)
{

#ifdef BALANCE_USING_CH0
    if (strcmp(balance[0].name, "balance0"))
    {
        LOG_E("do not find device %s", balance[0].name);
        return __exception;
    }
#endif
#ifdef BALANCE_USING_CH1
    if (strcmp(balance[1].name, "balance1"))
    {
        LOG_E("do not find device %s",balance[1].name);
        return __exception;
    }
#endif
#ifdef BALANCE_USING_CH2
    if (strcmp(balance[2].name, "balance2"))
    {
        LOG_E("do not find device %s",balance[2].name);
        return __exception;
    }
#endif

    return __ok;
}

#ifdef BALANCE_USING_CH0
static void b0_thread_entry(void *parameter)
{

    balance_factor_read(&balance[0]);
    while (1)
    {

		if (balance_boot_init(&balance[0]) == __ok)
		{
			if ((!balance[0].sings.balance_calibration)&&(!balance[0].sings.balance_peel))//如果是校准状态则不测量，校准后再开启测量
			{
				balance_weight(&balance[0]);

			}

		}
        rt_thread_mdelay(10);
    }

}
#endif
#ifdef BALANCE_USING_CH1
static void b1_thread_entry(void *parameter)
{
    while (1)
    {
        ;
    }
    rt_thread_mdelay(50);
}
#endif
#ifdef BALANCE_USING_CH2
static void b2_thread_entry(void *parameter)
{
    while (1)
    {
        ;
    }
    rt_thread_mdelay(50);
}
#endif

//创建线程
static char tid_create(void)
{
#ifdef BALANCE_USING_CH0
    rt_thread_t tid = rt_thread_create("balance_0", b0_thread_entry, RT_NULL, 1024, 25, 10);
    if (tid != NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("balance_0 thread is run failed");
        return __error;
    }
#endif
#ifdef BALANCE_USING_CH1
    rt_thread_t tid = rt_thread_create("balance_1", b1_thread_entry, RT_NULL, 1024, 25, 10);
    if (tid != NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("balance_1 thread is run failed");
        return __error;
    }
#endif
#ifdef BALANCE_USING_CH2
    rt_thread_t tid = rt_thread_create("balance_2", b2_thread_entry, RT_NULL, 1024, 25, 10);
    if (tid != NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("balance_2 thread is run failed");
        return __error;
    }
#endif

    return __ok;
}

//载入功能(对外接口)
char balance_load(void)
{
    char ret = __ok;

    ret = res_register();
    if (ret != __ok)
    {
        return __error;
    }
    ret = dev_mount();
    if (ret != __ok)
    {
        return __error;
    }
    ret = tid_create();
    if (ret != __ok)
    {
        return __error;
    }

    return __ok;
}

#endif

