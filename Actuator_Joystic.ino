#include "OneButton.h"
#include <EEPROM.h>

bool debug_mode = 0;      //Режим отладки 1 - включен, 0 - выключен
unsigned int delay_time;  //Частота обработки сигналов, для режима отладки низкая, для рабочего режима высокая

unsigned long last_time;

bool isSavedPosition = 0;
bool on = 1;                         //on - высокий уровень
bool off = 0;                        //off - низкий уровень
#define open_saved_pos_switch_pin 2  //Переключатель open/saved position 0 - saved, 1 - open

//x-actuator
#define x_pin A0                          //Потенциометр джойстика
#define x_actuator_current_signal_pin A1  //Текущее положение актуатора
byte x_up_pin = 10;                       //Motor direction to push(open) L_PWM
byte x_down_pin = 11;                     //Motor direction to pull(close) R_PWM
byte x_enable_motor_pin = 12;             //Motor on/off

int x_actuator_current_signal;
int x_actuator_saved_signal = 500;

//y-actuator
#define y_pin A2  //Потенциометр джойстика
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
  pinMode(x_pin, INPUT);
  pinMode(x_actuator_current_signal_pin, INPUT);
  pinMode(x_up_pin, OUTPUT);
  pinMode(x_down_pin, OUTPUT);
  pinMode(x_enable_motor_pin, OUTPUT);
  digitalWrite(x_up_pin, off);
  digitalWrite(x_down_pin, off);
  digitalWrite(x_enable_motor_pin, off);

  pinMode(x_pin, INPUT);
  pinMode(y_pin, INPUT);
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

    if (analogRead(x_pin) > 600)
      clockwise_rotation_motor(x_up_pin, x_down_pin, x_enable_motor_pin);
    else if (analogRead(x_pin) < 400)
      counterclockwise_rotation_motor(x_up_pin, x_down_pin, x_enable_motor_pin);
    else
      stop_rotation_motor(x_up_pin, x_down_pin, x_enable_motor_pin);
  }

  if (debug_mode) {
    if (millis() - last_time > 1000) {
      last_time = millis();
      Serial.print("Curent position x-actuator: ");
      Serial.print(analogRead(x_actuator_current_signal_pin));
      Serial.print("\tJoystic position ");
      Serial.println(analogRead(x_pin));
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
      counterclockwise_rotation_motor(x_up_pin, x_down_pin, x_enable_motor_pin);
    else if (x_actuator_current_signal < (saved_data.x_actuator_saved_pos))
      clockwise_rotation_motor(x_up_pin, x_down_pin, x_enable_motor_pin);
    else {
      stop_rotation_motor(x_up_pin, x_down_pin, x_enable_motor_pin);
      isSavedPosition = 1;
    }

    //y-actuator

    //Выход из цикла
    //Если переключились в open position
    if (!digitalRead(open_saved_pos_switch_pin)) { break; }

    //Если тронули управления джойстиком
    if (analogRead(x_pin) > 600 || analogRead(x_pin) < 400) {
      isSavedPosition = 1;
      break;
    }

    // if (analogRead(y_pin) > 600 || analogRead(y_pin) < 400) {
    //   isSavedPosition = 1;
    //   break;
    // }
  }
}

//Кнопка нажата
void go_to_open_pos() {
  //x-actuator
  counterclockwise_rotation_motor(x_up_pin, x_down_pin, x_enable_motor_pin);

  //y-actuator

  isSavedPosition = 0;
}

void clockwise_rotation_motor(byte up_pin, byte down_pin, byte motor_pin) {
  digitalWrite(up_pin, off);
  digitalWrite(down_pin, on);
  digitalWrite(motor_pin, on);
}

void counterclockwise_rotation_motor(byte up_pin, byte down_pin, byte motor_pin) {
  digitalWrite(up_pin, on);
  digitalWrite(down_pin, off);
  digitalWrite(motor_pin, on);
}

void stop_rotation_motor(byte up_pin, byte down_pin, byte motor_pin) {
  digitalWrite(up_pin, off);
  digitalWrite(down_pin, off);
  digitalWrite(motor_pin, off);
}

void OneClick() {
  Serial.println("----------Короткое нажатие----------");
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
