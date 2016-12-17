#include "sys.h"
#include "usart.h"	
#include "stdarg.h"
#include "delay.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif
 
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
_ttywrch(int ch){
	ch=ch;              //aware
}

//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (u8) ch;      
	return ch;
}
#endif
 
#if EN_USART5_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound){     //MOTOR_USARTx USART1
   //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE); //使能GPIOD时钟	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //使能GPIOC时钟
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5,ENABLE);	//使能USART5时钟
	
	//GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);   //?
	//GPIO_PinRemapConfig(GPIO_Remap_USART2,ENABLE);   //?
	//GPIO_PinRemapConfig(GPIO_Remap_USART3,ENABLE);   //?
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 ;
	GPIO_Init(GPIOD,&GPIO_InitStructure); //初始化PD2
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_UART5); //GPIOD2复用为USART5

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOC,&GPIO_InitStructure); //初始化PC12
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_UART5); //GPIOC12复用为USART5
//////////////////////////////////////////////////////////////////////////////////////////	
	
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
   
	USART_Init(UART5, &USART_InitStructure); //初始化串口5
    USART_Cmd(UART5, ENABLE);  //使能串口5
	//USART_ClearFlag(USART1, USART_FLAG_TC);  	TC发送完成  1<<6
	
#if EN_USART5_RX	
	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart5 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;//串口5中断通道   
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
#endif	
}



void UART5_IRQHandler(void)                	//串口5中断服务程序
{
	u8 Res,i,len; 
	uint32_t ccr1,ccr2;
#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)  
       // RXNE准备好读取接收到的数据 1<<5
	{
		Res =USART_ReceiveData(UART5);//(USART1->DR);	//读取接收到的数据   中断标志已清除
		len=USART_RX_STA&0x3fff;
		if((USART_RX_STA&0x8000)==0)//接收未完成   0x8000=1<<15
		{
			if(USART_RX_STA&0x4000)//接收到了0x0d  0x4000=1<<14
			{
				if(Res!=0x0a)USART_RX_STA=0;//接收错误,重新开始
				else {    USART_RX_STA|=0x8000;                    //check
				     }	//接收完成了     
			}
			else //还没收到0X0D
			{	
				if(Res==0x0d)USART_RX_STA|=0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//接收数据错误,重新开始接收	  
				}		 
			}
		}   		 
     } 
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
} 

#endif	

 void USART_SendString(USART_TypeDef* USARTx, char *fmt, ...)
 {
	char buffer[STR_BUFFER_LEN+1];  // 
	u8 i = 0;
	
	va_list arg_ptr;
	va_start(arg_ptr, fmt);  
	vsnprintf(buffer, STR_BUFFER_LEN+1, fmt, arg_ptr);
	USART_ClearFlag(USARTx,USART_FLAG_TXE);
	while ((i < STR_BUFFER_LEN) && buffer[i])
	{
		if(buffer[i] == '\n'){
        USART_SendData(USARTx,(u8)'\r');
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET); 
        USART_SendData(USARTx,(u8)buffer[i++]);
        }else{
	    USART_SendData(USARTx, (u8) buffer[i++]);
        }
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET); 
	}
	va_end(arg_ptr);
 }
 



