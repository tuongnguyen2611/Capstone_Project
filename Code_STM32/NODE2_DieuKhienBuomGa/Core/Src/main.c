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
#include <string.h>
#include "MCP2515.h"
#include <math.h>
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
ADC_HandleTypeDef hadc1;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
//Cac bien danh gia hieu qua bo dieu khien buom ga
float static_error = 0;
float min_error = 0, overshoot = 0;
float time = 0, rise_time = 0, settling_time = 0;
uint8_t rise_time_flag = 0, settling_time_flag = 0;

//Bien doc cam bien
float tps1Raw, tps2Raw, desiredRaw;

//Thoi gian lay mau
const float sampling_time = 0.01;

//Cac bien calib cam bien TPS
uint16_t TPS1min, TPS2min, TPS1max, TPS2max ;
float TPSmin = 0.0f, TPSmax = 0.0f, old_TPSmin = 0.0f, old_TPSmax = 0.0f;

//Thong so bo dieu khien PID
float Kp = 180, Ki = 200, Kd = 0;
float setpoint, actual, error, integral, Integral, Derivative, output, old_error;
uCAN_MSG rxMessage; uCAN_MSG txMessage;

//Cac gia tri bo loc EMA
float tps1, tps2, desired, alpha = 0.7;

//Cac ham
void computePI(float setpoint, float actual);
uint16_t get_adc_channel(uint32_t channel);
void Setup_Hbridge_TPS(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void Send_Calib_Data(void);
void Evaluate_PID_controller(float setpoint, float error);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t get_adc_channel(uint32_t channel){//Ham doc cam bien
	uint16_t adc;
	//config channel
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK){
		Error_Handler();
	}
	//read adc
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 5);
	adc = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	return adc;
}
void Setup_Hbridge_TPS(void){//Ham calib TPS
	//Xac dinh chieu quay buom ga
	HAL_GPIO_WritePin(GPIOB,IN1_Pin,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB,IN2_Pin,GPIO_PIN_RESET);
	//Calib cam bien TPS
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
	htim2.Instance->CCR1 = 0;//tra buom ga ve 0%
  HAL_Delay(1000);//doi buom ga ve 0%
  TPS1min = get_adc_channel(ADC_CHANNEL_1);//ghi gia tri TPS1 tai do mo min
  TPS2min = get_adc_channel(ADC_CHANNEL_2);//ghi gia tri TPS2 tai do mo min
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
	htim2.Instance->CCR1 = 2200;//dua buom ga len 100%
	HAL_Delay(2000);//doi buom ga len 100%
  TPS1max = get_adc_channel(ADC_CHANNEL_1);//ghi gia tri TPS1 tai do mo max
  TPS2max = get_adc_channel(ADC_CHANNEL_2);//ghi gia tri TPS2 tai do mo max
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
	htim2.Instance->CCR1 = 0;//tra buom ga ve 0%
	HAL_Delay(1000);//doi buom ga ve 0%
  TPSmin = TPS1min - TPS2min;
  TPSmax = TPS1max - TPS2max;
}
void Send_Calib_Data(void){//Ham gui gia tri calib len CAN
		//CAN FRAME GUI TPSmin VA TPSmax (1 lan)
		txMessage.frame.id = 0x110;
		txMessage.frame.dlc = 8;
    memcpy(&txMessage.frame.data0, &TPSmin, sizeof(float));
    memcpy(&txMessage.frame.data4, &TPSmax, sizeof(float));	
		CANSPI_Transmit(&txMessage);
}
void Evaluate_PID_controller(float setpoint, float error){//Ham phu danh gia thong so hoat dong bo dieu khien PID
	static_error = fabsf(error)*0.05 + (1-0.05)*static_error;//Tinh sai so tinh
	if (error < min_error){min_error = error;}//Xac dinh sai so o dinh vot lo (overshooting-peak error)
	overshoot = fabsf(min_error)/setpoint;//Tinh do vot lo
  if (setpoint == 0){min_error = time = rise_time = settling_time = overshoot = 0; rise_time_flag = settling_time_flag = 0;}//reset cac thong so danh gia khi buom ga ve 0
	if (setpoint!=0){time += 0.01;}//Dem thoi gian moi chu ky lay mau
	if (old_error*error<0 && rise_time_flag == 0 &&actual>5){rise_time = time; rise_time_flag = 1;}//Xac dinh buom ga da dat setpoint khi sai so doi dau, bat co de xac dinh da co gia tri
	if (static_error<=1 && rise_time_flag == 1 && settling_time_flag == 0){settling_time = time; settling_time_flag = 1;}//Xac dinh buom ga da dat trang thai tinh, bat co de xac dinh da co gia tri
}
void computePI(float setpoint, float actual){//Ham tinh toan bo dieu khien PI
  error = setpoint - actual;
	Evaluate_PID_controller(setpoint, error);//danh gia bo dieu khien
  integral += error*sampling_time;//tinh thanh phan tich phan
  if (setpoint == 0) integral = 0;	//tranh hien tuong bao hoa tich phan khi calib khong chuan (khi do mo = 0 nhung actual khac 0), khien tich phan sai so tang cao
  Integral = Ki * integral;
  if (Integral<0){Integral = 0;}
	if (Integral>1450){Integral = 1450;} //tranh hien tuong bao hoa tich phan
	Derivative = Kd*(error - old_error)/sampling_time;//tinh thanh phan dao ham
  output = Kp * error + Integral + Derivative;//output cua thuat toan PID
  if (output<0){output = 0;}
	if (output>3599){output = 3599;}//tranh output vuot qua PWM cho phep (tu 0 den 3599)
  if (setpoint == 0) output = 0;	//tat dien ap khi buom ga o trang thai nghi
  HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
	htim2.Instance->CCR1 = output;//xuat PWM
	old_error = error;//cap nhat bien old_error
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){//ham ngat timer 1 10ms
	if (htim->Instance == htim1.Instance){
    //Loc EMA
    tps1 = tps1Raw*alpha + (1-alpha)*tps1;
    tps2 = tps2Raw*alpha + (1-alpha)*tps2;
		//Tinh do mo thuc te
    actual = ((tps1 - tps2)-TPSmin)*100/(TPSmax - TPSmin);
		if (actual<0){actual = 0;}
		if (actual>100){actual = 100;}
		//Tinh toan PID
    computePI(setpoint, actual);
		HAL_TIM_Base_Start_IT(&htim1);//tai kich hoat ngat timer
	}
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
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1);
	MCP2515_Reset();
  CANSPI_Initialize();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	//Xac dinh chieu quay motor và calib TPS
	Setup_Hbridge_TPS();//Xac dinh chieu quay motor và calib TPS
  Send_Calib_Data();//Gui gia tri calib len CAN
	HAL_TIM_Base_Start_IT(&htim1);//Bat ngat timer1 (10ms)
  while (1)
  {
	//Nhan CAN
  if (CANSPI_Receive(&rxMessage))
  {
    switch (rxMessage.frame.id)
    {
      case 0x111: // Nhan Kp, Ki
        if (rxMessage.frame.dlc == 8)
        {
          memcpy(&Kp, &rxMessage.frame.data0, sizeof(float));
          memcpy(&Ki, &rxMessage.frame.data4, sizeof(float));
        }
        break;

      case 0x112: // Nhan setpoint
        if (rxMessage.frame.dlc == 4)
        {
          memcpy(&setpoint, &rxMessage.frame.data0, sizeof(float));
					  if (setpoint<0){setpoint = 0;}
						if (setpoint>100){setpoint = 100;}
        }
        break;

      case 0x115: // Nhan TPS1, TPS2
        if (rxMessage.frame.dlc == 8)
        {
          memcpy(&tps1Raw, &rxMessage.frame.data0, sizeof(float));
          memcpy(&tps2Raw, &rxMessage.frame.data4, sizeof(float));
        }
        break;

      case 0x116: // Nhan alpha
        if (rxMessage.frame.dlc == 4)
        {
          memcpy(&alpha, &rxMessage.frame.data0, sizeof(float));
        }
        break;
				
      default:
        // Không xu ly frame l
        break;
    
    }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 7199;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 99;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3599;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CAN_CS_GPIO_Port, CAN_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, IN1_Pin|IN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CAN_CS_Pin */
  GPIO_InitStruct.Pin = CAN_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAN_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IN1_Pin IN2_Pin */
  GPIO_InitStruct.Pin = IN1_Pin|IN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
