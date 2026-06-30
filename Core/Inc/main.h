/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PHASE_U_VOLT_Pin GPIO_PIN_6
#define PHASE_U_VOLT_GPIO_Port GPIOF
#define PHASE_V_VOLT_Pin GPIO_PIN_7
#define PHASE_V_VOLT_GPIO_Port GPIOF
#define PHASE_W_VOLT_Pin GPIO_PIN_8
#define PHASE_W_VOLT_GPIO_Port GPIOF
#define VBUS_Pin GPIO_PIN_9
#define VBUS_GPIO_Port GPIOF
#define ADC_IU_Pin GPIO_PIN_3
#define ADC_IU_GPIO_Port GPIOA
#define ADC_IV_Pin GPIO_PIN_4
#define ADC_IV_GPIO_Port GPIOA
#define DRIVER_SD_Pin GPIO_PIN_5
#define DRIVER_SD_GPIO_Port GPIOA
#define ADC_IW_Pin GPIO_PIN_6
#define ADC_IW_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
