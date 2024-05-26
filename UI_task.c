/*****************************************************************************
 * University of Southern Denmark
 * Embedded Programming (EMP)
 *
 * MODULENAME.: UI_task.c
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
#include "UI_task.h"
#include "lcd.h"
#include "Security_task.h"
#include "encoder_task.h"

/*****************************    Defines    *******************************/
#define PF0                 0       // Bit 0

#define init_state          0
#define start_state         1
#define withdraw_state      2
#define locked_state        3
#define unlocked_state      4
#define banknote_state      5
#define reset_state         6
#define amount_state        7
#define DEBOUNCE_DELAY_MS   50
#define DOUBLE_CLICK_WINDOW_MS 500

/*****************************   Constants   *******************************/
/*****************************   Variables   *******************************/
INT8U UIstate = init_state;
extern INT8U banknote;

extern INT8U locked;
extern QueueHandle_t xQueue_key, xQueue_lcd, xQueue_enco;
extern SemaphoreHandle_t xSemaphore_key, xSemaphore_lcd, xSemaphore_unlocked, xSemaphore_enco;

/*****************************   Functions   *******************************/
void write_to_clear_LCD(INT8U write[], INT8U size)
{
    /*****************************************************************************
     *   Input    :  Array and size of the array
     *   Output   :  -
     *   Function :  Clears LCD and displays the content of the array
     *****************************************************************************/
    if ( xSemaphoreTake(xSemaphore_lcd, (TickType_t ) 10) == pdTRUE)
    {
        INT8U i = 0;
        INT8U clear = 0xff;
        xQueueSend(xQueue_lcd, &clear, 0);
        for (i = 0; i < size - 1; i++)
        {
            xQueueSend(xQueue_lcd, &write[i], 0);
        }
        xSemaphoreGive(xSemaphore_lcd);
    }
}

void write_to_LCD(INT8U write[], INT8U size)
{
    /*****************************************************************************
     *   Input    :  Array and size of the array
     *   Output   :  -
     *   Function :  Display the content of the array to LCD
     *****************************************************************************/
    if ( xSemaphoreTake(xSemaphore_lcd, (TickType_t ) 10) == pdTRUE)
    {
        INT8U i = 0;
        for (i = 0; i < size - 1; i++)
        {
            xQueueSend(xQueue_lcd, &write[i], 0);
        }
        xSemaphoreGive(xSemaphore_lcd);
    }
}

void move(INT8U x, INT8U y)
{
    /*****************************************************************************
     *   Input    :  x and y coordinates on display
     *   Output   :  -
     *   Function :  Moves LCD cursor
     *****************************************************************************/
    if ( xSemaphoreTake(xSemaphore_lcd, (TickType_t ) 10) == pdTRUE)
    {
        move_LCD(x, y);
        xSemaphoreGive(xSemaphore_lcd);
    }
}

INT8U button1_pushed()              //Checks if SW1 is pressed
{
    return (!(GPIO_PORTF_DATA_R & 0x10));  // SW1 at PF4
}

INT8U button2_pushed()              // hard coded SW2 double click validation
{
    static TickType_t last_button_press = 0;    // Takes the time of last button press
    static TickType_t last_click_time = 0;      // Takes the time of last click
    TickType_t current_time = xTaskGetTickCount();  // Gets current time in ticks
    if ((current_time - last_button_press ) >= DEBOUNCE_DELAY_MS){  // Validates if debounce delay has passed since last button press
        if (!(GPIO_PORTF_DATA_R & 0x01)){           // Checks if SW2 is pressed
            if ((current_time - last_click_time) <= DOUBLE_CLICK_WINDOW_MS){  // Check if the second click is within the double click window time
                last_click_time = current_time;      // Updates last click time
                return 2;                            // Double click indicator
            }
            last_click_time = current_time;          // Updates last click time
            last_button_press = current_time;        // Updates last button press
            return 1;                                // Single click indicator
        }
    }
    return 0;
}

void UI_task(void *pvParameters)
{
    /*****************************************************************************
     *   Input    :  -
     *   Output   :  -
     *   Function :  Task displays and handles all interaction with the user
     *****************************************************************************/
    INT8U ch;
    while (1)
    {
        switch (UIstate)
        {
        case init_state:
            if(1)
            {
                INT8U write_LCD1[] = "Welcome to ATM!";
                write_to_clear_LCD(write_LCD1, sizeof(write_LCD1)); // Writes to clear LCD
                move(0,1);
                INT8U write_LCD2[] = "Press switch1";
                write_to_LCD(write_LCD2, sizeof(write_LCD2)); // Writes to clear LCD
                UIstate = start_state;
            }
            break;

        case start_state:
            if(button1_pushed())
            {
                INT8U write_LCD1[] = "Choose: 1. 1000 ";
                write_to_clear_LCD(write_LCD1, sizeof(write_LCD1)); // Writes to clear LCD
                move(0,1);
                INT8U write_LCD2[] = "2. 5000 3.9999";
                write_to_LCD(write_LCD2, sizeof(write_LCD2)); // Writes to clear LCD
                UIstate = withdraw_state;
            }
            break;

        case withdraw_state:
            if(1)
            {
                if (uxQueueMessagesWaiting(xQueue_key))         //Waits until there is an element in the key queue
                {
                    if ( xSemaphoreTake(xSemaphore_key, (TickType_t ) 10) == pdTRUE)
                    {
                        xQueueReceive(xQueue_key, &ch, portMAX_DELAY);
                        xSemaphoreGive(xSemaphore_key);
                    }

                    if(ch == '1')                             // input "1" keypad detected
                    {
                        INT8U write_LCD[] = "Selected: 1000";
                        write_to_clear_LCD(write_LCD, sizeof(write_LCD));
                        vTaskDelay(1000 / portTICK_RATE_MS);   // wait 1000 msec
                        UIstate = locked_state;
                    }

                    if(ch == '2')                             // input "2" keypad detected
                    {
                        INT8U write_LCD[] = "Selected: 5000";
                        write_to_clear_LCD(write_LCD, sizeof(write_LCD));
                        vTaskDelay(1000 / portTICK_RATE_MS);   // wait 1000 msec
                        UIstate = locked_state;
                    }

                    if(ch == '3')                             // input "3" keypad detected
                    {
                        INT8U write_LCD[] = "Selected: 9999";
                        write_to_clear_LCD(write_LCD, sizeof(write_LCD));
                        vTaskDelay(1000 / portTICK_RATE_MS);   // wait 1000 msec
                        UIstate = locked_state;
                    }
                }
            }
            break;

        case locked_state:
            if(1)
            {
                INT8U write_LCD1[] = "Insert Password";
                write_to_clear_LCD(write_LCD1, sizeof(write_LCD1)); // Writes to clear LCD
                UIstate = unlocked_state;
            }

            break;

        case unlocked_state:
            if (xSemaphoreTake(xSemaphore_unlocked, (TickType_t ) 10) == pdTRUE)
            {
                if(locked == 0)
                {
                    INT8U write_LCD[] = "Correct Password!";
                    locked = 1;
                    write_to_clear_LCD(write_LCD, sizeof(write_LCD)); // Writes to clear LCD
                    vTaskDelay(1000 / portTICK_RATE_MS);   // wait 1000 msec
                    UIstate = amount_state;
                }
                xSemaphoreGive(xSemaphore_unlocked);
            }

            break;

        case amount_state:
            if(1)
            {
                INT8U state = 1;                         // flag and switch between states of withdrawal amount
                INT8U write_LCD1[] = "Select amount:";
                write_to_clear_LCD(write_LCD1, sizeof(write_LCD1));
                move(0,1);
                INT8U write_LCD2[] = "500 kroner";
                write_to_LCD(write_LCD2, sizeof(write_LCD2));
                vTaskDelay(1000 / portTICK_RATE_MS);
                while(1){
                    if(button2_pushed() && state == 1){
                        state = 2;
                        move(0,1);
                        INT8U write_LCD1[] = "200 kroner";
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                    }

                    else if(button1_pushed() && state == 2){
                        state = 1;
                        move(0,1);
                        INT8U write_LCD1[] = "500 kroner";
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                    }
                    else if(button2_pushed() && state == 2){
                        state = 3;
                        move(0,1);
                        INT8U write_LCD1[] = "100 kroner";
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                    }
                    else if(button1_pushed() && state == 3){
                        state = 2;
                        move(0,1);
                        INT8U write_LCD1[] = "200 kroner";
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                    }
                    else if(button2_pushed() && state == 3){
                        state = 4;
                        move(0,1);
                        INT8U write_LCD1[] = "50 kroner ";
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                    }
                    else if(button1_pushed() && state == 4){
                        state = 3;
                        move(0,1);
                        INT8U write_LCD1[] = "100 kroner";
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                    }
                    else if(button2_pushed() && state == 4){}
                    else if(button2_pushed() == 2) {
                        move(0,1);
                        INT8U write_LCD[] = "Success!";
                        write_to_clear_LCD(write_LCD, sizeof(write_LCD));
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                        INT8U write_LCD1[] = "Select cash";
                        write_to_clear_LCD(write_LCD1, sizeof(write_LCD1)); // Writes to clear LCD
                        move(0, 1);                                 // Moves LCD
                        INT8U write_LCD2[] = "100 kroner";
                        write_to_LCD(write_LCD2, sizeof(write_LCD2));
                        banknote = 1;
                        UIstate = banknote_state;
                        break; // Break out of the loop
                    }
                }
            }
            break;

        case banknote_state:
            if(1)
            {
                INT8U blinks = 0;
                if (enco_pushed())
                {
                    INT8U write_LCD[] = "Processing";
                    write_to_clear_LCD(write_LCD, sizeof(write_LCD)); // Writes to clear LCD
                    move(0, 1);                                     // Moves LCD
                    if (banknote == 2)
                    {
                        while(blinks != 5){
                        INT8U write_LCD1[] = "10 kroner...";
                        GPIO_PORTF_DATA_R ^= 0x08;                     // Displays GREEN LCD
                        vTaskDelay(500 / portTICK_RATE_MS);
                        blinks++;
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                        }
                    }
                    else if (banknote == 1)
                    {
                        INT8U write_LCD1[] = "100 kroner...";
                        GPIO_PORTF_DATA_R ^= 0x02;                     // Displays RED LCD
                        vTaskDelay(1000 / 1000 / portTICK_RATE_MS);
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                    }
                    else if (banknote == 3)
                    {
                        INT8U write_LCD1[] = "50 kroner...";
                        GPIO_PORTF_DATA_R ^= 0x0a;                     // Displays YELLOW LCD
                        vTaskDelay(1000 / 500 / portTICK_RATE_MS);
                        write_to_LCD(write_LCD1, sizeof(write_LCD1));
                    }
                    vTaskDelay(2000 / portTICK_RATE_MS);
                    GPIO_PORTF_DATA_R = 0x00;
                    UIstate = withdraw_state;
                    INT8U write_LCD1[] = "Choose: 1. 1000 ";
                    write_to_clear_LCD(write_LCD1, sizeof(write_LCD1)); // Writes to clear LCD
                    move(0,1);
                    INT8U write_LCD2[] = "2. 5000 3.9999";
                    write_to_LCD(write_LCD2, sizeof(write_LCD2)); // Writes to clear LCD
                }
                if (uxQueueMessagesWaiting(xQueue_enco))
                {
                    if ( xSemaphoreTake(xSemaphore_enco, (TickType_t) 10) == pdTRUE)
                    {
                        xQueueReceive(xQueue_enco, &ch, portMAX_DELAY);
                        if (ch == 1){ // CC
                            if (banknote == 1)
                            {
                                banknote = 2;
                                move(0,1);
                                INT8U write_LCD1[] = "10 kroner ";
                                write_to_LCD(write_LCD1, sizeof(write_LCD1));
                            }
                            else if (banknote == 2)
                            {
                                banknote = 3;
                                move(0,1);
                                INT8U write_LCD1[] = "50 kroner ";
                                write_to_LCD(write_LCD1, sizeof(write_LCD1));
                            }
                            else if (banknote == 3)
                            {
                                banknote = 1;
                                move(0,1);
                                INT8U write_LCD1[] = "100 kroner";
                                write_to_LCD(write_LCD1, sizeof(write_LCD1));
                            }
                            xSemaphoreGive(xSemaphore_enco);
                        }

                        else{ // C
                            if (banknote == 1)
                            {
                                banknote = 3;
                                move(0,1);
                                INT8U write_LCD1[] = "50 kroner ";
                                write_to_LCD(write_LCD1, sizeof(write_LCD1));
                            }
                            else if (banknote == 2)
                            {
                                banknote = 1;
                                move(0,1);
                                INT8U write_LCD1[] = "100 kroner";
                                write_to_LCD(write_LCD1, sizeof(write_LCD1));
                            }
                            else if (banknote == 3)
                            {
                                banknote = 2;
                                move(0,1);
                                INT8U write_LCD1[] = "10 kroner ";
                                write_to_LCD(write_LCD1, sizeof(write_LCD1));
                            }
                            xSemaphoreGive(xSemaphore_enco);
                        }
                        xSemaphoreGive(xSemaphore_enco);
                        vTaskDelay(1000 / portTICK_RATE_MS); // wait 1000 msec
                    }
                }
            }
            break;
        }
    }
}

/****************************** End Of Module *******************************/
