/**
  * @file board_selector.h
  * @brief Detects which light board the firmware runs on from the selector ADC pin voltage.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef BOARD_SELECTOR_H
#define BOARD_SELECTOR_H

// Expected selector pin voltage for each board.
#define LEFT_FRONT_VOLTAGE (2.2f)
#define RIGHT_FRONT_VOLTAGE (3.0f)
#define LEFT_BACK_VOLTAGE (0.0f)
#define RIGHT_BACK_VOLTAGE (1.0f)

typedef enum
{
	BOARD_LEFT_BACK = 0,
	BOARD_RIGHT_BACK = 1,
	BOARD_LEFT_FRONT = 2,
	BOARD_RIGHT_FRONT = 3,
	BOARD_UNKNOWN
} board_e;

/**
  * @brief Identify the board by reading the selector pin voltage.
  * @retval board_e   Detected board, or BOARD_UNKNOWN if no threshold matched.
  */
board_e BOARD_ChooseBoard(void);

#endif // BOARD_SELECTOR_H
