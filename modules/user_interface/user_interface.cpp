//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "user_interface.h"

#include "code.h"
#include "siren.h"
#include "smart_home_system.h"
#include "fire_alarm.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "matrix_keypad.h"
#include "display.h"

//=====[Declaration of private defines]========================================

#define DISPLAY_REFRESH_TIME_MS       1000   
#define ALARM_REPORT_INTERVAL_MS      60000  
#define TEMPERATURE_WARNING_THRESHOLD 30     

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromUserInterface[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static bool incorrectCodeState = OFF;
static bool systemBlockedState = OFF;

static bool codeComplete = false;
static int numberOfCodeChars = 0;

static int accumulatedAlarmReportTime = 0;

static bool previousGasWarningState   = false;
static bool previousTempWarningState  = false;

static bool promptDisplayed = false; 

//=====[Declarations (prototypes) of private functions]========================

static void userInterfaceMatrixKeypadUpdate();
static void incorrectCodeIndicatorUpdate();
static void systemBlockedIndicatorUpdate();

static void userInterfaceDisplayInit();
static void userInterfaceDisplayUpdate();

//=====[Implementations of public functions]===================================

void userInterfaceInit()
{
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
    matrixKeypadInit( SYSTEM_TIME_INCREMENT_MS );
    userInterfaceDisplayInit();
}

void userInterfaceUpdate()
{
    
    userInterfaceMatrixKeypadUpdate(); 
    incorrectCodeIndicatorUpdate();
    systemBlockedIndicatorUpdate();
    userInterfaceDisplayUpdate();      

    
    if ( sirenStateRead() && !promptDisplayed ){
        promptDisplayed = true;
        displayCharPositionWrite( 0, 3 );
        displayStringWrite( "                " ); 
        displayCharPositionWrite( 0, 3 );
        displayStringWrite( "code:" );
    }
    if ( !sirenStateRead() ) {
        promptDisplayed = false; 
    }

   
    char keyPressed = matrixKeypadUpdate();

    if ( keyPressed == '4' ) {
        displayCharPositionWrite( 0, 3 );
        displayStringWrite( "                " ); 
        displayCharPositionWrite( 0, 3 );
        if ( gasDetectorStateRead() ) {
            displayStringWrite( "Gas:DET " );
        } else {
            displayStringWrite( "Gas:OK  " );
        }
    }

    if ( keyPressed == '5' ) {
        displayCharPositionWrite( 0, 3 );
        displayStringWrite( "                " );
        displayCharPositionWrite( 0, 3 );
        if ( overTemperatureDetectorStateRead() ){
            displayStringWrite( "Tmp:HIGH" );
        } else {
            displayStringWrite( "Tmp:OK  " );
        }
    }

    
    accumulatedAlarmReportTime += SYSTEM_TIME_INCREMENT_MS;

    if ( accumulatedAlarmReportTime >= ALARM_REPORT_INTERVAL_MS ) {
        accumulatedAlarmReportTime = 0;
        displayCharPositionWrite( 8, 3 );
        if ( sirenStateRead() ) {
            displayStringWrite( "Alm:ON  " );
        } else {
            displayStringWrite( "Alm:OFF " );
        }
    }

   bool currentGasWarning  = gasDetectorStateRead();
    bool currentTempWarning = temperatureSensorReadCelsius() > TEMPERATURE_WARNING_THRESHOLD;

    if ( currentGasWarning != previousGasWarningState ) {
        previousGasWarningState = currentGasWarning;
        if ( !promptDisplayed ) {  
            displayCharPositionWrite( 0, 3 );
            if ( currentGasWarning ) {
                displayStringWrite( "GAS:( " );
            } else {
                displayStringWrite( "        " );
            }
        }
    }

    if ( currentTempWarning != previousTempWarningState ) {
        previousTempWarningState = currentTempWarning;
        if ( !promptDisplayed ) {  
            displayCharPositionWrite( 0, 3 );
            if ( currentTempWarning ) {
                displayStringWrite( "HOT:( " );
            } else {
                displayStringWrite( "        " );
            }
        }
    }}

bool incorrectCodeStateRead()
{
    return incorrectCodeState;
}

void incorrectCodeStateWrite( bool state )
{
    incorrectCodeState = state;
}

bool systemBlockedStateRead()
{
    return systemBlockedState;
}

void systemBlockedStateWrite( bool state )
{
    systemBlockedState = state;
}

bool userInterfaceCodeCompleteRead()
{
    return codeComplete;
}

void userInterfaceCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

static void userInterfaceMatrixKeypadUpdate()
{
    static int numberOfHashKeyReleased = 0;
    char keyReleased = matrixKeypadUpdate();

    if( keyReleased != '\0' ) {

        if( sirenStateRead() && !systemBlockedStateRead() ) {
            if( !incorrectCodeStateRead() ) {
                codeSequenceFromUserInterface[numberOfCodeChars] = keyReleased;
                numberOfCodeChars++;
                if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
                    codeComplete = true;
                    numberOfCodeChars = 0;
                }
            } else {
                if( keyReleased == '#' ) {
                    numberOfHashKeyReleased++;
                    if( numberOfHashKeyReleased >= 2 ) {
                        numberOfHashKeyReleased = 0;
                        numberOfCodeChars = 0;
                        codeComplete = false;
                        incorrectCodeState = OFF;
                    }
                }
            }
        }
    }
}

static void userInterfaceDisplayInit()
{
    displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );

    displayCharPositionWrite( 0, 0 );
    displayStringWrite( "Temperature:" );

    displayCharPositionWrite( 0, 1 );
    displayStringWrite( "Gas:" );

    displayCharPositionWrite( 0, 2 );
    displayStringWrite( "Alarm:" );

    displayCharPositionWrite( 0, 3 );
    displayStringWrite( "code:" );
}

static void userInterfaceDisplayUpdate()
{
    static int accumulatedDisplayTime = 0;
    char temperatureString[3] = "";

    if( accumulatedDisplayTime >= DISPLAY_REFRESH_TIME_MS ) {

        accumulatedDisplayTime = 0;

        sprintf( temperatureString, "%.0f", temperatureSensorReadCelsius() );
        displayCharPositionWrite( 12, 0 );
        displayStringWrite( temperatureString );
        displayCharPositionWrite( 14, 0 );
        displayStringWrite( "'C" );

        displayCharPositionWrite( 4, 1 );
        if ( gasDetectorStateRead() ) {
            displayStringWrite( "Detected    " );
        } else {
            displayStringWrite( "Not Detected" );
        }

        displayCharPositionWrite( 6, 2 );
        if ( sirenStateRead() ) {
            displayStringWrite( "ON " );
        } else {
            displayStringWrite( "OFF" );
        }

    } else {
        accumulatedDisplayTime += SYSTEM_TIME_INCREMENT_MS;
    }
}

static void incorrectCodeIndicatorUpdate()
{
    incorrectCodeLed = incorrectCodeStateRead();
}

static void systemBlockedIndicatorUpdate()
{
    systemBlockedLed = systemBlockedState;
}
