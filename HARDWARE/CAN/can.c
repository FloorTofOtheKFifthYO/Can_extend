#include "can.h"
#include "stm32f4xx.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_it.h"
#include "stdlib.h"
#include "misc.h"

extern void USART_SendString(USART_TypeDef* USARTx, char *fmt, ...);
static void (*msg_rcv_callback)(CanRxMsg *can_rx_msg); //attention
static CanRxMsg g_can_rx_msg;
can_callback_struct can_cb_array[256]; 

/* 函数名：void can_init(void *)
 * 功能：can总线初始化
 * 参数：
 * 返回值：初始化成功则返回1
 */
int can_init(){
    int i;
    CAN_InitTypeDef can_init_struct;

    //msg_rcv_callback = msg_rcv_callback_func;
    can_rcc_config();
    can_gpio_config();
    CAN_DeInit(CANX); 
    CAN_StructInit(&can_init_struct);
    can_init_struct.CAN_TTCM = DISABLE;  //非时间触发通信模式
    can_init_struct.CAN_AWUM = DISABLE;  //睡眠模式通过软件唤醒
    can_init_struct.CAN_ABOM = DISABLE;  //软件自动离线管理
    can_init_struct.CAN_NART = DISABLE;   //禁止报文自动传送
    can_init_struct.CAN_RFLM = DISABLE;  //报文不锁定，新的覆盖旧的
    can_init_struct.CAN_TXFP = ENABLE;  //优先级由报文标识符决定
    can_init_struct.CAN_Mode = CAN_MODE_X;     //模式设置
    can_init_struct.CAN_SJW  = CAN_SJW_X;     //同步跳跃宽度
    can_init_struct.CAN_BS1  = CAN_BS1_X; //范围CAN_BS1_1tq ~ CAN_BS1_16tq
    can_init_struct.CAN_BS2  = CAN_BS2_X; //范围CAN_BS1_1tq ~ CAN_BS1_16tq
    can_init_struct.CAN_Prescaler = CAN_PRESCALER_X;  //分频系数(Fdiv)CAN_PRESCALER_X + 1
    CAN_Init(CANX,&can_init_struct);
      //总共有28组过滤器  
    CAN1->FMR |= 1; //过滤器组工作在初始化模式  
    CAN1->FMR &= 0xffffc0ff;//对FMR寄存器中CAN2SB[5:0]进行清零
    CAN1->FMR |= (14<<8);  //CAN2的过滤器组从14开始  
    CAN1->FM1R &= ~(1<<14);//过滤器组14的寄存器工作在屏蔽模式  
    CAN1->FS1R |= (1<<14);//过滤器组14为单个32位寄存器
    CAN1->FFA1R = 0x00000000;//全部关联到FIFO0
#if CAN_SELECT == 1
    CAN1->FA1R &= ~(1<<0);//禁用过滤器组0
    CAN1->sFilterRegister[0].FR1 &= 0x00000000;  
    CAN1->sFilterRegister[0].FR2 &= 0x00000000;  
    CAN1->sFilterRegister[0].FR1 |= CAN_FILTER_ID;  
    CAN1->sFilterRegister[0].FR2 |= CAN_FILTER_MASK;  
    CAN1->FA1R |= (1<<0);//使能过滤器组0
#elif CAN_SELECT == 2
    CAN1->FA1R &= ~(1<<14);//禁用过滤器组14  
    CAN1->sFilterRegister[14].FR1 &= 0x00000000;  
    CAN1->sFilterRegister[14].FR2 &= 0x00000000;  
    CAN1->sFilterRegister[14].FR1 |= CAN_FILTER_ID;  
    CAN1->sFilterRegister[14].FR2 |= CAN_FILTER_MASK;  
    CAN1->FA1R |= (1<<14);//使能过滤器组14  
#endif
    CAN1->FMR &= ~1; //过滤器组正常工作
    CAN_ITConfig(CANX,CAN_IT_FMP0,ENABLE);  //FIFO0消息挂号中断允许

    for(i = 0;i < 256;i++){
        can_cb_array[i].msg_rcv_callback = 0;
		can_cb_array[i].head=0;
    }
    can_nvic_config();
    return 1;
}

/* 函数名：int can_add_callback(u8 can_id,void *msg_rcv_callback_func)
 * 功能：增加对应标识符的CALLBACK函数
 * 参数：u8 can_id,接收消息的CALLBACK函数对应的标识符
         void *msg_rcv_callback_fuc 邮件接收回调函数的指针，每当接收到所需要的
 *       消息时会自动调用该函数。注意：这里是在接收中断里调用这个函数的，所以该
 *       函数不宜执行太久。如果该函数指针为NULL，则不使用邮件接收中断。
 * 返回值：-1,已经存在函数
 *          1,成功增加函数
 */
int can_add_callback(u8 can_id,void *msg_rcv_callback_func){
    if(can_cb_array[can_id].msg_rcv_callback != 0){
        return -1;
    }
    can_cb_array[can_id].msg_rcv_callback = msg_rcv_callback_func;
    return 1;
}

/* 函数名：void can_rcc_config()
 * 功能：can总线所需要外设的所有时钟设置
 * 参数：无
 * 返回值：无
 */
void can_rcc_config(){
#if CAN_SELECT == 1
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1,ENABLE);
#elif CAN_SELECT == 2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1,ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2,ENABLE);
#endif
}

/* 函数名：void can_gpio_config()
 * 功能：配置CAN总线所需要的引脚
 * 参数：
 * 返回值：
 */
void can_gpio_config(){
    GPIO_InitTypeDef gpio_init_stru;
    gpio_init_stru.GPIO_Mode = GPIO_Mode_AF;
    gpio_init_stru.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_stru.GPIO_OType = GPIO_OType_PP;  //推挽输出
    gpio_init_stru.GPIO_PuPd = GPIO_PuPd_UP;  //上拉
#if CAN_SELECT == 1
    gpio_init_stru.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;   //PA11 PA12
    GPIO_Init(GPIOA,&gpio_init_stru);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource11,GPIO_AF_CAN1);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource12,GPIO_AF_CAN1);
#elif CAN_SELECT == 2
    gpio_init_stru.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;   //PB12 PB13
    GPIO_Init(GPIOB,&gpio_init_stru);
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource12,GPIO_AF_CAN2);
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource13,GPIO_AF_CAN2);
#endif
}

/* 函数名：void can_nvic_config()
 * 功能：设置can的中断控制器nvic
 * 参数：
 * 返回值：
 */
void can_nvic_config(){
    NVIC_InitTypeDef nvic_init_struct;
#if CAN_SELECT == 1
    nvic_init_struct.NVIC_IRQChannel = CAN1_RX0_IRQn;
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority = CAN1_NVIC_IRQPP;
    nvic_init_struct.NVIC_IRQChannelSubPriority = CAN1_NVIC_IRQSP;
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_struct);
#elif CAN_SELECT == 2 
    nvic_init_struct.NVIC_IRQChannel = CAN2_RX0_IRQn;
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority = CAN2_NVIC_IRQPP;
    nvic_init_struct.NVIC_IRQChannelSubPriority = CAN2_NVIC_IRQSP;
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_struct);
#endif 
}

/* 函数名：int can_send_msg()
 * 功能：can发送一组数据
 * 参数：u8  can_id 数据包的标识符
 *       u8* msg 数据数组
 *       u8  len 数据数组长度，最大为8
 * 返回值：-1,表示发送失败
 *         1 ,表示发送成功
 */
int can_send_msg(u8 can_id,u8 *msg,u8 len){
    CanTxMsg tx_msg;
    u8 mbox;
    u16 i = 0;

    tx_msg.StdId = can_id;
    tx_msg.IDE = CAN_Id_Standard;   //使用标准标识符
    tx_msg.RTR = CAN_RTR_Data;   //消息类型为数据帧
    tx_msg.DLC = len;   //消息长度
    for(i = 0;i < len;i++){
        tx_msg.Data[i] = msg[i];
    }
    mbox = CAN_Transmit(CANX,&tx_msg);  //开始发送消息
    i = 0;
    while((CAN_TransmitStatus(CANX,mbox) == CAN_TxStatus_Failed) && (i < 0xfff))i++;  //waiting
    if(i >= 0xfff) return -1;
    return 1;
}

Can_msg* check(u8 Can_id,u8 device){
	Can_msg* p;
	p=(Can_msg_list)can_cb_array[Can_id].head;
	if (p==0) return 0;
	while (p>0){
		if (device==p->device_id)
			return p;
		p=p->next;
	} 
	return 0;
}

void Insert(Can_msg* p,CanRxMsg *rcv_msg){
	int i;
	if (p==0){
		p=(Can_msg *)malloc(sizeof(Can_msg));
		p->device_id=rcv_msg->Data[0]>>1;
		p->len=rcv_msg->DLC-1;
		for (i=0;i<p->len;i++) p->msg[i]=rcv_msg->Data[i+1];
		p->next=(Can_msg*)can_cb_array[rcv_msg->StdId].head;
		can_cb_array[rcv_msg->StdId].head=(Can_msg_list*)p;
	}
	else{
		i=p->len;
		p->len+=rcv_msg->DLC-1;             //-1
		while(i<p->len){
			p->msg[i]=rcv_msg->Data[i+rcv_msg->DLC-p->len];   
			i++;}
	}
}

#if CAN_SELECT == 1
void CAN1_RX0_IRQHandler(void){
#elif CAN_SELECT == 2
void CAN2_RX0_IRQHandler(void){
#endif
	u8 i,device;
	Can_msg *p,*tmp;
    if(CAN_GetITStatus(CANX,CAN_IT_FMP0) != RESET){ //中断标志
        CAN_Receive(CANX,CAN_FIFO0,&g_can_rx_msg);  //接收
        if(can_cb_array[g_can_rx_msg.StdId].msg_rcv_callback != 0){
			device=g_can_rx_msg.Data[0]>>1;
			p=check(g_can_rx_msg.StdId,device);
			Insert(p,&g_can_rx_msg);
			if (p==0) p=(Can_msg*)can_cb_array[g_can_rx_msg.StdId].head;
			if (g_can_rx_msg.Data[0]&1){   //aware
				(can_cb_array[g_can_rx_msg.StdId].msg_rcv_callback)(p->device_id,p->len,p->msg);
				tmp=(Can_msg *)can_cb_array[g_can_rx_msg.StdId].head;
				if (tmp==p) {can_cb_array[g_can_rx_msg.StdId].head=0;free(p);}
				else{
					while (tmp->next!=p) tmp=tmp->next;
					tmp->next=p->next;
					free(p);}
			}
        }
        CAN_ClearITPendingBit(CANX,CAN_IT_FMP0);
    }
}

int can_send(u8 can_id,u8 device,u8 *msg){
	u8 tmp[8];
	int i=1,p=0;
	tmp[0]=device<<1;
	while (msg[p]!='\n'){   //最后一位结束
		tmp[i]=msg[p];
		if (i==7) { 
			if (can_send_msg(can_id,tmp,8)==-1) return -1;
			tmp[0]=device<<1;
			USART_SendString(UART5,"\nSend %d\n",p);
     		i=0;}
		i++;p++;
	}
	tmp[0]|=1;USART_SendString(UART5,"\nSend over\nlen:%d\ni:%d\n",p,i);
	if (i>1) if (can_send_msg(can_id,tmp,i)==-1) return -1;
	return 1;
}

int can_send_len(u8 can_id,u8 device,u8 *msg,int len){
	u8 tmp[8];
	int i=1,p=0;
	device=msg[p++];
	tmp[0]=device<<1;
	while (p<len){   //最后一位结束
		if (msg[p]=='\n') {tmp[0]|=1;i+=7;}
		else tmp[i]=msg[p];
		if (i>=7) { 
			if (i==7)
				{if (can_send_msg(can_id,tmp,8)==-1) return -1;}
			else 
				if (can_send_msg(can_id,tmp,i-7)==-1) return -1;
			tmp[0]=device<<1;
			USART_SendString(UART5,"\nSend %d\n",p);
     		i=0;}
		i++;p++;
	}
	USART_SendString(UART5,"\nSend over\nlen:%d\ni:%d\n",p,i);
	if (i>1) if (can_send_msg(can_id,tmp,i)==-1) return -1;
	return 1;
}
