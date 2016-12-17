#include "sys.h"
#include "delay.h"
#include "usart.h"
//#include "pwm.h"
#include "can.h"

#define can_rx 0
#define can_tx 1-can_rx

#if can_rx
uint8_t rcv[8],rcv_len;
void msg_rcv_func(u8 device,int len,u8* msg){
	int i;
	USART_SendString(UART5,"\nDevice_id:%d\n",device);
	USART_SendString(UART5,"msg:");
	for (i=0;i<len;i++){
		USART_SendString(UART5,"%c",msg[i]);
	}USART_SendString(UART5,"\n");
}
#endif

#if can_tx

void can_SendNum(int p){
	u8 i;u8 msg[8];
	i=0;
	if (p==0)
		i=1;
	while (p>0){
		msg[i]=p%10;
		p/=10;
		i++;
	}
	//USART_SendString(UART5,"\nlen=%d\n",i);
	if (can_send_msg(0x01,msg,i)==1) USART_SendString(UART5,"Sent!");
	else USART_SendString(UART5,"Not sent!");
}
#endif 

int main(void)
{   //system_stm32f4xx.c #define PLL_M=8 PLL_N=336 HSE -> SYSCLK 168MHZ
	// RCC->CFGR |= RCC_CFGR_PPRE2_DIV2; APHB1_CLK 84MHZ
	//AHB 168MHZ  APB2 84MHZ  APB1 42MHZ
	int i;
#if can_rx
	rcv_len=0;
#endif
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2 2:2   
	delay_init(168);  //初始化延时函数
	uart_init(115200);//初始化串口波特率为115200
	can_rcc_config();
	can_gpio_config();
	//can_nvic_config();
	if (can_init()==1) USART_SendString(UART5,"CAN1_init compelete\n");
	else USART_SendString(UART5,"CAN1_init failed\n");
	i=0;
#if can_rx 
	if  (can_add_callback(0x01,msg_rcv_func)==1) USART_SendString(UART5,"added\n");
	else USART_SendString(UART5,"add failed\n");
#endif
	
    while(1) {
//#if can_rx
//		if (rcv_len>0){
//			for (i=rcv_len-1;i>=0;i--){
//				USART_SendString(UART5,"%d",rcv[i]);
//				rcv[i]=0;
//			}
//			USART_SendString(UART5,"\n");
//			rcv_len=0;
//		}
//#endif
//////////////////////////////////////////////////////////////////////////////////////////////////	
#if can_tx
//		can_SendNum(i);
//		if(can_send(0x01,i,(u8 *)"abcdefghijklmnopqrstuvwxyz\n")==-1)
//			USART_SendString(UART5,"Send String Failed\n");
//		if (i==9) i=0;
//		else
//		 i++;
//	    delay_ms(1000);
	if(USART_RX_STA&0x8000){
		if (USART_RX_BUF[(USART_RX_STA&0x3fff)-1]=='!')
			USART_RX_BUF[(USART_RX_STA&0x3fff)-1]='\n';
		if (can_send_len(0x01,i,USART_RX_BUF,USART_RX_STA&0x3fff)==-1)
			USART_SendString(UART5,"Send String Failed\n");
		USART_RX_STA=0;
	}
#endif
	}
}
