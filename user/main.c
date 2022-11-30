#include <misc.h>
#include <stm32f10x.h>
#include <stm32f10x_exti.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_adc.h>
#include <lcd.h>
#include <Touch.h>
#include <stdio.h>
#include <stdlib.h>

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void EXTI_Configure(void);
void NVIC_Configure(void);


#include <ws2812b.h>

#define NUM_LEDS    12

RGB_t leds[NUM_LEDS];

//---------------------------------------------------------------------------------------------------

uint16_t jodo;
void USART1_Init(void);

void EXTI15_10_IRQHandler(void);

void Delay(void);


void send_com(char buf[]) {
char *s = buf;
while (*s) {
while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
;
USART_SendData(USART1, *s++);
}
}

void send_phone(char buf[]) {
char *s = buf;
while (*s) {
while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
;
USART_SendData(USART2, *s++);
}
}

void set_ENABLE(void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);	 // interrupt
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);     // RCC GPIO E
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);     // RCC GPIO C
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);     // RCC GPIO D
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	 // ADC1
}

void set_PC1(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void set_NVIC(void) {
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
	ADC_Cmd(ADC1, ENABLE);
}
void set_ADC(void) {
	ADC_InitTypeDef ADC_InitStructure;
	ADC_DeInit(ADC1);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_239Cycles5);
	ADC_Init(ADC1, &ADC_InitStructure);
}
void ADC_start(void) {
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}



void ADC1_2_IRQHandler(void) {
	uint16_t input;
	u8 str[10];
	double result;
//
////	while(ADC_GetFlagStatus(ADC1, 0x2)==RESET);
	input = ADC_GetConversionValue(ADC1);
	result = (double)input;
	jodo = result;
	ADC_ClearITPendingBit(ADC1,ADC_IT_EOC);
	//	LCD_DrawRectangle(pix_x, pix_y, pix_x+10, pix_y+10);
}

void USART1_Init(void)
{
  USART_InitTypeDef usart1_init_struct;
  GPIO_InitTypeDef gpioa_init_struct;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO |
 RCC_APB2Periph_GPIOA, ENABLE);

  gpioa_init_struct.GPIO_Pin = GPIO_Pin_9;
  gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
  gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &gpioa_init_struct);
  gpioa_init_struct.GPIO_Pin = GPIO_Pin_10;
  gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
  gpioa_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &gpioa_init_struct);

  usart1_init_struct.USART_BaudRate = 9600;
  usart1_init_struct.USART_WordLength = USART_WordLength_8b;
  usart1_init_struct.USART_StopBits = USART_StopBits_1;
  usart1_init_struct.USART_Parity = USART_Parity_No;
  usart1_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usart1_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_Init(USART1, &usart1_init_struct);
  USART_Cmd(USART1, ENABLE);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
void USART2_Init(void)
{
  USART_InitTypeDef usart2_init_struct;
  GPIO_InitTypeDef gpioa_init_struct;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2|  RCC_APB2Periph_AFIO |
 RCC_APB2Periph_GPIOA, ENABLE);

  // tx, rx ¼³Á¤
  gpioa_init_struct.GPIO_Pin = GPIO_Pin_2;
  gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
  gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &gpioa_init_struct);

  gpioa_init_struct.GPIO_Pin = GPIO_Pin_3;
  gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
  gpioa_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &gpioa_init_struct);

  usart2_init_struct.USART_BaudRate = 9600;
  usart2_init_struct.USART_WordLength = USART_WordLength_8b;
  usart2_init_struct.USART_StopBits = USART_StopBits_1;
  usart2_init_struct.USART_Parity = USART_Parity_No;
  usart2_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usart2_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_Init(USART2, &usart2_init_struct);
  USART_Cmd(USART2, ENABLE);
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

}
void USART1_IRQHandler() {

uint16_t word;
        if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET){
        word = USART_ReceiveData(USART1);
        USART_SendData(USART2, word);
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}

int rgb_count = 0;
unsigned char  RGB_array[4]; 
uint16_t test00 ;
void USART2_IRQHandler(void) {
  uint16_t word;

       if(USART_GetITStatus(USART2,USART_IT_RXNE)!=RESET){
          word =  USART_ReceiveData(USART2);
          USART_SendData(USART1, word);
          test00 = word;
          RGB_array[rgb_count] =  (unsigned char) word;
          
          rgb_count = (rgb_count+1) % 4;
          USART_ClearITPendingBit(USART2, USART_IT_RXNE);
      
       }
}

int darkness = 0;

int scopeValue(int num){
  if(num <0 ){
    num = 0;
  }
  if(num>255){
    num = 255;
  }
  
  return num;
  
}

void turnLED(){
      unsigned char  temp_array[4]; 
      for(int i= 0;i<4;i++){
        temp_array[i] = RGB_array[i];
      }
     
      if( temp_array[3] == 0){
        for(int i=0;i<NUM_LEDS;i++){    
              leds[i].r = 255 ;
              leds[i].g =255 ;
              leds[i].b = 255 ;
        }
         ws2812b_SendRGB(leds, NUM_LEDS);

         return;
      }
     
    while (!ws2812b_IsReady())  ;  // wait
          int brightness = 255- temp_array[3];
          for(int i=0;i<NUM_LEDS;i++){    
            leds[i].r = scopeValue(((255 - temp_array[0]) ) +brightness+ darkness)  ;
            leds[i].g =scopeValue( ((255 - temp_array[1])) + brightness+ darkness); 
            leds[i].b = scopeValue(((255 - temp_array[2]) ) + brightness+ darkness);
          }
        
     
    ws2812b_SendRGB(leds, NUM_LEDS);
}
        





void Delay(void) {
int i;

for (i = 0; i < 20000000; i++) {}
}

int flag_a= 0;
int flag_b=0;
int main() {
   ws2812b_Init();
    SystemInit();
   ws2812b_Init();
   set_ENABLE();
    set_PC1();
    set_ADC();
    set_NVIC();
   ADC_start();
   LCD_Init();
   LCD_Clear(WHITE);   
  //USART1_Init();
    USART2_Init();

    while (1) {
  
        //
        // Fill leds buffer
        //
      
           LCD_ShowNum(200, 1, test00, 4, BLACK, WHITE);
            LCD_ShowNum(200, 30, RGB_array[0], 4,BLACK, WHITE);
            LCD_ShowNum(200, 60, RGB_array[1],4, BLACK, WHITE);
            LCD_ShowNum(200, 90, RGB_array[2],4, BLACK, WHITE);

           turnLED();
          
           
           
           if (jodo <500){
             darkness = 0;
             
           }else if (jodo<900){
             darkness = 90;
             
           }else{
             darkness = 190;
           }
           
           
    
    
    }
}

