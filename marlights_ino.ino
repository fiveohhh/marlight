// switch pins
#define RIGHT_BLINKER_SWITCH_PIN 3
#define LEFT_BLINKER_SWITCH_PIN 4
#define BRAKE_SWITCH_PIN 8

// light pins
#define BRAKE_LIGHT_PIN 13 // This pin used to turn on brake light
#define POWER_PWM_PIN 5 // PWM for light power source
#define LEFT_BLINKER_LIGHT_PIN 12 // This pin used to turn on brake light
#define RIGHT_BLINKER_LIGHT_PIN 11 // This pin used to turn on brake light

#define MILLIS_MULTIPLIER 7 // Since we changed PWM freq, millis now reports milliseconds *7

#define LOW_PWM 177 // dim value of 50Hz flicker
#define HIGH_PWM 255 // bright value of 50Hz flicker

#define SHORT_DELAY 60
#define LONG_DELAY 120
#define BLINKER_DELAY 400

#define BRAKE_ON 0
#define BRAKE_OFF 1

#define BLINKER_STATUS_OFF 0
#define BLINKER_STATUS_LEFT 1
#define BLINKER_STATUS_RIGHT 2

#define PROGRAMMING_DELAY_MILLISECONDS 5000 // time to wait for programming input


enum VOLTAGE_STATE {
    VOLTAGE_STATE_UNKNOWN,
    VOLTAGE_STATE_TOO_LOW,
    VOLTAGE_STATE_NON_CHARGING,
    VOLTAGE_STATE_CHARGING,
    VOLTAGE_STATE_TOO_HIGH
};

enum LED_COLOR {
    LED_COLOR_OFF,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_ORANGE,
    LED_COLOR_BLUE
};

static VOLTAGE_STATE v_state = VOLTAGE_STATE_UNKNOWN;

void setLed(LED_COLOR ledColor)
{
}

// this is running at 60Hz
ISR(TIMER1_COMPA_vect){  //change the 0 to 1 for timer1 and 2 for timer2
    // used to toggle power to all pins.
   
    // 0 is off
    static int blinkStatus = BLINKER_STATUS_OFF;
    static unsigned long blinkerStartTime;

    // used to track the blinking voltage light.
    static unsigned char voltage_counter = 0;
    


    /*--This controls the MOSFET that gives our lights a 60Hz flicker---*/
    static bool isOn = 0;
    if (isOn)
    {
        // Set to low brightness
         analogWrite(POWER_PWM_PIN, LOW_PWM);
         isOn = 0;
    }
   else
   {
     // Set to high brightness
     analogWrite(POWER_PWM_PIN, HIGH_PWM);
     isOn = 1;
   }
    /*-------------------------------------------------------------------*/

#ifdef FEATURE_VOLTAGE_DETECT
    /*----Voltage light blink---*/
    if (v_state == VOLTAGE_STATE_TOO_LOW)
    {
        // voltage low, blink orange
        if (voltage_counter <= 30)
        {
            // turn led on orange
            setLed(LED_COLOR_ORANGE);
        }
        else
        {
            // shut led off
            setLed(LED_COLOR_OFF);
        }

    }
    else if (v_state == VOLTAGE_STATE_TOO_HIGH)
    {
        // voltage low, blink RED
        if (voltage_counter <= 30)
        {
            // turn led on red
            setLed(LED_COLOR_RED);
        }
        else
        {
            // shut led off
            setLed(LED_COLOR_OFF);
        }
    }
    else if (v_state == VOLTAGE_STATE_CHARGING)
    {
        // normal operations solid green light
        setLed(LED_COLOR_GREEN);
    }
    else if (v_state == VOLTAGE_STATE_NON_CHARGING)
    {
        // Probably not charging, orange light
        setLed(LED_COLOR_ORANGE);
    }
    else
    {
        // shouldn't be here.  turn led blue
    }
    if (voltage_counter >= 60)
    {
        // reset the voltage counter.
        voltage_counter = 0;
    }
    /*--------------------------*/
#endif


    /* We're assuming only one blinker will be present at a time.*/
#ifdef BLINKER_SUPPORT
    // handle left blinker stuffs
    if (digitalRead(LEFT_BLINKER_SWITCH_PIN) == LOW)
    {
#ifdef EXTERNAL_FLASHER
#else
        if (blinkStatus == BLINKER_STATUS_OFF)
        {
            // was off, now on, set start time
            blinkerStartTime = millis() / MILLIS_MULTIPLIER;
            digitalWrite(LEFT_BLINKER_LIGHT_PIN, HIGH);
        }
        if (((millis() / MILLIS_MULTIPLIER) - blinkerStartTime) > BLINKER_DELAY)
        {
            // toggle blinker
            digitalWrite(LEFT_BLINKER_LIGHT_PIN, !digitalRead(LEFT_BLINKER_LIGHT_PIN));

            // reset starttime
            blinkerStartTime = millis() / MILLIS_MULTIPLIER;
        }

        blinkStatus = BLINKER_STATUS_LEFT;
#endif
    }
    else if (digitalRead(LEFT_BLINKER_SWITCH_PIN) == HIGH)
    {
#ifdef EXTERNAL_FLASHER
#else
        // if it was on, but isn't, make sure we shut it off
        digitalWrite(LEFT_BLINKER_LIGHT_PIN, LOW);
#endif
    }

    // handle right blinker
    if (digitalRead(RIGHT_BLINKER_SWITCH_PIN) == LOW)
    {
#ifdef EXTERNAL_FLASHER
#else
        if (blinkStatus == BLINKER_STATUS_OFF)
        {
            // was off, now on, set start time
            blinkerStartTime = millis() / MILLIS_MULTIPLIER;
            digitalWrite(RIGHT_BLINKER_LIGHT_PIN, HIGH);
        }
        if (((millis() / MILLIS_MULTIPLIER) - blinkerStartTime) > BLINKER_DELAY)
        {
            // toggle blinker
            digitalWrite(RIGHT_BLINKER_LIGHT_PIN, !digitalRead(RIGHT_BLINKER_LIGHT_PIN));

            // reset starttime
            blinkerStartTime = millis() / MILLIS_MULTIPLIER;
        }

        blinkStatus = BLINKER_STATUS_RIGHT;
#endif
    }
    else if (digitalRead(RIGHT_BLINKER_SWITCH_PIN) == HIGH)
    {
#ifdef EXTERNAL_FLASHER
#else
        // if it was on, but isn't, make sure we shut it off
        digitalWrite(RIGHT_BLINKER_LIGHT_PIN, LOW);
#endif
    }
#endif

    // toggle voltage led blink


    
}

void setup()
{
  pinMode(POWER_PWM_PIN, OUTPUT);
  pinMode(BRAKE_LIGHT_PIN, OUTPUT);
  pinMode(LEFT_BLINKER_LIGHT_PIN, OUTPUT);
  pinMode(RIGHT_BLINKER_LIGHT_PIN, OUTPUT);


  pinMode(BRAKE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(LEFT_BLINKER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BLINKER_SWITCH_PIN, INPUT_PULLUP);
  
  pinMode(13, OUTPUT);
  
  Serial.begin(9600);
  cli();
  
  // Set pwm for pins 5 & 6 to 7kHz
  // this is to control the output of the power pins
  // since we're fluctuatig between low and high power.
  // We use timer 1 (ISR) to mudulate at 60Hz
  TCCR0B = TCCR0B & 0b11111000 | 2;

  
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 50hz increments
  OCR1A = 312;// = (16*10^6) / (50*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();
  
  unsigned long startTime;
  int brakeTapCount = 0; // Count the number of brake taps while in programming mode
  unsigned long brakeTapStart = 0; // used for debouncing brake tap
  
  
  Serial.println("Starting programming modes");
  startTime = millis();
  
  unsigned long programmingTime = millis() - startTime;
  
  while (programmingTime < (unsigned long)PROGRAMMING_DELAY_MILLISECONDS*MILLIS_MULTIPLIER)
  {
      
      if ((digitalRead(BRAKE_SWITCH_PIN) == BRAKE_ON) && ((millis() - brakeTapStart) > (300 * MILLIS_MULTIPLIER)))
      {
        digitalWrite(BRAKE_LIGHT_PIN, HIGH);
        Serial.println("Tap Detected");
        brakeTapStart = millis();
        brakeTapCount++;
        // We could get stuck in here if the user never lets go of the brake
        while(digitalRead(BRAKE_SWITCH_PIN) == BRAKE_ON);
        digitalWrite(BRAKE_LIGHT_PIN, LOW);
        
      }
      programmingTime = millis() - startTime;

      
  }
  Serial.println("Ending programming mode");
  Serial.print("Programming taps: ");
  Serial.println(brakeTapCount);
  
  if(brakeTapCount == 4)
  {
    unsigned long startTime;
    unsigned long counter = 0;
    // emergency flasher mode
    while(digitalRead(BRAKE_SWITCH_PIN) == BRAKE_OFF)
    {
      startTime = millis()/MILLIS_MULTIPLIER;
      digitalWrite(BRAKE_LIGHT_PIN, HIGH);
      while(((millis()/MILLIS_MULTIPLIER)-startTime) < 900);
   
      startTime = millis()/MILLIS_MULTIPLIER;
      digitalWrite(BRAKE_LIGHT_PIN, LOW);
      while(((millis()/MILLIS_MULTIPLIER)-startTime) < 900);  
    }
  }
}

// This takes care of the light blinking when the brake is depressed
void lightBlink()
{

  int blinkPulses = 4;
  unsigned long startTime;
  Serial.println("starting blink sequence");
  
  // perform 8 short blinks
  for(int i = 0; i < 8; i++)
  {
    Serial.println("blink on");
    digitalWrite(BRAKE_LIGHT_PIN, HIGH);
    startTime = millis()/MILLIS_MULTIPLIER;
    while(((millis()/MILLIS_MULTIPLIER)-startTime) < SHORT_DELAY);
    
    
    digitalWrite(BRAKE_LIGHT_PIN, LOW);
    Serial.println("blink off");
    startTime = millis()/MILLIS_MULTIPLIER;
    while(((millis()/MILLIS_MULTIPLIER)-startTime) < SHORT_DELAY);
  }
  
  // long blink
  for(int i = 0; i < 3; i++)
  {
    // break out early if brake has been released.
    if(digitalRead(BRAKE_SWITCH_PIN) == BRAKE_OFF)
    {
      break;
    }
    Serial.println("blink on");
    digitalWrite(BRAKE_LIGHT_PIN, HIGH);
    startTime = millis()/MILLIS_MULTIPLIER;
    while(((millis()/MILLIS_MULTIPLIER)-startTime) < LONG_DELAY);
    
    // break out early if brake has been released.
    if(digitalRead(BRAKE_SWITCH_PIN) == BRAKE_OFF)
    {
      break;
    }
    
    digitalWrite(BRAKE_LIGHT_PIN, LOW);
    Serial.println("blink off");
    startTime = millis()/MILLIS_MULTIPLIER;
    while(((millis()/MILLIS_MULTIPLIER)-startTime) < LONG_DELAY);
  }
  
  Serial.println("solid on");  
  digitalWrite(BRAKE_LIGHT_PIN, HIGH);
  
  

}

void updateVoltageStatus()
{
    // read analog value

    // if > 15V flash red
    v_state = VOLTAGE_STATE_TOO_HIGH;

    // if 13.5 to 15, solid green
    v_state = VOLTAGE_STATE_CHARGING;

    // if 12.5 to 13.5 solid orange
    v_state = VOLTAGE_STATE_NON_CHARGING;

    // if < 12.2 blink orange
    v_state = VOLTAGE_STATE_TOO_LOW;


}

void doStuff()
{
    // Do other things
    updateVoltageStatus();
}

void loop()
{
    // 
    digitalWrite(BRAKE_LIGHT_PIN, LOW);
    delay(100);// debounce any previous release
    while (digitalRead(BRAKE_SWITCH_PIN) == BRAKE_OFF)
    {
        doStuff();
    }
    
    // when brake hit is detected, blink the light.
    lightBlink();
    
    // wait for the brake to be released
    while (digitalRead(BRAKE_SWITCH_PIN) == BRAKE_ON)
    {
        doStuff();
    }

    // Brake was released, 
    Serial.println("Brake release detected.");
    digitalWrite(BRAKE_LIGHT_PIN, LOW);

    
    
}
