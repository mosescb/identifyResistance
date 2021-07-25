/*****************************************************************************
*
*      I D E N T I F Y   R E S I S T A N C E   O F   R E S I S T O R
*
* Author: Moses Christopher Bollavarapu <moseschristopherb@gmail.com>
*
*****************************************************************************/


/*****************************************************************************
*             G L O B A L   D E F I N E S   &   C O N S T A N T S
*****************************************************************************/
#define LCD_IS_I2C         0       // 0 => Parallel Display; 1 => I2C Display
#define SERIAL_DEBUG_EN    1       // 0 => Disable Debug;    1=> Enable Debug
#define LCD_I2C_ADDR       0x3F    // I2C Address of LCD

const int   ADC_RESOLUTION_MAX  = 1023;       // 10-bit ADC (0-1023)
const float ADC_REF_VOLTAGE     = 5.0;        // 5V Reference
const int   REF_RESISTOR_R1     = 10000;      // 10K ohm reference
const int   RESISTANCE_MAX      = 12000;      // 10K ohm with 20% tolerance
const int   RESISTANCE_MIN      = 80;         // 100 ohm with 20% tolerance
const int   NO_OF_VALS_TO_MATCH = 5;          // Number of same consecutive vals
const int   ADC_DIFF_TO_IGNORE  = 10;         // Acceptable ADC Val diff
const int   ADC_MEAS_DELAY      = 100;        // 100ms gap between two measurements

const int   ADC_MEAS_PIN        = A0;         // Analog 0 pin
const int   BUZZER_PIN          = 9;          // Buzzer pin
const int   REG_SEL             = 12;         // Register Select
const int   LCD_EN              = 11;         // LCD Enable
const int   LCD_DATA[4]         = {5,4,3,2};  // LCD Data lines

const int   LCD_ROWS            = 2;          // 16x2 LCD
const int   LCD_COLS            = 16;         // 16x2 LCD

#if LCD_IS_I2C
  const int   I2C_LCD_RW        = 1;          // LCD Read Write
  const int   I2C_LCD_EN        = 2;          // LCD Enable
  const int   I2C_LCD_RS        = 0;          // LCD Register Select
  const int   I2C_LCD_D4        = 4;          // LCD Data line 4
  const int   I2C_LCD_D5        = 5;          // LCD Data line 5
  const int   I2C_LCD_D6        = 6;          // LCD Data line 6
  const int   I2C_LCD_D7        = 7;          // LCD Data line 7
  const int   I2C_LCD_BL        = 3;          // LCD Backlight
#endif

/*****************************************************************************
*             I N C L U D E S
*****************************************************************************/
#if LCD_IS_I2C
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
#else
  #include <LiquidCrystal.h>
#endif


/*****************************************************************************
*             G L O B A L   V A R I A B L E S
*****************************************************************************/
volatile int  g_ixPrevVal   = -1;
volatile int  g_ixCurrVal   = -1;
volatile bool g_bBuzzed     = false;
volatile bool g_bInKilo     = false;
volatile bool g_bStabilized = false;
static   int  gs_uixCounter = 0;

#if LCD_IS_I2C
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR, I2C_LCD_RW, I2C_LCD_EN, I2C_LCD_RS, I2C_LCD_D4,
                        I2C_LCD_D5,   I2C_LCD_D6, I2C_LCD_D7, I2C_LCD_BL, POSITIVE);
#else
  LiquidCrystal lcd(REG_SEL, LCD_EN, LCD_DATA[0], LCD_DATA[1], LCD_DATA[2], LCD_DATA[3]);
#endif

/*****************************************************************************
@name: clearLcd()
@brief: clears the lcd on a particular line from a given cursor position
@param: f_ixCursor - Cursor position to start clearing
@param: f_ixLine - Line/Row to clear
*****************************************************************************/
void clearLcd(int f_ixCursor, int f_ixLine)
{
    lcd.setCursor(f_ixCursor,f_ixLine);

    for(int index = 0; index< LCD_COLS; index++)
        lcd.print(" ");
}
/*****************************************************************************
@name: buzz()
@brief: Buzzer buzzes for a given duration at a given pwm value
@param: f_fDurationInSeconds - Buzz duration in seconds
@param: f_u8Pwm - PWM value at which the buzzer buzzes
*****************************************************************************/
void buzz(float f_fDurationInSeconds, uint8_t f_u8Pwm)
{
    analogWrite(BUZZER_PIN, f_u8Pwm);
    delay(f_fDurationInSeconds * 1000);
    analogWrite(BUZZER_PIN, 0);
    g_bBuzzed = true;
}

/*****************************************************************************
@name: setup()
@brief: setup function executes once on controller reset
        - Configure the GPIOs
        - Buzz to denote Startup
        - Start the LCD
        - Start Serial Communication(based on flag)
*****************************************************************************/
void setup() {
    pinMode(ADC_MEAS_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    buzz(0.3, 125); // Program Init

    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.print("Resistance Meter");

#if SERIAL_DEBUG_EN
    Serial.begin(115200);
#endif

    delay(50);
}

/*****************************************************************************
@name: loop()
@brief: loop function is executed like while(1) [infinite loop]
        - Read ADC value
        - Calculate Resistance
        - Clear and Display on LCD
        - Serial Debug Interface (based on flag)
*****************************************************************************/
void loop() {

    // To compare the ADC values measured at 100ms time gap
    g_ixPrevVal = g_ixCurrVal;
    delay(ADC_MEAS_DELAY);
    g_ixCurrVal = analogRead(ADC_MEAS_PIN);

    // Measure Resistance from ADC value
    volatile float fAdcVol       = (g_ixCurrVal * ADC_REF_VOLTAGE) / ADC_RESOLUTION_MAX;
    volatile float fResistanceR2 = REF_RESISTOR_R1 * ((ADC_REF_VOLTAGE / fAdcVol) - 1);

    // Check if the Resistance value is out of bounds
    if((fResistanceR2 > RESISTANCE_MAX) || (fResistanceR2 < RESISTANCE_MIN))
    {
        fResistanceR2 = 0;
    }

#if SERIAL_DEBUG_EN
    Serial.print("RawADC Value: ");
    Serial.print(g_ixCurrVal);
    Serial.print("\tADC Voltage: ");
    Serial.print(fAdcVol);
    Serial.print("\tResistance: ");
    Serial.println(fResistanceR2);
#endif

    // To denote in Kilo Ohms notation on Screen
    volatile float fTempRes = fResistanceR2;
    if(int(fResistanceR2) >= 1000)
    {
      fTempRes /= 1000;
      g_bInKilo = true;
    }
    else
    {
      g_bInKilo = false;
    }

    if(g_bStabilized || fResistanceR2 == 0)
    {
      // Clear only second row as Title remains constant.
      clearLcd(0,1);
      lcd.setCursor(0, 1);
      lcd.print(fTempRes);

      if(g_bInKilo)
      {
        lcd.setCursor(12, 1);
        lcd.print("KOhm");
      }
      else
      {
        lcd.setCursor(13, 1);
        lcd.print("Ohm");
      }
      g_bStabilized = false; // Stop refreshing after this point...
    }

    // Check for last 'n' consecutive measurements to be matching (stabilization)
    // Buzz after having stable value
    if((g_ixPrevVal == g_ixCurrVal) && (int(fResistanceR2) >= RESISTANCE_MIN))
    {
      gs_uixCounter++;
      if((!g_bBuzzed) && (gs_uixCounter >= NO_OF_VALS_TO_MATCH))
      {
        g_bStabilized = true;
        buzz(0.15, 60);
      }
    }
    else if( abs(g_ixPrevVal - g_ixCurrVal) >= int(g_ixCurrVal/ADC_DIFF_TO_IGNORE) )
    {
      g_bBuzzed = false;
      g_bStabilized = false;
      gs_uixCounter = 0;
    }
}
