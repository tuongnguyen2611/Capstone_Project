/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "CANSPI.h"
#include "MCP2515.h"
#include "string.h"
#include <math.h>
#include "stdio.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
/* USER CODE BEGIN PV */
//Khai bao bien
uint8_t rx_buffer[5];
float received_value;
float Kp = 0.0f, Ki = 0.0f, alpha = 0.0f;
float old_Kp = 0.0f, old_Ki = 0.0f, old_alpha = 0.0f;
float setpoint = 0.0f;
float tps1, tps2, TPSmin, TPSmax;
volatile bool Calib_flag = false;
uCAN_MSG txMessage;	uCAN_MSG rxMessage;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//Ham ngat nhan UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{ 
    if (huart->Instance == USART1)
    {       
			uint8_t id = rx_buffer[0];
        
        memcpy(&received_value, &rx_buffer[1], sizeof(float));  // Chuyen 4 byte  -> float

        switch (id)
        {
            case 0x01:  // ID cho he so Kp
                Kp = received_value;
                break;

            case 0x02:  // ID cho he so Ki
                Ki = received_value;
                break;

						case 0x03:  // ID cho setpoint
                setpoint = received_value;
                break;	
						
						case 0x04:  // ID cho alpha + yeu cau gui gia tri Calib
                alpha = received_value;							
                Calib_flag = true;
                break;	
						
            default:
                // Bo qua neu khong xac dinh duoc ID
                break;
        }

      HAL_UART_Receive_IT(&huart1, rx_buffer, 5);//tai kich hoat ham ngat nhan UART
    }
}
//Ham gui UART - tps
void UART_SendTPSData(float tps1, float tps2 ) {
    uint8_t buffer[8];  // 4 byte cho moi bien
    int i = 0;
	
    // Dua tps1 vao buffer (4 byte)
    memcpy(&buffer[i], &tps1, sizeof(float));
    i += sizeof(float);

    // Ðua tps2 vào buffer (4 byte)
    memcpy(&buffer[i], &tps2, sizeof(float));
    i += sizeof(float);

    // Gui toàn bo buffer qua UART
    HAL_UART_Transmit(&huart1, buffer, sizeof(buffer), 100);
}
//Ham gui UART - gia tri calib
void UART_SendCalibData(float TPSmin, float TPSmax) {
    uint8_t buffer[8];  // 4 byte cho moi bien
    int i = 0;

    // Dua tps1 vao buffer (4 byte)
    memcpy(&buffer[i], &TPSmin, sizeof(float));
    i += sizeof(float);

    // Dua tps2 vao buffer (4 byte)
    memcpy(&buffer[i], &TPSmax, sizeof(float));
    i += sizeof(float);

    // Gui toan bo buffer qua UART
    HAL_UART_Transmit(&huart1, buffer, sizeof(buffer), 100);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)

{

  /* USER CODE BEGIN 1 */

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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, rx_buffer, 5);
	MCP2515_Reset();
	CANSPI_Initialize(); 
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		if (Kp!=old_Kp||Ki!=old_Ki)//check gia tri moi
		{
				//CAN FRAME 1 gui Kp va Ki
				txMessage.frame.id = 0x111;
				txMessage.frame.dlc = 8;
				memcpy(&txMessage.frame.data0, &Kp, sizeof(float));
				memcpy(&txMessage.frame.data4, &Ki, sizeof(float));			
				CANSPI_Transmit(&txMessage);
				old_Kp = Kp; //cap nhat gia tri moi
				old_Ki = Ki;
				HAL_Delay(5);				
		}
		
		if (alpha!=old_alpha)//check gia tri moi
		{
				//CAN FRAME 2 gui alpha
				txMessage.frame.id = 0x116;
				txMessage.frame.dlc = 4;
				memcpy(&txMessage.frame.data0, &alpha, sizeof(float));
				CANSPI_Transmit(&txMessage);
				old_alpha = alpha; //cap nhat gia tri moi
				HAL_Delay(5);				
		}	
		
		//CAN FRAME 3 gui setpoint (lien tuc)
		txMessage.frame.id = 0x112;
    txMessage.frame.dlc = 4;
		memcpy(&txMessage.frame.data0, &setpoint, sizeof(float));
		CANSPI_Transmit(&txMessage);
    HAL_Delay(5);
		
		//Check yeu cau gui gia tri calib ve matlab
		if (Calib_flag == true)
		{
				UART_SendCalibData(TPSmin,TPSmax);
				Calib_flag = false;
				HAL_Delay(5);			
		}
		
		//Nhan CAN 
		if(CANSPI_Receive(&rxMessage))
    {
				//CAN FRAME 4 nhan gia tri calib (tu node 2)
				if (rxMessage.frame.id == 0x110 && rxMessage.frame.dlc == 8) 
				{		
          memcpy(&TPSmin, &rxMessage.frame.data0, sizeof(float));
          memcpy(&TPSmax, &rxMessage.frame.data4, sizeof(float));
				}				
				//CAN FARME 5 nhan TPS1Raw va TPS2Raw (tu node 3)
        if (Calib_flag == false && rxMessage.frame.id == 0x115 && rxMessage.frame.dlc == 8) 
				{		
          memcpy(&tps1, &rxMessage.frame.data0, sizeof(float));
          memcpy(&tps2, &rxMessage.frame.data4, sizeof(float));
					UART_SendTPSData(tps1,tps2);//Gui ve matlab sau khi nhan
				}				
		    HAL_Delay(10);
		}
	}					
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CAN_CS_GPIO_Port, CAN_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CAN_CS_Pin */
  GPIO_InitStruct.Pin = CAN_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAN_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
