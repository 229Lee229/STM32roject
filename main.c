/**********************************************************************
 * flie:  main
 * brief:  
 * Input: 
 * Output:
 * Retval:
 *
 * History--------------------------------------------------------------
 * Version       Date         Name    			Changes and comments
 *=====================================================================
	V1.0		 3/28/2024	  Lee			����������˵�������� �ڲ��˵��ȴ�����
	V1.1		 3/29/2024 	  Lee 			һ�������˹��ܼ�(ö��)	ʹ�ýṹ��λ����д������ ��ʱ������
											TIM�����ж���judge_val����ʱExist(�Ѵ����)����ʾ ÿ5s��תһ�� ��main��ʹ��extern���������ṹ��
											lcd�����ȡ�� ��Ϊ��vcc����  
											����TFT �ı�����
												CS PB11 -> PC15
											 R0/A0 PB10 -> PC14
											   RES PB12 -> cancel
											   
    V1.2		 3/30/2024	  Lee			�������ȼ��������� 		
											��װJR6001��ʼ�������� ��װUSARTx_Init((USART_TypeDef* USARTx, u32 bound)) ʵ�ֿ��ٳ�ʼ��
											��װPWMSG90���� �����ʼ��
    V1.3         3/31/2024	  Lee			��װ���ģ���д�ⲿFlash �Ա�ϵ籣������	
	V1.4		 4/01/2024	  Lee			�ⲿFlash���Ӷ�д�ַ������ݺ��� 
	V1.5		 4/03/2024	  Lee			�������SPI����W25Q64	�����ֻ���11λд�����麯��  ���ܼ���ΪԤ����  
											δ����11λ�ֻ��Ű�Exit�Ļ� �Ȱ����������ٷ���
	V1.6		 4/04/2024	  Lee			������Flash����ɨ�� �Ƿ��д������ ɨ�赽�ַ�'-'��ʾ������  ��ˢ��unavailable ͬʱҲ��ָ����һ��Sector�ĵ�ַ 
											�洢��Flash�ĸ�ʽΪ "�ֻ���(11λ)-��֤��(5λ)"  ��֤����TIM2������Ϊ�������� ʱ���������ɴ��о�
											�������PA7�д���֤�о�
	V2.0		 4/05/2024	  Lee			������ϵ��̨�еĹ���Աģʽ ����ģʽ֧�ֲ鿴ȫ���洢���� �Լ���Ȩ����������  ����Ա����Ϊ1234
											������̴���֤
											���Ӵ����ʾ������֤��
											���Ӵ����������� ����unavailableѡ��
											���Ӵ�ȡ��ʱ������ʾ�û����Ź��ſ��� ȡ��ʱ��Ŀ��������ַ���������ݴ�ǰ��
	V3.0		 4/06/2024	  Lee			�޸�Ŀ��ȡ��������ַ�ĺ�������ת������һ��δ���������, 
											ÿ�ΰѺ���������ת�浽��ʱ������ ���Ѹ�������ַ��� �Ա����������ȷ������ָ�� 
											1-ȡ����ȡ��	3-���	4-��ϵ��̨(����Ա����)
											����ģ�鹦��ʵ�ֻ������98%
											���Ӵ��ʱ�������� Ŀ�ĸ��ض���Ⱥʹ�� �����ⲿ��Ⱥ�Դ˽������� �������Ҫ��֮һ
    V4.0		 4/09/2024    Lee			���ʹ��������������Ե�Ƭ���Ŀ���
											1-��ѯ��֤�� 	2-ʹ����֤���������(������ʵʱ���ⲿFlash���ݽ��и���)
											
	V6.0		 4/11/2024    Lee										
 ***********************************************************************/

#include "stm32f10x.h"                  // Device header
#include "My_include.h"
#include "my_image.h"




u16 TxData_16[11];
u8 RxData_8_match[11];			// ���յ����ֻ��Ž���ƥ��
u8 RxData_8_jdy[11];			// �������� ���յ���תΪ1���ֽ�1������
u8 TxData_8_Code[5];			// ������֤��
u8 RxData_8_Code[5];			// ������֤��


/* �����õ����� */
u16 unavailable = 0;
extern struct TIM_judge_val TIM_judge_val_1;
// u8 Exist_TIM2_limit = 1;
bool lcd_Exist_flag = 0;		/* �ж�λ ��ʾExist���� ÿ5���л� */

/*  key_Num_1 Ϊ���վ�������еĹ��ܰ��� */
struct key_Num{
	int8_t key_fun : 5;			/* ���ܰ��� Exit Reinput Moveup MoveDown Enter */
	u8 key_limit : 2;			/* ���ư��� 1-�ɽ�����һ���˵� 0-��ʾ�Ѿ�����һ���˵� ���ɲ��� ����һ���˵� ���˵��������� ���Ʋ���1~4 */
	u8 key_lcd_MainMenu : 4;	/* ���˵����� ����1~4 */
	int8_t key_temp;			/* ��ʱ���� ���ڴ洢Matrix_Key�ķ���ֵ */
}key_Num_1;



//enum key_fun_Num{				/* ���ܰ��� */
//	key_MoveUp = 10,
//	key_MoveDown,
//	key_Enter,
//	key_Reinput ,
//	key_Close,
//	key_Exit
//};



int main(void){
	SystemInit();				//��ʼ��RCC ����ϵͳ��ƵΪ72MHZ
	delay_init(72);	     		//��ʱ��ʼ��
	
	/*********************************************************/	
	LCD_Init();
	lcd_menu_Init(LCD_DISPLAY_Init_loading);			// Init... ����
	SG90PWM_Init();
	Init_Refresh_unavailable();	// W25��ʼ�� ��unavailableˢ�� 
	Exist_TIM_Init();			/* ��ʾ��ǰ�������� ��ʱ����ʼ�� */
	My_JR6001_Init(30);			/* NOTE ����ѡ������������ param:���� */
	Matrix_ssKey_Pin_Init();
	Serial_JDY31_Init();		// �������ڳ�ʼ��
	
	/*********************************************************/	

	
	key_Num_1.key_limit = 1;	// ����λ��һ ��ʾ�ɽ������˵����� 
	



	delay_ms(1500);
	
	
	lcd_menu_Init(LCD_DISPLAY_WEL_menu);			// ѡ�����
	
	USART_SendString(UART1_JR6001,JR6001_PLAY_OPEN_WEL);		// ���ܴ����ӭ�� 
	//	USART_SendString(UART1_JR6001,JR6001_PWD_ERROR);		


	/* �ϵ�SG90��λ */
	SG90_Start_Zeroing_Init();
	// SG90_Start_Zeroing_test_180();
	while(1){

		/* ���ռ�ֵ */
		key_Num_1.key_temp = Matrix_Key_Scan2();
		

		/* �ж�_����ѡ��1~4 */ 
		if(key_Num_1.key_temp >= 1 && key_Num_1.key_temp <= 4 &&  key_Num_1.key_limit == 1)	key_Num_1.key_lcd_MainMenu = key_Num_1.key_temp;		/* ������ѡ��1~4 ȡ�������ϵ */
		else key_Num_1.key_lcd_MainMenu = false;
		
		
//		/* �ж�_���ܰ��� Exit */
//		if(key_Num_1.key_temp == key_Exit) key_Num_1.key_fun = key_Num_1.key_temp;							/* Exit-16 */
//		else key_Num_1.key_fun = false;

		// ʱ��Ƭ��ѯ

		/* ִ��_�������˵�  Exit */
//		if(key_Num_1.key_fun == key_Exit && key_Num_1.key_limit == 0)	{
//			lcd_menu_Init(LCD_DISPLAY_WEL_menu);		/* Exit-15 */
//			key_Num_1.key_limit = 1;					/* ����λ��һ �ɲ���MailMenu */
//		}
		
		
		/* �ж�_TIM_Exist_flag(ÿ5s��תһ����ʾ) �Ƿ����5s ��0 ��ʾExist���� */
		if(TIM_judge_val_1.lcd_Exist_flag_Num  > 5)	TIM_judge_val_1.lcd_Exist_flag_Num  = 0;
		
		
		/* ִ��_�ж��Ƿ�5s ���˷�תһ����ʾ ��ʾExist����*/
		if(TIM_judge_val_1.lcd_Exist_flag_Num  == 5 && key_Num_1.key_limit == 1){
			lcd_Exist_flag =! lcd_Exist_flag;
			TIM_judge_val_1.lcd_Exist_flag_Num  = 0;
			lcd_menu_Exist(lcd_Exist_flag);
		}		





		// ���˵��˵�ѡ��
		switch(key_Num_1.key_lcd_MainMenu){
			/* PickUp code PickUp*/
			case 1:		
						key_Num_1.key_limit = 0;
						lcd_menu_1st();				// һ���˵�
						key_Num_1.key_limit = 1;

						break;
			/* Scan the QR code to pick up the package */
			case 2:		
				/*
						USART_SendString(UART1_JR6001,JR6001_SCAN_QR);		// ���ܴ����ӭ�� 
						LCD_Clear(WHITE);
						LCD_Fill(0,20,128,128,WHITE); //�����ʾ����                           
                        LCD_DrawMatrixCode(15,20,100,100,testQr_image,true);*///��ʾ��ά��ͼƬ ��С90*90  
						key_Num_1.key_limit = 0;
						lcd_menu_2nd();
						key_Num_1.key_limit = 1;
						break;
			/* Storing objects */
			case 3:						
						key_Num_1.key_limit = 0;
						lcd_menu_3rd();
						key_Num_1.key_limit = 1;
						break;
			/* Contact the backend */
			case 4:			
						
						key_Num_1.key_limit = 0;
						lcd_menu_4th();
						key_Num_1.key_limit = 1;
						// Generate_data_reports(CLEAR_DATA);
						// Generate_data_reports(DISPLAY_DATA);
						break;
			default:break;
		}
		
		
		
		
	}	
	
	
}
