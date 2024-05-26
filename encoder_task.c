/*****************************************************************************
 * University of Southern Denmark
 * Embedded Programming (EMP)
 *
 * MODULENAME.: encoder_task.c
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
#include "encoder_task.h"

/*****************************    Defines    *******************************/
#define PF0                 0       // Bit 0

/*****************************   Constants   *******************************/
/*****************************   Variables   *******************************/
extern INT8U banknote;
extern QueueHandle_t xQueue_enco;
extern SemaphoreHandle_t xSemaphore_enco;

/*****************************   Functions   *******************************/
INT8U readAB()
{
    return ((GPIO_PORTA_DATA_R & 0x60) >> 5); //01100000 Read bit 5 and 6 and shift 5 times
}

INT8U enco_pushed()                //Checks if a push has occured on the encoder
{
    return (!(GPIO_PORTA_DATA_R & 0x80));       //returns 0 if no press happened
}

void encoder_task(void *pvParameters)
{
    /*****************************************************************************
     *   Input    :  -
     *   Output   :  -
     *   Function :  This task reads from the encoders and sends a CV or CCV on a
     *               queue depending on the direction turned
     *****************************************************************************/
    INT8U AB, AB_prev = 0, YY;
    INT8U CCV = 0;           //Value send if it is rotated Counter Clockwise
    INT8U CV = 1;          //Value send if it is rotated Clockwise

    while (1)
    {
        AB = readAB();                                              //Read value
        if (AB == AB_prev){} //Checks if The new value and the prev value is the same
        else
        {
            YY = AB ^ AB_prev;                           //xor on AB and AB_prev
            if ((AB & 0x01) == ((AB & 0x02) >> 1))                  //If A = B
            {
                if ((YY & 0x01) == 1 && ((YY & 0x02) >> 1) == 0)    //If YY = 01
                {
                    if ( xSemaphoreTake(xSemaphore_enco,
                            (TickType_t ) 1000) == pdTRUE) //Takes semaphore to the encoder
                    {
                        xQueueOverwrite(xQueue_enco, &CV); //Writes a Clockwise turn on encoder
                        xSemaphoreGive(xSemaphore_enco);
                    }
                }
                else
                {
                    if ((YY & 0x01) == 0 && ((YY & 0x02) >> 1) == 1) //If YY = 10
                    {
                        if ( xSemaphoreTake(xSemaphore_enco,
                                (TickType_t ) 1000) == pdTRUE) //Takes semaphore to the encoder
                        {
                            xQueueOverwrite(xQueue_enco, &CCV); //Writes a Counter Clockwise turn on encoder
                            xSemaphoreGive(xSemaphore_enco);
                        }
                    }
                }
            }
        }
        AB_prev = AB;           //Saves previous value
    }
}

/****************************** End Of Module *******************************/
