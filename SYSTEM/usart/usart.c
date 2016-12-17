#include "sys.h"
#include "usart.h"	
#include "stdarg.h"
#include "delay.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ʹ��	  
#endif
 
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
_ttywrch(int ch){
	ch=ch;              //aware
}

//�ض���fputc���� 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
	USART1->DR = (u8) ch;      
	return ch;
}
#endif
 
#if EN_USART5_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	

//��ʼ��IO ����1 
//bound:������
void uart_init(u32 bound){     //MOTOR_USARTx USART1
   //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE); //ʹ��GPIODʱ��	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //ʹ��GPIOCʱ��
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5,ENABLE);	//ʹ��USART5ʱ��
	
	//GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);   //?
	//GPIO_PinRemapConfig(GPIO_Remap_USART2,ENABLE);   //?
	//GPIO_PinRemapConfig(GPIO_Remap_USART3,ENABLE);   //?
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 ;
	GPIO_Init(GPIOD,&GPIO_InitStructure); //��ʼ��PD2
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_UART5); //GPIOD2����ΪUSART5

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOC,&GPIO_InitStructure); //��ʼ��PC12
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_UART5); //GPIOC12����ΪUSART5
//////////////////////////////////////////////////////////////////////////////////////////	
	
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
   
	USART_Init(UART5, &USART_InitStructure); //��ʼ������5
    USART_Cmd(UART5, ENABLE);  //ʹ�ܴ���5
	//USART_ClearFlag(USART1, USART_FLAG_TC);  	TC�������  1<<6
	
#if EN_USART5_RX	
	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);//��������ж�

	//Usart5 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;//����5�ж�ͨ��   
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
#endif	
}



void UART5_IRQHandler(void)                	//����5�жϷ������
{
	u8 Res,i,len; 
	uint32_t ccr1,ccr2;
#if SYSTEM_SUPPORT_OS 		//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)  
       // RXNE׼���ö�ȡ���յ������� 1<<5
	{
		Res =USART_ReceiveData(UART5);//(USART1->DR);	//��ȡ���յ�������   �жϱ�־�����
		len=USART_RX_STA&0x3fff;
		if((USART_RX_STA&0x8000)==0)//����δ���   0x8000=1<<15
		{
			if(USART_RX_STA&0x4000)//���յ���0x0d  0x4000=1<<14
			{
				if(Res!=0x0a)USART_RX_STA=0;//���մ���,���¿�ʼ
				else {    USART_RX_STA|=0x8000;                    //check
				     }	//���������     
			}
			else //��û�յ�0X0D
			{	
				if(Res==0x0d)USART_RX_STA|=0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
				}		 
			}
		}   		 
     } 
#if SYSTEM_SUPPORT_OS 	//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
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
 



