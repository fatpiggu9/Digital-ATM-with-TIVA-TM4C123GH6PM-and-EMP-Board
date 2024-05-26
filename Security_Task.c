/*****************************************************************************
 * University of Southern Denmark
 * Embedded Programming (EMP)
 *
 * MODULENAME.: Security_Task.c
 *
 * PROJECT....: EMP
 *
 * DESCRIPTION: See module specification file (.h-file).
 *
 * Change Log:
 ******************************************************************************
 * Date    Id    Change
 * YYMMDD
 * --------------------
 * 050128  KA    Module created.
 *
 *****************************************************************************/

/***************************** Include files *******************************/
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "Task.h"
#include "queue.h"
#include "semphr.h"
#include "emp_type.h"
#include "Security_Task.h"
#include "lcd.h"

/*****************************    Defines    *******************************/
#define PF0             0       // Bit 0

#define init_state      0
#define pressed1_state  1
#define pressed2_state  2
#define pressed3_state  3
#define pressed4_state  4
#define correct_state   5

#define false           0
#define true            1


/*****************************   Constants   *******************************/
/*****************************   Variables   *******************************/

extern INT8U locked;
extern QueueHandle_t xQueue_key;
extern SemaphoreHandle_t xSemaphore_key, xSemaphore_unlocked;

/*****************************   Functions   *******************************/

void security_task(void *pvParameters)
{
    /*****************************************************************************
     *   Input    :  -
     *   Output   :  -
     *   Function :  This task ensures that the system is locked until the
     *               correct code is entered
     *****************************************************************************/
    INT8U ch;
    INT8U code, thous, hunds, tens, ones;
    INT8U currState = init_state;

    while (1)
    {
        switch (currState)
        {
        case init_state:
            if (uxQueueMessagesWaiting(xQueue_key))         //Waits until there is an element in the key queue
            {
                if ( xSemaphoreTake(xSemaphore_key, (TickType_t ) 10) == pdTRUE)
                {
                    xQueueReceive(xQueue_key, &ch, portMAX_DELAY);
                    thous = ch;                             //Receive from Queue and put it as the thousands of the number
                    currState = pressed1_state;
                    xSemaphoreGive(xSemaphore_key);
                }
            }
            break;
        case pressed1_state:
            if (uxQueueMessagesWaiting(xQueue_key))         //Waits until there is an element in the key queue
            {
                if ( xSemaphoreTake(xSemaphore_key, (TickType_t ) 10) == pdTRUE)
                {
                    xQueueReceive(xQueue_key, &ch, portMAX_DELAY);
                    hunds = ch;                             //Receive from Queue and put it as the hundreds of the number
                    currState = pressed2_state;
                    xSemaphoreGive(xSemaphore_key);
                }
            }
            break;
        case pressed2_state:
            if (uxQueueMessagesWaiting(xQueue_key))         //Waits until there is an element in the key queue
            {
                if ( xSemaphoreTake(xSemaphore_key, (TickType_t ) 10) == pdTRUE)
                {
                    xQueueReceive(xQueue_key, &ch, portMAX_DELAY);
                    tens = ch;                              //Receive from Queue and put it as the tens of the number
                    currState = pressed3_state;
                    xSemaphoreGive(xSemaphore_key);
                }
            }
            break;
        case pressed3_state:
            if (uxQueueMessagesWaiting(xQueue_key))         //Waits until there is an element in the key queue
            {
                if ( xSemaphoreTake(xSemaphore_key, (TickType_t ) 10) == pdTRUE)
                {
                    xQueueReceive(xQueue_key, &ch, portMAX_DELAY);
                    ones = ch;                              //Receive from Queue and put it as the ones of the number
                    currState = pressed4_state;
                    xSemaphoreGive(xSemaphore_key);
                }
            }
            break;
        case pressed4_state:
            code = thous * 1000 + hunds * 100 + tens * 10 + ones;   //Makes the 4 keypresses into a singular code
            if (code % 8 == 0)                           //Checks if code is divisible by 8
            {
                if ( xSemaphoreTake(xSemaphore_unlocked,
                        (TickType_t ) 10) == pdTRUE)
                {
                    locked = 0;                                //Sets the shared memory to open
                    xSemaphoreGive(xSemaphore_unlocked);
                }
                currState = correct_state;                           //enters correct state if divisible by 8
            }
            else
            {
                currState = init_state;                             //enters reset state if not
            }
            break;
        case correct_state:
            if (uxQueueMessagesWaiting(xQueue_key))                 //Checks if there is things in the queue
            {
                if ( xSemaphoreTake(xSemaphore_key, (TickType_t ) 10) == pdTRUE)
                {
                    xSemaphoreGive(xSemaphore_key);
                    //currState = init_state;
                }
            }
            currState = init_state;
            vTaskDelay(10 / portTICK_RATE_MS); // wait 10 msc
            break;
        }
    }
}
/****************************** End Of Module *******************************/

