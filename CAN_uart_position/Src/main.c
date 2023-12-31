/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include "stdio.h"
#include "string.h"
#include "pid.h"
#include "bsp_can.h"
#include "CAN_receive.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
int16_t myspeed_rpm1;
int16_t mygiven_current1;
int16_t myspeed_rpm2;
int16_t mygiven_current2;

fp32 motor1_speed,motor2_speed;
	
pid_type_def motor_pid_data;

uint8_t input[10];

float SET_SPEED = 0.08f;

int16_t cur_pos,set_pos,err_pos;
  
void usart_printf(const char *fmt,...)
{
    static uint8_t tx_buf[256] = {0};
    static va_list ap;
    static uint16_t len;
    va_start(ap, fmt);
    len = vsprintf((char *)tx_buf, fmt, ap);
    va_end(ap);
    //usart1_tx_dma_enable(tx_buf, len);
    HAL_UART_Transmit_DMA(&huart1,tx_buf, len);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart1) {
        // 解析输入数据
        uint8_t command = input[0] - '0';
        uint16_t value = (input[1] - '0') * 1000 + (input[2] - '0') * 100 + (input[3] - '0') * 10 + (input[4] - '0');

        // 根据输入数据设置参数
        switch (command) {
            case 0:
				set_pos = (uint16_t)value;
			break;
			
            case 1:
				//usart_printf("%d\n",value);
				motor_pid_data.Kp = ((float)value)/10;
			break;
			
            case 2:
				//usart_printf("%d\n",value);
				motor_pid_data.Ki = ((float)value)/10;
			break;
			
            case 3:
				//usart_printf("%d\n",value);
				motor_pid_data.Kd = ((float)value)/10;
			break;
			
            default:
                break;
        }
		usart_printf("%d",command);
		usart_printf("%d\n",value);
        // 开始下一次数据接收
        HAL_UART_Receive_DMA(&huart1, input, 5);
    }
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	int32_t total = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  can_filter_init();
  #define KP 10
  #define KI 50
  #define KD 200
  #define MAX_OUT 4000.0f
  #define MAX_IOUT 100
	
  #define CHASSIS_MOTOR_RPM_TO_VECTOR_SEN 0.000415809748903494517209f

  
  fp32 motor1_speed,motor2_speed;
  
  
  fp32 pid[3]= {KP,KI,KD};
  PID_init(&motor_pid_data,PID_POSITION,pid,MAX_OUT,MAX_IOUT); 
  
  HAL_UART_Receive_DMA(&huart1, input, 5);
  usart_printf("start");
  
  set_pos = motor_chassis[1].ecd;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		cur_pos = motor_chassis[1].ecd;
	  
	    err_pos = motor_chassis[1].ecd - motor_chassis[1].last_ecd;
	    
	    //当前位置远大于上次位置,从8000->100
		if(err_pos > 4096){
			err_pos = motor_chassis[1].last_ecd + (8192 - motor_chassis[1].ecd);
		}
		if(err_pos < -4096){
			err_pos = motor_chassis[1].ecd + (8192 - motor_chassis[1].last_ecd);
		}
		
	    total += err_pos;
		
	    //motor1_speed = motor_chassis[0].speed_rpm*CHASSIS_MOTOR_RPM_TO_VECTOR_SEN;
	    //motor2_speed = motor_chassis[1].speed_rpm*CHASSIS_MOTOR_RPM_TO_VECTOR_SEN;
	  
	    //mygiven_current1 = PID_calc(&motor_pid_data,cur_pos,0);
		
		mygiven_current2 = PID_calc(&motor_pid_data,total - set_pos,0);
	  
		if(err_pos > 4096){
			cur_pos = 8192 + motor_chassis[1].ecd;
		}
		if(err_pos < -4096){
			cur_pos = -(8192 - motor_chassis[1].ecd);
		}
		usart_printf("%d,%d,%d,%d,%d,%d,%d\n",
										(int)(motor2_speed*1000),
										(int)(cur_pos),
										(int)(total),
										(int)(mygiven_current2),
										(int)(motor_pid_data.Kp),
										(int)(motor_pid_data.Ki),
										(int)(motor_pid_data.Kd));
	  
        CAN_cmd_chassis(0, mygiven_current2, 0, 0);
        //CAN_cmd_chassis(0, 0, 0, 0);
	  
        HAL_Delay(2);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
