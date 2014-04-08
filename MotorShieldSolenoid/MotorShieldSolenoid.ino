#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"
#include <Servo.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myStepper = AFMS.getStepper(200, 2);
// And connect DC motors to ports
Adafruit_DCMotor *myMotor1 = AFMS.getMotor(1);
Adafruit_DCMotor *myMotor2 = AFMS.getMotor(2);
Adafruit_DCMotor *myMotor3 = AFMS.getMotor(3);
Adafruit_DCMotor *myMotor4 = AFMS.getMotor(4);

int t_drop = 50;
int t_calib_1 = 0, t_calib_2 = 0;
int t_delay = 1000;
int t_enable = 0;

void runmotor(int motor) {
  if(motor == 1) {
    myMotor1->run(FORWARD);
    myMotor3->run(FORWARD);
  } else {
    myMotor2->run(FORWARD);
    myMotor4->run(FORWARD);
  }
}

void stopmotor(int motor) {
  if(motor == 1) {
    myMotor1->run(RELEASE);
    myMotor3->run(RELEASE);
  } else {
    myMotor2->run(RELEASE);
    myMotor4->run(RELEASE);
  }
}

void update() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Drop: ");
  lcd.print(t_drop);
  if(t_enable == 1) {
    lcd.print("+");
    lcd.print(t_calib_1);
  } else if(t_enable == 2) {
    lcd.print("+");
    lcd.print(t_calib_2); 
  }
  lcd.print(" msec");
  lcd.setCursor(0, 1);
  lcd.print("Rate: ");
  lcd.print(t_delay/500);
  lcd.print(".");
  lcd.print((t_delay/50)%10);
  lcd.print(" sec  ");
  lcd.print(t_enable);
}

void setup() {
  int i, test = 1;
  uint8_t button, button_last = 0;
  
  Serial.begin(9600);           // set up Serial library at 9600 bps
    
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);
  
  AFMS.begin();  // create with the default frequency 1.6KHz
   
  // turn on motor M1
  myMotor1->setSpeed(255);
  myMotor1->run(RELEASE);
  myMotor2->setSpeed(255);
  myMotor2->run(RELEASE);
  myMotor3->setSpeed(255);
  myMotor3->run(RELEASE);
  myMotor4->setSpeed(255);
  myMotor4->run(RELEASE);
  
  // wait for MAX chip to stabilize
  delay(500);

  if(lcd.readButtons() == BUTTON_SELECT) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibration");
    delay(2000);

    while(1) {
      while(1) {
        lcd.setCursor(0, 1);
        lcd.print("1=");
        lcd.print(t_calib_1);
        if(test == 1) lcd.print("*   ");
        else          lcd.print("    ");
        lcd.setCursor(8, 1);
        lcd.print("2=");
        lcd.print(t_calib_2);
        if(test == 2) lcd.print("*   ");
        else          lcd.print("    ");
          
        button = lcd.readButtons();
        if(button != button_last) {
          if(button == BUTTON_RIGHT) {
            if(test == 1) test = 2;
            else          test = 1;
          }
          if(button == BUTTON_LEFT) {
            for(i = 0; i < 100; i++) {
               runmotor(test);
               delay(50 + ((test == 1) ? t_calib_1 : t_calib_2)); 
               stopmotor(test);
               delay(150);
            }
          }
          if(button == BUTTON_UP) {
            if(test == 1) t_calib_1++;
            else          t_calib_2++;
          }
          if(button == BUTTON_DOWN)
            if(test == 1) t_calib_1--;
            else          t_calib_2--;
          if(button == BUTTON_SELECT)
            goto done;
        }
        button_last = button; 
        delay(100);
      }
    }
  }
  
done:
  update();
}

int timer = 0, motor = 0;
void loop() {
  uint8_t buttons = lcd.readButtons();
  static uint8_t buttons_last;
  
  if(buttons && (buttons != buttons_last)) {
    if(buttons & BUTTON_UP) {
      if((t_enable == 3) || (t_enable == 0))
        t_drop += 10;
      else if(t_enable == 1)
        t_calib_1++;
      else if(t_enable == 2)
        t_calib_2++;
    }
    if(buttons & BUTTON_DOWN) {
      if((t_enable == 3) || (t_enable == 0))
        t_drop -= 10;
      else if(t_enable == 1)
        t_calib_1--;
      else if(t_enable == 2)
        t_calib_2--;
    }
    if(buttons & BUTTON_RIGHT) {
      t_delay += 100;
    }
    if(buttons & BUTTON_LEFT) {
      t_delay -= 100; 
    }
    if(buttons & BUTTON_SELECT) {
      if(!t_enable) t_enable = 3;
      else          t_enable--;
      stopmotor(1);
      stopmotor(2);
    }
    update();
    timer = 0;
  }
  buttons_last = buttons;
  
  if(timer == 0) {
    if(!motor && (t_enable & 1)) {
      runmotor(1);
      delay(t_drop + t_calib_1);
      stopmotor(1);
      timer += t_drop + t_calib_1;
    } else if(motor && (t_enable & 2)) {
      runmotor(2);
      delay(t_drop + t_calib_2);
      stopmotor(2);
      timer += t_drop + t_calib_1;
    }
  }
  
  delay(8);
  timer += 10;
  
  if(timer >= t_delay) {
    timer = 0;
    motor++;
    motor &= 1;
  }
}
