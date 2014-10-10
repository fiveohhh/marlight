
#define RIGHT_BLINKER_SWITCH_PIN 3
#define LEFT_BLINKER_SWITCH_PIN 4
#define BRAKE_SWITCH_PIN 8
#define BRAKE_LIGHT_PIN 13 // This pin used to turn on brake light
#define POWER_PWM_PIN 5 // PWM for light power source
#define LEFT_BLINKER_LIGHT_PIN 12 // This pin used to turn on brake light
#define RIGHT_BLINKER_LIGHT_PIN 11 // This pin used to turn on brake light

#define MILLIS_MULTIPLIER 7 // Since we changed PWM freq, millis now reports milliseconds *7

#define LOW_PWM 177
#define HIGH_PWM 255

#define SHORT_DELAY 60
#define LONG_DELAY 120
#define BLINKER_DELAY 400

#define BRAKE_ON 0
#define BRAKE_OFF 1

#define BLINKER_STATUS_OFF 0
#define BLINKER_STATUS_LEFT 1
#define BLINKER_STATUS_RIGHT 2

#define PROGRAMMING_DELAY_MILLISECONDS 5000

volatile int brakeSwitchStatus = 0; // 

ISR(TIMER1_COMPA_vect){  //change the 0 to 1 for timer1 and 2 for timer2
    // used to toggle power to all pins.
   
    // 0 is off
    static int blinkStatus = BLINKER_STATUS_OFF;
    static unsigned long blinkerStartTime;

    // This controls the MOSFET that gives our lights a 60Hz flicker
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
   
    // handle left blinker stuffs
    if (digitalRead(LEFT_BLINKER_SWITCH_PIN) == LOW)
    {
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
    }
    else if (digitalRead(LEFT_BLINKER_SWITCH_PIN) == HIGH)
    {
        // if it was on, but isn't, make sure we shut it off
        digitalWrite(LEFT_BLINKER_LIGHT_PIN, LOW);
    }

    // handle right blinker
    if (digitalRead(RIGHT_BLINKER_SWITCH_PIN) == LOW)
    {
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
    }
    else if (digitalRead(RIGHT_BLINKER_SWITCH_PIN) == HIGH)
    {
        // if it was on, but isn't, make sure we shut it off
        digitalWrite(RIGHT_BLINKER_LIGHT_PIN, LOW);
    }


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
  
  brakeSwitchStatus = digitalRead(BRAKE_SWITCH_PIN);
  Serial.begin(9600);
  cli();
  
  // Set pwm for pins 5 & 6 to 7kHz
  TCCR0B = TCCR0B & 0b11111000 | 2;
  
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 50hz increments
  OCR1A = 312;// = (16*10^6) / (60*1024) - 1 (must be <65536)
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

void lightBlink()
{

  int blinkPulses = 4;
  unsigned long startTime;
  Serial.println("starting blink sequence");
  
  // short blink
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
    if(digitalRead(BRAKE_SWITCH_PIN) == BRAKE_OFF)
    {
      break;
    }
    Serial.println("blink on");
    digitalWrite(BRAKE_LIGHT_PIN, HIGH);
    startTime = millis()/MILLIS_MULTIPLIER;
    while(((millis()/MILLIS_MULTIPLIER)-startTime) < LONG_DELAY);
    
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
  while(digitalRead(BRAKE_SWITCH_PIN) == BRAKE_ON);
  
  Serial.println("Exit brake function");  
  digitalWrite(BRAKE_LIGHT_PIN, LOW);

}

void loop()
{
    // 
    digitalWrite(BRAKE_LIGHT_PIN, LOW);
    delay(100);// debounce any previous release
    while(digitalRead(BRAKE_SWITCH_PIN) == BRAKE_OFF);
    lightBlink();
}
