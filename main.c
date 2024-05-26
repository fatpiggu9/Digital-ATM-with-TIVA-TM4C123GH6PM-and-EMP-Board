/**
 * main.c
 */
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "emp_type.h"
#include "gpio.h"
#include "systick_frt.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "status_led.h"
#include "lcd.h"
#include "key.h"
#include "UI_task.h"
#include "Security_task.h"
#include "encoder_task.h"

#define USERTASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define IDLE_PRIO 0
#define LOW_PRIO 1
#define MED_PRIO 2
#define HIGH_PRIO 3
#define QUEUE_LEN 128

QueueHandle_t xQueue_lcd, xQueue_key, xQueue_enco;
SemaphoreHandle_t xSemaphore_lcd, xSemaphore_key, xSemaphore_unlocked, xSemaphore_enco;

INT8U banknote = 1;
INT8U locked = 1;

static void setupHardware(void){
    init_systick();
    status_led_init();
    init_gpio();

    xQueue_lcd = xQueueCreate  ( QUEUE_LEN , sizeof( INT8U ));
    xQueue_key = xQueueCreate  ( QUEUE_LEN , sizeof( INT8U ));
    xQueue_enco = xQueueCreate ( QUEUE_LEN , sizeof( INT8U ));
    xSemaphore_lcd = xSemaphoreCreateMutex();
    xSemaphore_key = xSemaphoreCreateMutex();
    xSemaphore_unlocked = xSemaphoreCreateMutex();
    xSemaphore_enco = xSemaphoreCreateMutex();
}

int main(void)
{
	setupHardware();
	xTaskCreate( status_led_task, "status_led", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
	xTaskCreate( key_task, "key_task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
	xTaskCreate( lcd_task, "LCD_task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
	xTaskCreate( UI_task, "UI_task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
	xTaskCreate( security_task, "security_task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
	xTaskCreate( encoder_task, "encoder_task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
	vTaskStartScheduler();
	return 0;
}
