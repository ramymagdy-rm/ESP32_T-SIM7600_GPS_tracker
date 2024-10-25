/**
***************************************************************************************************
** @file       : src/main.cpp
** @brief      : main GNSS app
** @author     : Ramy Ezzat
** @version    : 25-10-2024
** @copyright  : RAMY EZZAT
***************************************************************************************************
***/

/*
**-------------------------------------------------------------------------------------------------
** Includes
**-------------------------------------------------------------------------------------------------
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "utilities.h"
#include "secrets.h"
#include <Ticker.h>
#include <TinyGsmClient.h>


/*
**-------------------------------------------------------------------------------------------------
** Private typedefs
**-------------------------------------------------------------------------------------------------
*/

typedef struct {
    float lat2      = 0.0;  // float lat2;
    float lon2      = 0.0;  // float lon2;
    float speed2    = 0.0;  // float speed2;
    float alt2      = 0.0;  // float alt2;
    int   vsat2     = 0;    // int   vsat2;
    int   usat2     = 0;    // int   usat2;
    float accuracy2 = 0.0;  // float accuracy2;
    int   year2     = 0;    // int   year2;
    int   month2    = 0;    // int   month2;
    int   day2      = 0;    // int   day2;
    int   hour2     = 0;    // int   hour2;
    int   min2      = 0;    // int   min2;
    int   sec2      = 0;    // int   sec2;
} GNSS_DATA_T;


/*
**-------------------------------------------------------------------------------------------------
** Constant variables declaration
**-------------------------------------------------------------------------------------------------
*/

// GPRS credentials, if any
const char apn[] = "internet.emt.ee";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details to test TCP/SSL
const char server[] = "api.thingspeak.com";

// auxilary constants
// time
const uint32_t delay_ms_10 = 10;
const uint32_t delay_ms_100 = 100;
const uint32_t delay_ms_300 = 300;
const uint32_t delay_ms_1000 = 1000;
// stack depth
const uint32_t STACK_SIZE_2000 = 2000;


/*
**-------------------------------------------------------------------------------------------------
** Private variables declaration
**-------------------------------------------------------------------------------------------------
*/

static uint32_t final_sleep_time = OP_DEEP_SLEEP_TIME;


/*
**-------------------------------------------------------------------------------------------------
** Debugger defination
**-------------------------------------------------------------------------------------------------
*/

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

/*
**-------------------------------------------------------------------------------------------------
** Private functions prototypes
**-------------------------------------------------------------------------------------------------
*/

static void light_sleep(uint32_t sec);
static void deep_sleep(uint32_t sec);
static void post_gnss_data(float lat, float lon);
static void power_off(void);

/*
**-------------------------------------------------------------------------------------------------
** Public functions prototypes
**-------------------------------------------------------------------------------------------------
*/
void gnss_task(void *parameters);


/**************************************************************************************************
**  @brief  Arduino setup function
***************************************************************************************************/
void setup() {
    /* Set console baud rate */
    SerialMon.begin(UART_BAUDRATE);
    vTaskDelay(delay_ms_10 / portTICK_PERIOD_MS);

    /* Set GSM module baud rate */
    SerialAT.begin(UART_BAUDRATE, SERIAL_8N1, MODEM_RX, MODEM_TX);

    /* The indicator light of the board can be controlled */
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    /*
        MODEM_PWRKEY IO:4 The power-on signal of the modulator must be given to it,
        otherwise the modulator will not reply when the command is sent
    */
    pinMode(MODEM_PWRKEY, OUTPUT);
    digitalWrite(MODEM_PWRKEY, HIGH);
    vTaskDelay(delay_ms_300 / portTICK_PERIOD_MS);
    digitalWrite(MODEM_PWRKEY, LOW);

    /*
        MODEM_FLIGHT IO:25 Modulator flight mode control,
        need to enable modulator, this pin must be set to high
    */
    pinMode(MODEM_FLIGHT, OUTPUT);
    digitalWrite(MODEM_FLIGHT, HIGH);

    /* create necessary tasks */
    xTaskCreate(gnss_task,          // Task function
                "GNSS task",        // Task name
                STACK_SIZE_2000,    // Task stack depth
                NULL,               // Task parameters
                1,                  // Task priority
                NULL);              // Task Handle
}


/**************************************************************************************************
**  @brief  Arduino loop function
***************************************************************************************************/
void loop() {
}


/**************************************************************************************************
**  @brief  Enter ESP32 light/lite sleep
**  @param[in] sec a uint32 sleep value in seconds
***************************************************************************************************/
static void light_sleep(uint32_t sec) {
    esp_sleep_enable_timer_wakeup(sec * uS_TO_S_FACTOR);
    vTaskDelay(delay_ms_300 / portTICK_PERIOD_MS);
    esp_light_sleep_start();
}


/**************************************************************************************************
**  @brief  Enter ESP32 deep sleep
**  @param[in] sec a uint32 sleep value in seconds
***************************************************************************************************/
static void deep_sleep(uint32_t sec) {
    esp_sleep_enable_timer_wakeup(sec * uS_TO_S_FACTOR); // Timer wakeup after 300sec ~ 5mins
    vTaskDelay(delay_ms_300 / portTICK_PERIOD_MS);
    esp_deep_sleep_start();
}


/**************************************************************************************************
**  @brief  Post GNSS data to thingspeak
**  @param[in] lat a float latitude value
**  @param[in] lon a float longitute value
***************************************************************************************************/
void post_gnss_data(float lat, float lon) {
    // gsm test tcp
    TinyGsmClient client(modem, 0);
    const int port = 80;
    DBG("Connecting to ", server);
    if (!client.connect(server, port)) {
        DBG("... failed");
    } else {
        // Convert float to a string, adjust the size based on the expected length
        char lat_buffer[20];
        char lon_buffer[20];
        dtostrf(lat, 4, 8, lat_buffer);
        dtostrf(lon, 4, 8, lon_buffer);

        client.print(String("GET ") + "https://api.thingspeak.com/update?api_key=" + API_KEY + "&field1=" + lat_buffer + "&field2=" + lon_buffer + " HTTP/1.0\r\n");
        client.print(String("Host: ") + server + "\r\n");
        client.print("Connection: close\r\n\r\n");
        // Wait for data to arrive
        uint32_t start = millis();
        while (client.connected() && !client.available() &&
            millis() - start < 30000L) {
        vTaskDelay(delay_ms_100 / portTICK_PERIOD_MS);
        };

        // Read data
        start = millis();
        while (client.connected() && millis() - start < 5000L) {
            while (client.available()) {
                SerialMon.write(client.read());
                start = millis();
            }
        }

        client.stop();
    }
}


/**************************************************************************************************
**  @brief  power off routine
***************************************************************************************************/
void power_off(void) {
    // disconnect gprs
    modem.gprsDisconnect();
    // Wait for gprs to power off
    light_sleep(5);
    if (!modem.isGprsConnected()) {
        DBG("GPRS disconnected");
    } else {
        DBG("GPRS disconnection: Failed");
    }

    // modem turn off
    DBG("Powering off modem");
    modem.poweroff();

    DBG("End of clycle, enabling deep sleep. Will wake up in ", final_sleep_time, " seconds");
    // Wait for modem to power off
    light_sleep(5);

    // go to deep sleep
    deep_sleep(final_sleep_time);
}


/**************************************************************************************************
**  @brief  Aquire and post GNSS data
**  @param[in] void NULL parameters
***************************************************************************************************/
void gnss_task(void *parameters) {

    for(;;) {
        // initialise modem
        DBG("Initializing modem...");
        if (!modem.init()) {
            DBG("Failed to restart modem, delaying 10s and retrying");
            return;
        }

        // gsm test gnss 
        String ret;
        //    do {
        //        ret = modem.setNetworkMode(2);
        //        vTaskDelay(delay_ms_300 / portTICK_PERIOD_MS);
        //    } while (ret != "OK");
        ret = modem.setNetworkMode(2);
        DBG("setNetworkMode:", ret);

        // get GNSS mode
        uint8_t mode = modem.getGNSSMode();
        DBG("GNSS Mode:", mode);

        // set GNSS mode for assurance
        modem.setGNSSMode(1, 1);
        light_sleep(1);

        // set modem mode
        String name = modem.getModemName();
        DBG("Modem Name:", name);

        // set modem info
        String modemInfo = modem.getModemInfo();
        DBG("Modem Info:", modemInfo);

        // Unlock your SIM card with a PIN if any
        if (GSM_PIN && modem.getSimStatus() != 3) {
            modem.simUnlock(GSM_PIN);
        }

        DBG("Waiting for network...");
        if (!modem.waitForNetwork(600000L)) {
            DBG("Failed modem network, sleeping for sleeping for now and retrying in 1H");
            final_sleep_time = LONG_DEEP_SLEEP_TIME;
            power_off();
        }

        // check if network is connected
        if (modem.isNetworkConnected()) {
            DBG("Network connected");
        }

        // gsm test gprs
        DBG("Connecting to", apn);
        if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
            DBG("Failed connecting to", apn, "sleeping for sleeping for now and retrying in 1H");
            final_sleep_time = LONG_DEEP_SLEEP_TIME;
            power_off();
        }

        // checking if GPRS is connected
        bool res = modem.isGprsConnected();
        DBG("GPRS status:", res ? "connected" : "not connected");

        // get sim CCID
        String ccid = modem.getSimCCID();
        DBG("CCID:", ccid);

        // get IMEI
        String imei = modem.getIMEI();
        DBG("IMEI:", imei);

        // get IMSI
        String imsi = modem.getIMSI();
        DBG("IMSI:", imsi);

        // get operator
        String cop = modem.getOperator();
        DBG("Operator:", cop);

        // get local ip adress
        IPAddress local = modem.localIP();
        DBG("Local IP:", local);

        // get signal quality
        int csq = modem.getSignalQuality();
        DBG("Signal quality:", csq);

        // gsm test gps
        DBG("Enabling GPS/GNSS/GLONASS");
        modem.enableGPS();
        light_sleep(2);

        // get GNSS data
        GNSS_DATA_T gnss_data;
        for (int i = 0; i < MAX_GNSS_TRIALS; i++) {
            DBG("Requesting current GPS/GNSS/GLONASS location");
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            if (modem.getGPS(&gnss_data.lat2, &gnss_data.lon2, 
                            &gnss_data.speed2, &gnss_data.alt2, 
                            &gnss_data.vsat2, &gnss_data.usat2, 
                            &gnss_data.accuracy2, &gnss_data.year2, 
                            &gnss_data.month2, &gnss_data.day2, 
                            &gnss_data.hour2, &gnss_data.min2, &gnss_data.sec2)) {
                DBG("Latitude:", String(gnss_data.lat2, 8), " | Longitude:", String(gnss_data.lon2, 8));
                DBG("Speed:", gnss_data.speed2, " | Altitude:", gnss_data.alt2);
                //DBG("Visible Satellites:", gnss_data.vsat2, "\tUsed Satellites:", gnss_data.usat2);
                //DBG("Accuracy:", gnss_data.accuracy2);
                DBG("Year:", gnss_data.year2, " | Month:", gnss_data.month2, " | Day:", gnss_data.day2);
                DBG("UTC time Hour:", gnss_data.hour2, " | Minute:", gnss_data.min2, " | Second:", gnss_data.sec2);
                break;
            } else {
                // Wait for retry
                light_sleep(30);
            }
        }

        DBG("Disabling GPS");
        modem.disableGPS();
        
        // check gnss data validity
        if (gnss_data.lat2 == 0.0 && gnss_data.lon2 == 0.0) {
            DBG("lat ", gnss_data.lat2, " lon ", gnss_data.lon2, " ARE incorrect ... sleeping for sleeping for now and retrying in 1H");
            final_sleep_time = LONG_DEEP_SLEEP_TIME;
        } else {
            // post GNSS data
            post_gnss_data(gnss_data.lat2, gnss_data.lon2);
            final_sleep_time = OP_DEEP_SLEEP_TIME;
        }

        //power off routine
        power_off();
    }
}

/****************************************** END OF FILE ******************************************/