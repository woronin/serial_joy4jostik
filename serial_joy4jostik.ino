//////////////////////////////////////////////
//     2017.12.29 woronin,  umkiedu@gmail.com     
//     Robot UMKI controller K6_2_mini
//     To connect using 4joyjostik mobile app by link http://arduino-robot.site/basic/serial                 
//     - for ANDROID 4.0.1 or later version; 
//////////////////////////////////////////////   
#include <SoftwareSerial.h>
 
SoftwareSerial BTSerial(6, 7); // RX, TX
int byte_forward[]={0, 0,   129, 0, 4, 0, 0};
int byte_bakward[]={0, 0,   127, 0, 4, 0, 0};
int byte_left[]=   {0, 129, 0,   0, 4, 0, 0};
int byte_right[]=  {0, 127, 0,   0, 4, 0, 0};
int byte_stop[]=   {0, 0,   0,   0, 4, 0, 0};
int byte_start[]=  {0, 0,   0,   0, 4, 0, 2};
int m1speed=3; // скорость левый
int m2speed=5; // скорость правый
int m1direction=2; // направление правый
int m2direction=4; // направление левый

int program_move[25], program_time[25], program_speed[25]; //инициализация трех массивов, направления, времени, скорости
int side, pwm=255,time1, time2, press_time;


void setup() {
  // инициализируем те порты,
  BTSerial.begin(9600);
  Serial.begin(19200);
}
void loop() // выполняется циклически записываем в порт по шнурку все полученны данные с блютуса
{

  int inByte[7], i, count; //i - это элемент массива команды из 7 байт
  static int n=0; //  номер команды движения из 25
  count = BTSerial.available();
  if (count<7) return;
        for (i = 0; i<7; i++) {
        inByte[i] = BTSerial.read();   
        delay(50);
//        Serial.print(inByte[i]); 
      } 
  if (inByte[1]==byte_forward[1] && inByte[2]==byte_forward[2]&& inByte[6]==byte_forward[6]) {
     go_forward(pwm);
     program_move[n]=0x2; //запоминеаем в программе шаг вперед
     time1 = millis();  // Запоминаем время нажатия кнопки
     n++;
  }
 
  else if (inByte[1]==byte_left[1] && inByte[2]==byte_left[2]&& inByte[6]==byte_left[6]) {
     go_left(pwm);
     program_move[n]=0x4; //запоминеаем в программе шаг влево
     time1 = millis();  // Запоминаем время нажатия кнопки     
     n++;   
      }

  else if (inByte[1]==byte_right[1] && inByte[2]==byte_right[2]&& inByte[6]==byte_right[6]) {
     go_right(pwm);
     program_move[n]=0x8; //запоминеаем в программе шаг вправо
     time1 = millis();  // Запоминаем время нажатия кнопки     
     n++;     
  }     

  else if (inByte[1]==byte_bakward[1] && inByte[2]==byte_bakward[2]&& inByte[6]==byte_bakward[6]) {
     go_back(pwm);
     program_move[n]=0xc; //запоминеаем в программе шаг назад
     time1 = millis();  // Запоминаем время нажатия кнопки     
     n++;
  }
  else if (inByte[1]==byte_stop[1] && inByte[2]==byte_stop[2]&& inByte[6]==byte_stop[6]) {
      time2 = millis();  // Запоминаем время отпускания кнопки
      if (n>0)
      program_time[n-1]=time2-time1; //записываем по индексу в массив время движения
      go_stop(pwm);
      inByte[1]=1;
   }  

  else if (inByte[1]==byte_start[1] &&inByte[2]==byte_start[2] && inByte[6]==byte_start[6]) {
      Serial.println(" 1111 ");   
      go_program (pwm); // запускаем езду по программе
      n=0;
  }     
  if (n>=25) n=0; // массив из 25 шагов команды, защита от переполнения

}

void go_forward(int pwm) // вперед поехали
{
digitalWrite(m1direction,LOW); // вперед правый
digitalWrite(m2direction,LOW); // вперед левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}

void go_back(int pwm)  // назад поехали
{
digitalWrite(m1direction,HIGH); // назад правый
digitalWrite(m2direction,HIGH); // назад левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}

void go_left(int pwm)  // влево поехали
{
digitalWrite(m1direction,LOW); // вперед правый
digitalWrite(m2direction,HIGH); // назад левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}

void go_right(int pwm) // вправо поехали
{
digitalWrite(m1direction,HIGH ); // назад правый
digitalWrite(m2direction,LOW); // вперед левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}

void go_stop(int pwm) // стоп
{
      analogWrite(m1speed, 0); // скорость стоп
      analogWrite(m2speed, 0);
}

void go_program(int pwm) // Подпрограмма езды по заданым шагам программы
{
  int n;
 for (n = 0; n< 25; n++) {
   Serial.print(n); 
   Serial.print(" np= ");    
   Serial.print(program_move[n]); 
      Serial.print(" tm= ");    
   Serial.println(program_time[n]); 
   if (program_move[n]==0x2) {
     go_forward(pwm);
     delay(program_time[n]);
     go_stop(pwm);
  }
  else if (program_move[n]==0x4) {
     go_left(pwm);
     delay(program_time[n]);
     go_stop(pwm);
  }
  else if (program_move[n]==0x8) {
     go_right(pwm);
     delay(program_time[n]);
     go_stop(pwm);
  }
  else if (program_move[n]==0xc) {
     go_back(pwm);
     delay(program_time[n]);
     go_stop(pwm);
  }

 }
}
