#include "OneButton.h"
#include <EEPROM.h>

bool debug_mode = 0;      //Режим отладки 1 - включен, 0 - выключен

unsigned long last_time;

bool isSavedPosition = 0;
bool on = 1;                         //on - высокий уровень
bool off = 0;                        //off - низкий уровень
#define open_saved_pos_switch_pin 2  //Переключатель open/saved position 1 - saved, 0 - open

//x-actuator
#define x_joystic_pin A0                  //Положение джойстика, вход с потенициометра джойстика
#define x_actuator_current_signal_pin A1  //Текущее положение актуатора, вход с потенциометра актуатора
byte x_L_PWM_pin = 10;                    //Motor rotation counterclockwise L_PWM
byte x_R_PWM_pin = 11;                    //Motor rotation clockwise R_PWM
byte x_enable_motor_pin = 12;             //Motor on/off

int x_actuator_current_signal;
int x_actuator_saved_signal = 500;

//y-actuator
#define y_joystic_pin A2  //Потенциометр джойстика
// #define back_up 7                        //Motor direction to push(open)
// #define back_down 8                      //Motor direction to pull(close)
// #define back_motor 9                     //Motor on/off

#define joystic_butt 3
OneButton Joystic_button(joystic_butt, true);

struct EepromData {
  int x_actuator_saved_pos = 0;
  int y_actuator_saved_pos = 0;
};

EepromData saved_data;

//--------------------------------------------------------setup--------------------------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(open_saved_pos_switch_pin, INPUT_PULLUP);

  //x-actuator setup
  pinMode(x_joystic_pin, INPUT);
  pinMode(x_actuator_current_signal_pin, INPUT);
  pinMode(x_L_PWM_pin, OUTPUT);
  pinMode(x_R_PWM_pin, OUTPUT);
  pinMode(x_enable_motor_pin, OUTPUT);

  pinMode(x_joystic_pin, INPUT);
  pinMode(y_joystic_pin, INPUT);
  pinMode(joystic_butt, INPUT_PULLUP);

  Joystic_button.attachClick(OneClick);
  Joystic_button.attachDoubleClick(DoubleClick);
}

//--------------------------------------------------------loop--------------------------------------------------------
void loop() {
  Joystic_button.tick();

  if (!digitalRead(open_saved_pos_switch_pin)) {
    go_to_open_pos();
  } else {
    if (!isSavedPosition) { go_to_saved_pos(); }

    if (analogRead(x_joystic_pin) > 600)
      right_rotation_motor(x_L_PWM_pin, x_R_PWM_pin, x_enable_motor_pin);
    else if (analogRead(x_joystic_pin) < 400)
      left_rotation_motor(x_L_PWM_pin, x_R_PWM_pin, x_enable_motor_pin);
    else
      stop_rotation_motor(x_L_PWM_pin, x_R_PWM_pin, x_enable_motor_pin);
  }

  if (debug_mode) {
    if (millis() - last_time > 1000) {
      last_time = millis();
      Serial.print("Curent position x-actuator: ");
      Serial.print(analogRead(x_actuator_current_signal_pin));
      Serial.print("\tJoystic position ");
      Serial.println(analogRead(x_joystic_pin));
    }
  }
}

//Кнопка отпущена
void go_to_saved_pos() {
  EEPROM.get(0, saved_data);  // прочитать из адреса 0

  while (!isSavedPosition) {
    //x-actuator
    x_actuator_current_signal = analogRead(x_actuator_current_signal_pin);

    if (x_actuator_current_signal > (saved_data.x_actuator_saved_pos))
      left_rotation_motor(x_L_PWM_pin, x_R_PWM_pin, x_enable_motor_pin);
    else if (x_actuator_current_signal < (saved_data.x_actuator_saved_pos))
      right_rotation_motor(x_L_PWM_pin, x_R_PWM_pin, x_enable_motor_pin);
    else {
      stop_rotation_motor(x_L_PWM_pin, x_R_PWM_pin, x_enable_motor_pin);
      isSavedPosition = 1;
    }

    //y-actuator

    //Выход из цикла
    //Если переключились в open position
    if (!digitalRead(open_saved_pos_switch_pin)) { break; }

    //Если тронули управления джойстиком
    if (analogRead(x_joystic_pin) > 600 || analogRead(x_joystic_pin) < 400) {
      isSavedPosition = 1;
      break;
    }

    // if (analogRead(y_joystic_pin) > 600 || analogRead(y_joystic_pin) < 400) {
    //   isSavedPosition = 1;
    //   break;
    // }
  }
}

//Кнопка нажата
void go_to_open_pos() {
  //x-actuator
  left_rotation_motor(x_L_PWM_pin, x_R_PWM_pin, x_enable_motor_pin);

  //y-actuator

  isSavedPosition = 0;
}

void right_rotation_motor(byte L_PWM, byte R_PWM, byte LR_Enable) {
  digitalWrite(L_PWM, off);
  digitalWrite(R_PWM, on);
  digitalWrite(LR_Enable, on);
}

void left_rotation_motor(byte L_PWM, byte R_PWM, byte LR_Enable) {
  digitalWrite(L_PWM, on);
  digitalWrite(R_PWM, off);
  digitalWrite(LR_Enable, on);
}

void stop_rotation_motor(byte L_PWM, byte R_PWM, byte LR_Enable) {
  digitalWrite(L_PWM, off);
  digitalWrite(R_PWM, off);
  digitalWrite(LR_Enable, off);
}

void OneClick() {
  go_to_open_pos();
  delay(10);
}

void DoubleClick() {
  save_new_pos();
  delay(10);
}

void save_new_pos() {
  saved_data.x_actuator_saved_pos = analogRead(x_actuator_current_signal_pin);

  // saved_data.y_actuator_saved_pos = analogRead(y_actuator_current_signal_pin);
  saved_data.y_actuator_saved_pos = 0;  //удалить

  EEPROM.put(0, saved_data);  // поместить структуру в EEPROM по адресу 0
}
