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
	V1.0		 3/28/2024	  Lee			基本完成主菜单界面操作 内部菜单等待完善
	V1.1		 3/29/2024 	  Lee 			一、定义了功能键(枚举)	使用结构体位域进行存放限制 临时变量等
											TIM函数中定义judge_val来定时Exist(已存件数)的显示 每5s翻转一次 在main中使用extern进行声明结构体
											lcd背光脚取消 改为接vcc常亮  
											二、TFT 改变引脚
												CS PB11 -> PC15
											 R0/A0 PB10 -> PC14
											   RES PB12 -> cancel
											   
    V1.2		 3/30/2024	  Lee			考虑优先级分组问题 		
											封装JR6001初始化函数等 封装USARTx_Init((USART_TypeDef* USARTx, u32 bound)) 实现快速初始化
											封装PWMSG90函数 舵机初始化
    V1.3         3/31/2024	  Lee			封装软件模拟读写外部Flash 以便断电保存数据	
	V1.4		 4/01/2024	  Lee			外部Flash增加读写字符型数据函数 
	V1.5		 4/03/2024	  Lee			增加软件SPI驱动W25Q64	增加手机号11位写入数组函数  功能键改为预处理  
											未输满11位手机号按Exit的话 先把数组清零再返回
	V1.6		 4/04/2024	  Lee			开机对Flash进行扫描 是否有存件数据 扫描到字符'-'表示有数据  并刷新unavailable 同时也是指向下一个Sector的地址 
											存储进Flash的格式为 "手机号(11位)-验证码(5位)"  验证码由TIM2计数器为种子生成 时间种子生成待研究
											矩阵键盘PA7列待考证研究
	V2.0		 4/05/2024	  Lee			增加联系后台中的管理员模式 管理模式支持查看全部存储数据 以及有权利清理数据  管理员密码为1234
											矩阵键盘待考证
											增加存件提示保存验证码
											增加存件后驱动舵机 根据unavailable选择
											增加存取件时语音提示用户几号柜门开关 取件时的目标扇区地址后续的数据待前移
	V3.0		 4/06/2024	  Lee			修复目标取件扇区地址的后续数据转存后最后一个未清除的问题, 
											每次把后续的数据转存到临时数组中 并把该扇区地址清除 以避免后续不正确的扇区指向 
											1-取件码取件	3-存件	4-联系后台(管理员设置)
											以上模块功能实现基本完成98%
											增加存件时输入密码 目的给特定人群使用 避免外部人群对此进行滥用 满足毕设要求之一
    V4.0		 4/09/2024    Lee			完成使用蓝牙串口软件对单片机的控制
											1-查询验证码 	2-使用验证码驱动舵机(驱动后实时对外部Flash数据进行更新)
											
	V6.0		 4/11/2024    Lee										
 ***********************************************************************/

#include "stm32f10x.h"                  // Device header
#include "My_include.h"
#include "my_image.h"




u16 TxData_16[11];
u8 RxData_8_match[11];			// 接收到的手机号进行匹配
u8 RxData_8_jdy[11];			// 缓冲数组 接收到的转为1个字节1个数据
u8 TxData_8_Code[5];			// 发送验证码
u8 RxData_8_Code[5];			// 接收验证码


/* 不可用的数量 */
u16 unavailable = 0;
extern struct TIM_judge_val TIM_judge_val_1;
// u8 Exist_TIM2_limit = 1;
bool lcd_Exist_flag = 0;		/* 判断位 显示Exist余量 每5秒切换 */

/*  key_Num_1 为接收矩阵键盘中的功能按键 */
struct key_Num{
	int8_t key_fun : 5;			/* 功能按键 Exit Reinput Moveup MoveDown Enter */
	u8 key_limit : 2;			/* 限制按键 1-可进入下一级菜单 0-表示已经在下一级菜单 不可操作 进入一级菜单 主菜单按键限制 限制操作1~4 */
	u8 key_lcd_MainMenu : 4;	/* 主菜单按键 操作1~4 */
	int8_t key_temp;			/* 临时变量 用于存储Matrix_Key的返回值 */
}key_Num_1;



//enum key_fun_Num{				/* 功能按键 */
//	key_MoveUp = 10,
//	key_MoveDown,
//	key_Enter,
//	key_Reinput ,
//	key_Close,
//	key_Exit
//};



int main(void){
	SystemInit();				//初始化RCC 设置系统主频为72MHZ
	delay_init(72);	     		//延时初始化
	
	/*********************************************************/	
	LCD_Init();
	lcd_menu_Init(LCD_DISPLAY_Init_loading);			// Init... 界面
	SG90PWM_Init();
	Init_Refresh_unavailable();	// W25初始化 对unavailable刷新 
	Exist_TIM_Init();			/* 显示当前可用数量 定时器初始化 */
	My_JR6001_Init(30);			/* NOTE 放在选项界面后有问题 param:音量 */
	Matrix_ssKey_Pin_Init();
	Serial_JDY31_Init();		// 蓝牙串口初始化
	
	/*********************************************************/	

	
	key_Num_1.key_limit = 1;	// 限制位置一 表示可进入主菜单操作 
	



	delay_ms(1500);
	
	
	lcd_menu_Init(LCD_DISPLAY_WEL_menu);			// 选项界面
	
	USART_SendString(UART1_JR6001,JR6001_PLAY_OPEN_WEL);		// 智能储物柜欢迎您 
	//	USART_SendString(UART1_JR6001,JR6001_PWD_ERROR);		


	/* 上电SG90复位 */
	SG90_Start_Zeroing_Init();
	// SG90_Start_Zeroing_test_180();
	while(1){

		/* 接收键值 */
		key_Num_1.key_temp = Matrix_Key_Scan2();
		

		/* 判断_按键选择1~4 */ 
		if(key_Num_1.key_temp >= 1 && key_Num_1.key_temp <= 4 &&  key_Num_1.key_limit == 1)	key_Num_1.key_lcd_MainMenu = key_Num_1.key_temp;		/* 主界面选择1~4 取件存件联系 */
		else key_Num_1.key_lcd_MainMenu = false;
		
		
//		/* 判断_功能按键 Exit */
//		if(key_Num_1.key_temp == key_Exit) key_Num_1.key_fun = key_Num_1.key_temp;							/* Exit-16 */
//		else key_Num_1.key_fun = false;

		// 时间片轮询

		/* 执行_跳回主菜单  Exit */
//		if(key_Num_1.key_fun == key_Exit && key_Num_1.key_limit == 0)	{
//			lcd_menu_Init(LCD_DISPLAY_WEL_menu);		/* Exit-15 */
//			key_Num_1.key_limit = 1;					/* 限制位置一 可操作MailMenu */
//		}
		
		
		/* 判断_TIM_Exist_flag(每5s翻转一次显示) 是否大于5s 置0 显示Exist余量 */
		if(TIM_judge_val_1.lcd_Exist_flag_Num  > 5)	TIM_judge_val_1.lcd_Exist_flag_Num  = 0;
		
		
		/* 执行_判断是否5s 到了翻转一次显示 显示Exist余量*/
		if(TIM_judge_val_1.lcd_Exist_flag_Num  == 5 && key_Num_1.key_limit == 1){
			lcd_Exist_flag =! lcd_Exist_flag;
			TIM_judge_val_1.lcd_Exist_flag_Num  = 0;
			lcd_menu_Exist(lcd_Exist_flag);
		}		





		// 主菜单菜单选择
		switch(key_Num_1.key_lcd_MainMenu){
			/* PickUp code PickUp*/
			case 1:		
						key_Num_1.key_limit = 0;
						lcd_menu_1st();				// 一级菜单
						key_Num_1.key_limit = 1;

						break;
			/* Scan the QR code to pick up the package */
			case 2:		
				/*
						USART_SendString(UART1_JR6001,JR6001_SCAN_QR);		// 智能储物柜欢迎您 
						LCD_Clear(WHITE);
						LCD_Fill(0,20,128,128,WHITE); //清空显示区域                           
                        LCD_DrawMatrixCode(15,20,100,100,testQr_image,true);*///显示二维码图片 大小90*90  
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
