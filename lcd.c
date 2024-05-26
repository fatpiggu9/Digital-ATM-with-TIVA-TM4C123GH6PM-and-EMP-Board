/*****************************************************************************
* University of Southern Denmark
* Embedded Programming (EMP)
*
* MODULENAME.: lcd.c
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
#include "emp_type.h"
#include "lcd.h"
#include "glob_def.h"
#include "tmodel.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"


/*****************************    Defines    *******************************/

#define QUEUE_LEN   128
#define scale_val   0xFE
#define offset_pos  0xFD
#define offset_val  0xFC
#define number_pos  0xFB
#define number_val  0xFA


extern QueueHandle_t xQueue_lcd;
extern SemaphoreHandle_t xSemaphore_lcd;

enum LCD_states
{
  LCD_POWER_UP,
  LCD_INIT,
  LCD_READY,
  LCD_ESC_RECEIVED,
};

/*****************************   Constants   *******************************/
const INT8U LCD_init_sequense[]= 
{
  0x30,		// Reset
  0x30,		// Reset
  0x30,		// Reset
  0x20,		// Set 4bit interface
  0x28,		// 2 lines Display
  0x0C,		// Display ON, Cursor OFF, Blink OFF
  0x06,		// Cursor Increment
  0x01,		// Clear Display
  0x02,     // Home
  0xFF		// stop
}; 


/*****************************   Variables   *******************************/
enum LCD_states LCD_state = LCD_POWER_UP;
INT8U LCD_init;

/*****************************   Functions   *******************************/
INT8U wr_ch_LCD( INT8U Ch )
/*****************************************************************************
*   OBSERVE  : LCD_PROC NEEDS 20 mS TO PRINT OUT ONE CHARACTER 
*   Function : See module specification (.h-file).
*****************************************************************************/
{
        return( xQueueSend( xQueue_lcd, &Ch, portMAX_DELAY));
}

void wr_str_LCD( INT8U *pStr )
/*****************************************************************************
*   Function : See module specification (.h-file).
*****************************************************************************/
{
  while( *pStr )
  {
    wr_ch_LCD( *pStr );
	pStr++;
  }
}

void move_LCD( INT8U x, INT8U y )
/*****************************************************************************
*   Function : See module specification (.h-file).
*****************************************************************************/
{
  INT8U Pos;

  Pos = y*0x40 + x;
  Pos |= 0x80;
  wr_ch_LCD( ESC );
  wr_ch_LCD( Pos );
}

void wr_ctrl_LCD_low( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write low part of control data to LCD.
******************************************************************************/
{
  INT8U temp;
  volatile int i;

  temp = GPIO_PORTC_DATA_R & 0x0F;
  temp  = temp | ((Ch & 0x0F) << 4);
  GPIO_PORTC_DATA_R  = temp;
  for( i=0; i<1000; i )
	  i++;
  GPIO_PORTD_DATA_R &= 0xFB;        // Select Control mode, write
  for( i=0; i<1000; i )
	  i++;
  GPIO_PORTD_DATA_R |= 0x08;		// Set E High

  for( i=0; i<1000; i )
	  i++;

  GPIO_PORTD_DATA_R &= 0xF7;		// Set E Low

  for( i=0; i<1000; i )
	  i++;
}

void wr_ctrl_LCD_high( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write high part of control data to LCD.
******************************************************************************/
{
  wr_ctrl_LCD_low(( Ch & 0xF0 ) >> 4 );
}

void out_LCD_low( INT8U Ch )
/*****************************************************************************
*   Input    : Mask
*   Output   : -
*   Function : Send low part of character to LCD. 
*              This function works only in 4 bit data mode.
******************************************************************************/
{
  INT8U temp;
	  
  temp = GPIO_PORTC_DATA_R & 0x0F;
  GPIO_PORTC_DATA_R  = temp | ((Ch & 0x0F) << 4);
  GPIO_PORTD_DATA_R |= 0x04;        // Select data mode
  GPIO_PORTD_DATA_R |= 0x08;		// Set E High
  GPIO_PORTD_DATA_R &= 0xF7;		// Set E Low
}

void out_LCD_high( INT8U Ch )
/*****************************************************************************
*   Input    : Mask
*   Output   : -
*   Function : Send high part of character to LCD. 
*              This function works only in 4 bit data mode.
******************************************************************************/
{
  out_LCD_low((Ch & 0xF0) >> 4);
}

void wr_ctrl_LCD( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write control data to LCD.
******************************************************************************/
{
  static INT8U Mode4bit = FALSE;
  INT16U i;

  wr_ctrl_LCD_high( Ch );
  if( Mode4bit )
  {
	for(i=0; i<1000; i++);
	wr_ctrl_LCD_low( Ch );
  }
  else
  {
	if( (Ch & 0x30) == 0x20 )
	  Mode4bit = TRUE;
  }
}

void clr_LCD()
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Clear LCD.
******************************************************************************/
{
  wr_ctrl_LCD( 0x01 );
}


void home_LCD()
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Return cursor to the home position.
******************************************************************************/
{
  wr_ctrl_LCD( 0x02 );
}

void Set_cursor( INT8U Ch )
/*****************************************************************************
*   Input    : New Cursor position
*   Output   : -
*   Function : Place cursor at given position.
******************************************************************************/
{
  wr_ctrl_LCD( Ch );
}


void out_LCD( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write control data to LCD.
******************************************************************************/
{
  INT16U i;

  out_LCD_high( Ch );
  for(i=0; i<1000; i++);
  out_LCD_low( Ch );
}


void lcd_task( void *pvParameters )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
  INT8U ch;
  INT8U my_state = LCD_POWER_UP;
  while(1){
      switch( my_state )
      {
          case LCD_POWER_UP:
              LCD_init = 0;
              my_state = LCD_INIT;
              vTaskDelay(100 / portTICK_RATE_MS); // wait 100 ms.
              break;

          case LCD_INIT:
              if( LCD_init_sequense[LCD_init] != 0xFF ){
                  wr_ctrl_LCD( LCD_init_sequense[LCD_init++] );
              }
              else
              {
                  my_state = LCD_READY;
              }
              vTaskDelay(10 / portTICK_RATE_MS); // wait 100 ms.
              break;
          case LCD_READY:
              if(uxQueueMessagesWaiting(xQueue_lcd))
              {
                  if( xSemaphoreTake( xSemaphore_lcd, portMAX_DELAY)) //( TickType_t ) 100 ) == pdTRUE )
                  {
                      if( xQueueReceive( xQueue_lcd, &ch, portMAX_DELAY ))
                      {
                          switch( ch )
                          {
                          case 0xFF:
                              clr_LCD();
                              break;
                          case ESC:
                              my_state = LCD_ESC_RECEIVED;
                              break;
                          default:
                          out_LCD( ch );
                          }
                      }
                      xSemaphoreGive( xSemaphore_lcd );
                  }
              }
              vTaskDelay( 10 / portTICK_RATE_MS); // wait 100-1000 ms. (200-1000)
              break;

          case LCD_ESC_RECEIVED:
              if(uxQueueMessagesWaiting(xQueue_lcd))
              {
                  if( xSemaphoreTake( xSemaphore_lcd, portMAX_DELAY))
                  {
                      if( xQueueReceive( xQueue_lcd, &ch, portMAX_DELAY ))
                      {
                          if( ch & 0x80 )
                          {
                              Set_cursor( ch );
                          }
                          else
                          {
                              switch( ch )
                              {
                              case '@':
                                  home_LCD();
                                  break;
                              }
                          }
                          my_state = LCD_READY;
                          vTaskDelay(100 / portTICK_RATE_MS); // wait 100 ms.
                      }
                      xSemaphoreGive( xSemaphore_lcd );
                  }
              }
              vTaskDelay( 100 / portTICK_RATE_MS); // wait 100-1000 ms. (200-1000)
              break;
      }
  }
}


/****************************** End Of Module *******************************/




