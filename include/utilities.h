/**
***************************************************************************************************
** @file       : include/utilities.h
** @brief      : utilities preprocessors
** @author     : Ramy Ezzat
** @version    : 25-10-2024
** @copyright  : RAMY EZZAT
**************************************************************************************************
***/

/* Define to prevent recursive inclusion */
#ifndef UTILITIES_H
#define UTILITIES_H


/*
**-------------------------------------------------------------------------------------------------
** User defines preproc
**-------------------------------------------------------------------------------------------------
*/

#define uS_TO_S_FACTOR      1000000ULL  /* Conversion factor for micro seconds to seconds */
#define LONG_DEEP_SLEEP_TIME    3600    /* Long time ESP32 will go to sleep (in seconds) */
#define OP_DEEP_SLEEP_TIME  300         /* operational time ESP32 will go to sleep (in seconds) */
#define MAX_GNSS_TRIALS     10

#define UART_BAUDRATE       115200

#define MODEM_TX            27
#define MODEM_RX            26
#define MODEM_PWRKEY        4
#define MODEM_DTR           32
#define MODEM_RI            33
#define MODEM_FLIGHT        25
#define MODEM_STATUS        34

#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14
#define SD_CS               13

#define LED_PIN             12


/*
**-------------------------------------------------------------------------------------------------
** Preprocessors
**-------------------------------------------------------------------------------------------------
*/

// Define T-SIM module
#define TINY_GSM_MODEM_SIM7600

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
#define SerialAT Serial1

// See all AT commands
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints
#define TINY_GSM_DEBUG SerialMon

#endif /* UTILITIES_H */

/****************************************** END OF FILE ******************************************/