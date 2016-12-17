#include "pwm.h"
#include "usart.h"
 

int Check_fre(u8 len)
{
	u8 data[4]={'f','r','e',':'};
	int i;
	for (i=0;i<len-3;i++)
	 if(USART_RX_BUF[i]==data[0]
		 &&USART_RX_BUF[i+1]==data[1]
	     &&USART_RX_BUF[i+2]==data[2]
	     &&USART_RX_BUF[i+3]==data[3])
	 return i+3;
	return 0;
}
int Check_per(u8 len)
{
	u8 data[4]={'p','e','r',':'};
	int i;
	for (i=0;i<len-3;i++)
	 if(USART_RX_BUF[i]==data[0]
		 &&USART_RX_BUF[i+1]==data[1]
	     &&USART_RX_BUF[i+2]==data[2]
	     &&USART_RX_BUF[i+3]==data[3])
	 return i+3;
	return 0;
}
void rFail(void)
{
	printf("\nNot Legal\n");
	USART_RX_STA=0;
}
void Check(u8 len)
{
	//May use memcpy,strcpy,strcmp
	uint32_t _arr;
	u16 _CCR1;
	int p_arr=Check_fre(len);
	int p_CCR=Check_per(len);
	int i,_f=0,_c=0;
	if (!p_arr||!p_CCR) {rFail();return;}
	for (i=p_arr+1;USART_RX_BUF[i]!=' ';i++)
	 _f=_f*10+USART_RX_BUF[i]-48;
	for (i=p_CCR+1;i<len;i++)
	 _c=_c*10+USART_RX_BUF[i]-48;
	_c=100-_c;
	_arr=1000000/_f;
	_CCR1=_arr*(_c/100.f);
	TIM_SetAutoreload(TIM3,_arr-1);
	TIM_SetCompare1(TIM3,_CCR1-1);
	USART_SendString(UART5,"\n%d %d\n",_arr,_CCR1);
	USART_RX_STA=0;
}

//TIM3 PWM部分初始化 
//PWM输出初始化
//arr：自动重装值
//psc：时钟预分频数
void TIM3_PWM_Init(u32 arr,u32 psc)
{		 					 
	//此部分需手动修改IO口设置
	
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  	//TIM3时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); 	//使能PORTC时钟	
	

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;           //GPIOC7
	GPIO_Init(GPIOC,&GPIO_InitStructure);              //初始化PC7
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; 
	GPIO_Init(GPIOC,&GPIO_InitStructure);              //初始化PC6
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_TIM3); //GPIOC6复用为定时器3
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_TIM3); //GPIOC6复用为定时器3
	  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //计数模式
	TIM_TimeBaseStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);//初始化定时器3
	
	//初始化TIM3 Channel1 PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //选择定时器模式:TIM脉冲宽度调制模式2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM3 OC1
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM3 OC2
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR1上的预装载寄存器
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
 
    TIM_ARRPreloadConfig(TIM3,ENABLE);//ARPE使能 
	
	TIM_Cmd(TIM3, ENABLE);  //使能TIM3  							  
}  

void TIM8_PWM_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	u16 period;
	u16 cmp;
	period = 1000000/1000 - 1;  //10000Hz
	cmp = period*5000/10000;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8,ENABLE);  	//TIM8时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);   //使能PORTA时钟	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); 	//使能PORTC时钟	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE); 	//使能PORTG时钟	
	

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 ;           //GPIOC8
	GPIO_Init(GPIOC,&GPIO_InitStructure);              //初始化PC8 爬杆电机 PC9 装螺旋桨
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_TIM8); //GPIOC8复用为定时器8
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_TIM8);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       // 继电器等
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    //
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_11|GPIO_Pin_15|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14; 
	GPIO_Init(GPIOG,&GPIO_InitStructure); //PG3 PG4 爬杆方向  PG11 爬杆刹车 PG15 磁铁
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 ;  //PA8 PA9 打脸方向
	GPIO_Init(GPIOA,&GPIO_InitStructure);




	TIM_TimeBaseStructure.TIM_Prescaler = 167;  //168/(167+1)=1M
	TIM_TimeBaseStructure.TIM_Period = period;  //freq = 1000000/(TIM_Period+1)   TIM_Period = 1000000/freq - 1
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM8,&TIM_TimeBaseStructure);
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = cmp;
	TIM_OC3Init(TIM8,&TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM8, TIM_OCPreload_Enable);
	TIM_OC4Init(TIM8,&TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM8, TIM_OCPreload_Enable);
	
	TIM_ARRPreloadConfig(TIM8,ENABLE);
	TIM_Cmd(TIM8,ENABLE);
    TIM_CtrlPWMOutputs(TIM8,ENABLE);

}


