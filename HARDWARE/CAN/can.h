/******************************代码简要说明**********************************/
/* 功能：CAN收发
 * 使用的外设:CAN1、CAN2、NVIC中断控制器
 * 使用的引脚:CAN1: CAN_RX PA11 , CAN_TX PA12
 *            CAN2: CAN_RX PB12 , CAN_TX PB13
 * 使用说明:    本代码可以选择使用CAN1或者CAN2，但是由于时间问题，我就没有写实现两个
 *          可以同时使用的代码。宏CAN_SELECT可以选择使用哪一个CAN。配置宏CAN1_ID
 *          或者CAN2_ID可以设置自己所发送的数据包的标识符。CAN_MODE_X选择CAN所使
 *          用的模式，通常为CAN_Mode_Normal模式，即正常模式。CAN总线的时序非常重
 *          要：本代码是在APB1时钟为42MHz下配置的CAN总线时序，宏CAN_SJW_X、
 *          CAN_BS1_X、CAN_BS2_X、CAN_PRESCALER_X可以配置CAN总线的位时序和通信速
 *          率。在本代码中，就只使用一个过滤器，过滤模式是32位掩码模式，可通过宏
 *          CAN_FILTER_ID、CAN_FILTER_MASK来设置需要过滤出的标识符的数据包。
 *              本代码是以中断的方式来读取数据的。每次接收到数据之后，会调用一个
 *          函数，该函数是使用者自己定义的函数，在对CAN进行初始化时，把该函数地址
 *          作为参数来执行函数can_init(msg_rcv_func),msg_rcv_func()函数就会在接收到
 *          can总线数据之后自动调用。msg_rcv_func()函数的参数必须为CanRxMsg类型而
 *          且返回值是void，也即函数必须定义成这样(函数名无所谓):
 *                      void msg_rcv_func(CanRxMsg can_rx_msg){
 *                          
 *                      }
 *              结构体can_rx_msg是CAN总线接收到的数据，可以在函数msg_rcv_func()
 *          里把数据复制到其它地方来存储，也可以在函数里直接对数据进行处理然后
 *          执行相应动作。不过我并不建议后一种做法，因为msg_rcv_func()函数是在一
 *          个中断函数里面调用的，也即函数msg_rcv_func()的执行过程就是中断处理过
 *          成，在中断里停留太久，很容易使得工程出现一些意想不到的问题。中断的优
 *          先级可以通过宏CAN_NVIC_IRQPP、CAN_NVIC_IRQSP来配置。
 * 16年5月22号更新:can_send_msg()函数增加了标识符参数
 * 16年5月25号更新:修改了can_init函数，该函数没有参数了。新增了can_add_callback()
 *          函数，该函数可以增加对应标识符的callback函数。如执行函数
 *          can_add_callback(0x11,msg_rcv_func)函数之后，每当接收到标识符是0x11的
 *          CAN数据包时，都会自动调用函数msg_rcv_func(CanRxMsg can_rx_msg)
 *          
 *          
 */
#include "stm32f4xx.h"
#include "stm32f4xx_can.h"

#define CAN_SELECT 1    //1 则使用CAN1 2则使用CAN2 

#define CAN1_ID 0x12     //stm的CAN总线的标识符
#define CAN2_ID 0x12

#define CAN_MODE_X CAN_Mode_Normal  //模式
//CAN总线时序和速率配置(此时配为1Mbps)
#define CAN_SJW_X CAN_SJW_1tq
#define CAN_BS1_X CAN_BS1_9tq   //时间段1的时间单元
#define CAN_BS2_X CAN_BS2_4tq   //时间段2的时间单元
#define CAN_PRESCALER_X 3     //分频系数  42MHz
/* tq = (CAN_PRESCALER_X + 1)*tpclk
 * 速率 = 1/((CAN_SJW_X + CAN_BS1_X + CAN_BS2_X)*tq)    588M
 */

//过滤器设置(32位)
#define CAN_FILTER_ID 0x00000000   //如果都为0，则接收任意标识符的数据包
#define CAN_FILTER_MASK 0x00000000
//中断优先级设置
#define CAN1_NVIC_IRQPP 1              //CAN1中断抢断优先级
#define CAN1_NVIC_IRQSP 1              //CAN1中断子优先级
#define CAN2_NVIC_IRQPP 1              //CAN2中断抢断优先级
#define CAN2_NVIC_IRQSP 1              //CAN2中断子优先级

#define can_MaxBUFF 100

//#define device_id 0x3 

#if CAN_SELECT == 1
    #define CANX CAN1
    #define CAN_ID CAN1_ID
#elif CAN_SELECT == 2
    #define CANX CAN2
    #define CAN_ID CAN2_ID
#endif

typedef struct msg{
	u8 device_id;
	int len;
	u8 msg[can_MaxBUFF];
	struct msg* next;
}Can_msg,*Can_msg_list;

typedef struct{
    void (*msg_rcv_callback)(u8 device,int len,u8* msg);
	Can_msg_list *head;
}can_callback_struct;

int can_init();
void can_rcc_config();
void can_gpio_config();
void can_nvic_config();
int can_send_msg(u8 can_id,u8 *msg,u8 len);  //发送邮箱
int can_rcv_msg();   //接收信息
int can_add_callback(u8 can_id,void *msg_rcv_callback_func);
int can_send(u8 can_id,u8 device,u8 *msg); 
int can_send_len(u8 can_id,u8 device,u8 *msg,int len);

