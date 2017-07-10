/*
 * main.c
 */

#include "inc\lm3s1968.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/debug.h"
#include "drivers/rit128x96x4.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/watchdog.h"
#include <stdio.h>

//NOTE: The on-board buttons are high for not-pressed and low for pressed

//Information regarding a GPIO button
typedef struct
{
	long gpio_port_base;
	long gpio_pin_base;
	long button_state;
} button_t;

#define NUM_BUTTONS 5

//Used to choose a button from the button array
typedef enum
{
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_SELECT
} button_index;

//Keeps information about the player
typedef struct
{
	char *name;
	unsigned long life;
	unsigned long cmdr;
} player_t;

//Used to shift the cursor between four players
#define CURSOR_ROW_OFFSET_TOP 0
#define CURSOR_ROW_OFFSET_BOTTOM 48
#define CURSOR_COL_OFFSET_LEFT 0
#define CURSOR_COL_OFFSET_RIGHT 64

//Button array
static button_t buttons[NUM_BUTTONS];

//Used to draw a pointer on the display
static const unsigned char cursor[] = {
		0xF0,0x00,
		0xFF,0x00,
		0xFF,0xF0,
		0xFF,0x00,
		0xF0,0x00
};

//Clears the pointer off the screen
static const unsigned char clear[] = {
		0x00,0x00,
		0x00,0x00,
		0x00,0x00,
		0x00,0x00,
		0x00,0x00
};

//Each player
static player_t playerOne, playerTwo, playerThree, playerFour;

//Initialise a button struct
void buttonInit(button_t *button, long port, long pin)
{
	button->gpio_port_base = port;
	button->gpio_pin_base = pin;
	button->button_state = pin;
}

void buttonsInitLM3S1968(void)
{

	/* Initialise GPIO port G */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	SysCtlPeripheralEnable(GPIO_PORTG_BASE);

	GPIOPinTypeGPIOInput(GPIO_PORTG_BASE, (GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7));
	GPIOPadConfigSet(GPIO_PORTG_BASE, (GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7), GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	long i;
	for (i = 0; i < NUM_BUTTONS; i++)
	{
		buttonInit(&buttons[i], GPIO_PORTG_BASE, GPIO_PIN_3 << i);
	}
}

//Return the state of the button
tBoolean buttonStateGet (button_t *button)
{
	return GPIOPinRead(button->gpio_port_base, button->gpio_pin_base);
}

//TODO: Use this to remove locking mechanism
/*tBoolean buttonStateGetSinglePress (button_t *button)
{
	return 0;
}*/

//Initialises a player struct
player_t playerInit (player_t *player, char *name)
{
	player->life = 40;
	player->cmdr = 0;
	player->name = name;
	return *player;
}

//Shows information about the player
void playerPrint (player_t *player, unsigned long rowOffset, unsigned long colOffset)
{
	char s[30];
	char t[30];
	char u[30];

	sprintf(s, "%s", player->name);
	sprintf(t, "  %04d  ", player->life);
	sprintf(u, "   %02d   ", player->cmdr);

	RIT128x96x4StringDraw(s, colOffset, rowOffset, 15);
	RIT128x96x4StringDraw(t, colOffset, rowOffset + 10, 15);
	RIT128x96x4StringDraw(u, colOffset, rowOffset + 20, 15);
}

//Used to determine which player the cursor points to
player_t *getSelectedPlayer (unsigned long row, unsigned long column)
{
	if ((!row) && (!column))
	{
		return &playerOne;
	}
	if ((!row) && (column))
	{
		return &playerTwo;
	}
	if ((row) && (!column))
	{
		return &playerThree;
	}
	if ((row) && (column))
	{
		return &playerFour;
	}
}

void main(void) {

	// Set the clocking to run directly from the crystal.
	SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
				   SYSCTL_XTAL_8MHZ);

	/* Initialise the display */
	RIT128x96x4Init(1000000);

	/* Set up the buttons */
	buttonsInitLM3S1968();

	playerOne = playerInit (&playerOne, "Player 1");
	playerTwo = playerInit (&playerTwo, "Player 2");
	playerThree = playerInit (&playerThree, "Player 3");
	playerFour = playerInit (&playerFour, "Player 4");

	unsigned long ulRow, ulCol, ulWidth, ulHeight;
	ulCol = CURSOR_COL_OFFSET_LEFT;
	ulRow = CURSOR_ROW_OFFSET_TOP;
	long n;
	long lock = 0;

	ulWidth = 4;
	ulHeight = 5;

	while (1)
	{

		//RIT128x96x4Clear();

		//Clear the cursor's previous position after refreshing
		RIT128x96x4ImageDraw(clear, ulCol, ulRow, ulWidth, ulHeight);

		//Set the current state
		for (n = 0; n < NUM_BUTTONS; n++)
		{
			buttons[n].button_state = buttonStateGet(&buttons[n]);
		}


		if (!buttons[BUTTON_UP].button_state && buttons[BUTTON_SELECT].button_state)
		{
			if (ulRow == CURSOR_ROW_OFFSET_BOTTOM)
			{
				ulRow = CURSOR_ROW_OFFSET_TOP;

			}
		}
		if (!buttons[BUTTON_DOWN].button_state && buttons[BUTTON_SELECT].button_state)
		{
			if (ulRow == CURSOR_ROW_OFFSET_TOP)
			{
				ulRow = CURSOR_ROW_OFFSET_BOTTOM;
			}
		}
		if (!buttons[BUTTON_LEFT].button_state && buttons[BUTTON_SELECT].button_state)
		{
			if (ulCol == CURSOR_COL_OFFSET_RIGHT)
			{
				ulCol = CURSOR_COL_OFFSET_LEFT;
			}
		}
		if (!buttons[BUTTON_RIGHT].button_state && buttons[BUTTON_SELECT].button_state)
		{
			if (ulCol == CURSOR_COL_OFFSET_LEFT)
			{
				ulCol = CURSOR_COL_OFFSET_RIGHT;
			}
		}

		//Determine what to do while pressing select
		if (!buttonStateGet(&buttons[BUTTON_SELECT]))
		{
			if (!buttonStateGet(&buttons[BUTTON_UP]))
			{
				if (!lock)
				{
					(getSelectedPlayer(ulRow, ulCol))->life += 1;
					lock = 1;
				}
			}
			else if (!buttonStateGet(&buttons[BUTTON_DOWN]))
			{
				if (!lock)
				{
					(getSelectedPlayer(ulRow, ulCol))->life -= 1;
					lock = 1;
				}
			}
			else if (!buttonStateGet(&buttons[BUTTON_LEFT]))
			{
				if (!lock)
				{
					(getSelectedPlayer(ulRow, ulCol))->cmdr -= 1;
					lock = 1;
				}
			}
			else if (!buttonStateGet(&buttons[BUTTON_RIGHT]))
			{
				if (!lock)
				{
					(getSelectedPlayer(ulRow, ulCol))->cmdr += 1;
					lock = 1;
				}
			}
			else
			{
				if (lock)
				{
					lock = 0;
				}
			}
		}


		RIT128x96x4ImageDraw(cursor, ulCol, ulRow, ulWidth, ulHeight);

		playerPrint(&playerOne, CURSOR_ROW_OFFSET_TOP, CURSOR_COL_OFFSET_LEFT + 5);
		playerPrint(&playerTwo, CURSOR_ROW_OFFSET_TOP, CURSOR_COL_OFFSET_RIGHT + 5);
		playerPrint(&playerThree, CURSOR_ROW_OFFSET_BOTTOM, CURSOR_COL_OFFSET_LEFT + 5);
		playerPrint(&playerFour, CURSOR_ROW_OFFSET_BOTTOM, CURSOR_COL_OFFSET_RIGHT + 5);

		//Delay for approximately 1/60th of a second
		SysCtlDelay(SysCtlClockGet() / 180);

	}
}
