//////////////////////////////////////////////
//     2017.12.24 woronin,  umkiedu@gmail.com     
//     Robot UMKI controller K6_2_mini
//     To connect using 4joyjostik mobile app by link http://arduino-robot.site/basic/serial                 
//     - for ANDROID 4.0.1 or later version; 
//////////////////////////////////////////////   
#include <SoftwareSerial.h>
 
SoftwareSerial BTSerial(6, 7); // RX, TX
int byte_forward[]={0, 0, 129, 0, 4, 0, 0};
int byte_bakward[]={0, 0, 127, 0, 4, 0, 0};
int byte_left[]=   {0, 129, 0, 0, 4, 0, 0};
int byte_right[]=  {0, 127, 0, 0, 4, 0, 0};
int byte_stop[]=   {0, 0, 0, 0, 4, 0, 0};
int m1speed=3; // скорость левый
int m2speed=5; // скорость правый
int m1direction=2; // направление правый
int m2direction=4; // направление левый

int side, pwm=255;


void setup() {
  // инициализируем те порты,
  BTSerial.begin(9600);
  Serial.begin(19200);

}
void loop() // выполняется циклически записываем в порт по шнурку все полученны данные с блютуса
{

  int inByte[7], i;
  if (BTSerial.available()){
     if (BTSerial.read()==0){
      for (i = 0; i< 7; i++) {
        inByte[i] = BTSerial.read();   
        Serial.print(inByte[i]); 
        Serial.print(i); 
      } 
   
  if (inByte[1]==byte_forward[1] && inByte[2]==byte_forward[2]) {
     Serial.println(" 8888 ");
     go_forward(pwm);
  }
  if (inByte[1]==byte_bakward[1] && inByte[2]==byte_bakward[2]) {
     Serial.println(" 9999 ");
     go_back(pwm);
  }
  if (inByte[1]==byte_left[1] && inByte[2]==byte_left[2]) {
     Serial.println(" 7777 ");
     go_left(pwm);
  }
  if (inByte[1]==byte_right[1] && inByte[2]==byte_right[2]) {
     Serial.println(" 6666 ");
      go_right(pwm);
  }     
  if (inByte[1]==byte_stop[1] && inByte[2]==byte_stop[2]) {
      Serial.println(" 0000 ");   
      analogWrite(m1speed, 0); // скорость стоп
      analogWrite(m2speed, 0);
  }
             }
  }



}

void go_forward(int pwm)
{
digitalWrite(m1direction,LOW); // вперед правый
digitalWrite(m2direction,LOW); // вперед левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}

void go_back(int pwm)
{
digitalWrite(m1direction,HIGH); // назад правый
digitalWrite(m2direction,HIGH); // назад левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}

void go_left(int pwm)
{
digitalWrite(m1direction,LOW); // вперед правый
digitalWrite(m2direction,HIGH); // назад левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}

void go_right(int pwm)
{
digitalWrite(m1direction,HIGH ); // назад правый
digitalWrite(m2direction,LOW); // вперед левый
analogWrite(m1speed, pwm); // скорость
analogWrite(m2speed, pwm);
}
