//////////////////////////////////////////////
//     2018.05.01 woronin,  umkiedu@gmail.com
//     Robot UMKI controller  K6_mini
//     To connect using 4joyjostik mobile app by link http://arduino-robot.site/basic/serial
//     - for ANDROID 4.0.1 or later version;
//////////////////////////////////////////////
#include <SoftwareSerial.h>
#define diod_led 13 // Порт для диодика

SoftwareSerial BTSerial(6, 7); // RX, TX
int byte_forward[] = {0, 0,   129, 0, 4, 0, 0};
int byte_bakward[] = {0, 0,   127, 0, 4, 0, 0};
int byte_left[]    = {0, 129, 0,   0, 4, 0, 0};
int byte_right[]   = {0, 127, 0,   0, 4, 0, 0};
int byte_stop[]    = {0, 0,   0,   0, 4, 0, 0};
int byte_start[]   = {0, 0,   0,   0, 4, 0, 2};
int byte_a[]       = {0, 0,   0,   0, 4, 1, 0}; // A
int byte_b[]       = {0, 0,   0,   0, 4, 2, 0}; // B
int byte_c[]       = {0, 0,   0,   0, 4, 4, 0}; // C
int byte_x[]       = {0, 0,   0,   0, 4, 8, 0}; // X
int byte_y[]       = {0, 0,   0,   0, 4, 16, 0};// Y
int byte_z[]       = {0, 0,   0,   0, 4, 32, 0};// Z
int ml_speed = 3; // скорость моторов
int mr_speed = 3; // скорость моторов только для К6mini

int motor_r1 = 2; // направление правый
int motor_l1 = 4; // направление левый
int speaker = 5; // ножка спикера

int program_move[50], program_time[50], program_speed[50]; //инициализация трех массивов, направления, времени, скорости
int side, pwm = 255, press_time, time1, time2, press_but = 0, press_last = 0, flag_last_but = 0, flag_start_program = 0;
int flag_source = 0; // флаг источника данных команд 0- ничего , 1 - блютус, 2 - снап
int timeX = 150; // время в милисек для кнопки X
int timeY = 300; // время в милисек для кнопки Y
int n = 0; //  номер команды движения из 50
int count_snap_cikl = 0 ; // счетчик циклов в коде от снапа,задается в подпрограмме снапа
int inByte[64];
int inByte_count = 0; // Число прочитанных байтов в массив inbyte
int check_cmd(void); //  функция для проверки коректности принятых байт

void setup() {
  // инициализируем те порты,
  BTSerial.begin(9600);
  Serial.begin(19200);
  Serial.println("88888"); //  отладка поиска старта программы
}



void loop() // выполняется циклически записываем в порт по шнурку все полученны данные с блютуса
{
  int  i, count;
  static int count_snap = 0; //  длина в байтах команды от снапа
  static int flag_snap = 0; // только для снапа, когда 0 - ожидает конца пакета, 1 - получен последний байт пакета
  // todo  сделать защиту от переполнения массива inByte[]
  // todo  сдедать таймаут для ожидания 0x8f

  // читаем из блютус порта 7 байт
  count = BTSerial.available();
  if (count < 1) return;

  switch (flag_source) {
    case 0: //  определяем по первому байту откуда пришли байты в порт - либо с моб, либо от снап
      inByte[0] = BTSerial.read(); //  прочитали первый байт
      inByte_count = 1;
      if (inByte[0] == 0) flag_source = 1; // идут команды с мобилки пульта блютус
      else if (inByte[0] == 0x8e) {
        flag_source = 2; //  идет программа снапа
        count_snap = 1; // первый байт мы уже прочитали
        flag_snap = 0; // признак принятия пакета
      }
      else flag_source = 0; // чтобы при потеряных байтах возвращалась сюда же

      return;
      break;

    case 1: //  здесь идут 7 байт с мобильника. Один уже прочтен ранее
      if (inByte_count + count > 7) {
        count = 7 - inByte_count; // обрабатываем только одну команду за раз

      }

      for (i = 0; i < count; i++) {
        inByte[inByte_count++] = BTSerial.read();
        //        delay(10);
        //        Serial.print(inByte[i+1], HEX); // вывод в COM порт побайтоно в шестнадцатиричной системе
        //        Serial.print(" "); // ставим пробел между байтаами, чтобы удобно было смотреть монитор порта
      }
      if (check_cmd() == 0) {  //  пришла битая команда
M:
        for (i = 0; i < inByte_count - 1; i++) // сдвигаем элементы массива
        {
          inByte[i] = inByte[i + 1];
        }
        inByte_count--;
        if (inByte_count < 1)
        {
          flag_source = 0;
          return;
        }
        if (check_cmd() == 0)
          goto M;

      }
      if (inByte_count < 7) return;
      break;

    case 2: //  здесь мы читаем байты от програмы SNAP
      for (i = 0; i < count; i++) {
        inByte[count_snap] = BTSerial.read();
        Serial.print(inByte[i + 1], HEX); // вывод в COM порт побайтоно в шестнадцатиричной системе
        Serial.print(" "); // ставим пробел между байтаами, чтобы удобно было смотреть монитор порта
        if (inByte[count_snap] == 0x8f) {
          count_snap++;
          flag_snap = 1;

          break;
        }
        delay(10);
        count_snap++; // увеличиывем счетчик для массива исполняемой программы из Снапа
      }
      if (flag_snap == 0) return;
      break;
  }
  Serial.println(); // чтоб была видна каждая команда

  // начало разбор принятых байт
  if (flag_source == 1) { //  функция  обработки по пультику

    if ((inByte[1] == byte_forward[1]) && (inByte[2] == byte_forward[2]) && (inByte[5] == byte_forward[5]) && (inByte[6] == byte_forward[6])) {
      press_but = 48; // нажата кнопка вперед
    }
    else if ((inByte[1] == byte_left[1]) && (inByte[2] == byte_left[2])  && (inByte[5] == byte_left[5]) && (inByte[6] == byte_left[6])) {
      press_but = 52; // нажата кнопка влево
    }
    else if ((inByte[1] == byte_right[1]) && (inByte[2] == byte_right[2])  && (inByte[5] == byte_right[5]) && (inByte[6] == byte_right[6])) {
      press_but = 56; // нажата кнопка вправо
    }
    else if ((inByte[1] == byte_bakward[1]) && (inByte[2] == byte_bakward[2])  && (inByte[5] == byte_bakward[5]) && (inByte[6] == byte_bakward[6])) {
      press_but = 60; // нажата кнопка назад
    }
    else if ((inByte[1] == byte_stop[1]) && (inByte[2] == byte_stop[2]) && (inByte[5] == byte_stop[5]) && (inByte[6] == byte_stop[6])) {
      press_but = 65; // нажата кнопка стоп
    }
    else if ((inByte[1] == byte_start[1]) && (inByte[2] == byte_start[2]) && (inByte[5] == byte_start[5]) && (inByte[6] == byte_start[6])) {
      press_but = 66; // нажата кнопка старт
    }
    // Разбор нажатия дополнительных кнопок
    else if ((inByte[1] == byte_a[1]) && (inByte[2] == byte_a[2]) && (inByte[4] == byte_a[4]) && (inByte[5] == byte_a[5])) {
      press_but = 71; // нажата кнопка A
    }
    else  if ((inByte[1] == byte_b[1]) && (inByte[2] == byte_b[2]) && (inByte[4] == byte_b[4]) && (inByte[5] == byte_b[5])) {
      press_but = 72; // нажата кнопка B
    }
    else  if ((inByte[1] == byte_c[1]) && (inByte[2] == byte_c[2]) && (inByte[4] == byte_c[4]) && (inByte[5] == byte_c[5])) {
      press_but = 74; // нажата кнопка C
    }
    else if ((inByte[1] == byte_x[1]) && (inByte[2] == byte_x[2]) && (inByte[4] == byte_x[4]) && (inByte[5] == byte_x[5])) {
      press_but = 78; // нажата кнопка X
    }
    else if ((inByte[1] == byte_y[1]) && (inByte[2] == byte_y[2]) && (inByte[4] == byte_y[4]) && (inByte[5] == byte_y[5])) {
      press_but = 80; // нажата кнопка Y
    }
    else if ((inByte[1] == byte_z[1]) && (inByte[2] == byte_z[2]) && (inByte[4] == byte_z[4]) && (inByte[5] == byte_z[5])) {
      press_but = 90; // нажата кнопка Z
    }
    // конец разбор принятых байт

    if (n >= 25) n = 25; // массив из 25 шагов команды, защита от переполнения

    // обработка нажатия кнопки
    if ( press_but > 67) { // если нажата кнопка A B C X Y Z то
      flag_source = 0; //  опускаем флаг источника данных
      press_last = press_but; // запоминаем последнюю нажатую кнопку
      return; // уходим на следующий цикл
    }
    // Разбор кнопок движения +++++++++++++++++++++++++++++++++++
    // Движение вперед
    if ( press_but == 48) {
      n++; // для программы увеличиваем на единицу индекс массива
      time1 = millis();  // Запоминаем время нажатия кнопки
      flag_last_but = 1; // поднимаем флаг нажатой кнопки   для подсчета времени
      if (flag_start_program == 1)  {
        memset (program_move, 0, sizeof(int) * 25); //для первой команды  после программы обнуляем массив
        n = 1; // ставим индексы всех массивов в 0
        flag_start_program = 0; // опускаем флаг старта программы
      }
      if (press_last == 78) {
        go_forward_x(pwm);
        program_move[n] = 0x0; //запоминаем в программе шаг вперед на времяX
      }
      else if (press_last == 80) {
        go_forward_y(pwm);
        program_move[n] = 0x2; //запоминаем в программе шаг впере на время Y
      }
      else
      {
        go_forward_z(pwm);
        program_move[n] = 0x1; //запоминаем в программе шаг вперед на время Z  по стопу
      }
    }

    // Движение влево ++++++++++++++++++++++++++++
    if ( press_but == 52) {
      n++; // для программы увеличиваем на единицу индекс массива
      time1 = millis();  // Запоминаем время нажатия кнопки
      flag_last_but = 1; // поднимаем флаг нажатой кнопки   для подсчета времени
      if (flag_start_program == 1)  {
        memset (program_move, 0, sizeof(int) * 25); //для первой команды  после программы обнуляем массив
        n = 1; // ставим индексы всех массивов в 0
        flag_start_program = 0; // опускаем флаг старта программы
      }

      if (press_last == 78) {
        go_left_x(pwm);
        program_move[n] = 0x4; //запоминаем в программе шаг влево на времяX
      }
      else if (press_last == 80) {
        go_left_y(pwm);
        program_move[n] = 0x6; //запоминаем в программе шаг влево на время Y
      }
      else
      {
        go_left_z(pwm);
        program_move[n] = 0x5; //запоминаем в программе шаг влево на время Z  по стопу
      }
    }

    // Движение вправо ++++++++++++++++++++++++++++++
    if ( press_but == 56) {
      n++; // для программы увеличиваем на единицу индекс массива
      time1 = millis();  // Запоминаем время нажатия кнопки
      flag_last_but = 1; // поднимаем флаг нажатой кнопки   для подсчета времени
      if (flag_start_program == 1)  {
        memset (program_move, 0, sizeof(int) * 25); //для первой команды  после программы обнуляем массив
        n = 1; // ставим индексы всех массивов в 0
        flag_start_program = 0; // опускаем флаг старта программы
      }

      if (press_last == 78) {
        go_right_x(pwm);
        program_move[n] = 0x8; //запоминаем в программе шаг вправо на времяX
      }
      else if (press_last == 80) {
        go_right_y(pwm);
        program_move[n] = 0xA; //запоминаем в программе шаг вправо на время Y
      }
      else
      {
        go_right_z(pwm);
        program_move[n] = 0x9; //запоминаем в программе шаг вправо на время Z  по стопу
      }
    }

    // Движение назад +++++++++++++++++++++++++++++++++++++
    if ( press_but == 60) {
      n++; // для программы увеличиваем на единицу индекс массива
      time1 = millis();  // Запоминаем время нажатия кнопки
      flag_last_but = 1; // поднимаем флаг нажатой кнопки   для подсчета времени
      if (flag_start_program == 1)  {
        memset (program_move, 0, sizeof(int) * 25); //для первой команды  после программы обнуляем массив
        n = 1; // ставим индексы всех массивов в 0
        flag_start_program = 0; // опускаем флаг старта программы
      }

      if (press_last == 78) {
        go_bakward_x(pwm);
        program_move[n] = 0xC; //запоминаем в программе шаг назад на времяX
      }
      else if (press_last == 80) {
        go_bakward_y(pwm);
        program_move[n] = 0xE; //запоминаем в программе шаг назад на время Y
      }
      else
      {
        go_bakward_z(pwm);
        program_move[n] = 0xD; //запоминаем в программе шаг назад на время Z  по стопу
      }
    }

    // Движение стоп ++++++++++++++++++++++++++++++
    if ( press_but == 65) {
      time2 = millis();  // Запоминаем время нажатия кнопки
      if (n > 0) {
        if (flag_last_but == 1)
          program_time[n] = time2 - time1; //записываем по индексу в массив время движения
        flag_last_but = 0; // опускаем флаг нажатой кнопки
      }
      go_stop(pwm);
    }

    // Движение по кнопке программа +++++++++++++++++++
    if ( press_but == 66) {
      if (press_last == 71)go_program_a (pwm, n); // запускаем езду по командам программе в прямой последовательности
      if (press_last == 72)go_program_b (pwm, n); // запускаем езду по командам программе в обратной последовательности
      flag_start_program = 1; // поднимаем флаг старта программы
    }
    flag_source = 0; //  опускаем флаг источника данных
  } //  endif (flag_source == 1)

  if (flag_source == 2) { // обрабатываем массив принятых данных от СНАПА ++++++++++++++++
    for (i = 0; i < count_snap - 2; i++) {
      if (inByte[i] == 0x03 ) {
        count_snap_cikl = inByte[i + 1]; // подсчитываем количество повторов цикла в коде
      }
      if (count_snap_cikl == 0) count_snap_cikl = 1; //Задаем один проход если небыло циклов в программе снапа
    }
    for (i = 1; i <= count_snap - 2; i++) {
      program_move[i - 1] = inByte[i]; // переносим принятый массив байт в массив для исполнения
    }
    for (i = 1; i <= count_snap_cikl; i++) { //  исполняем программу в цикле с учетом повторов
      memset (program_time, 0, sizeof(int) * 25); //обнуляем массив времен
      go_program_a (pwm, count_snap - 2); // запускаем езду по командам программе в прямой последовательности
    }
    flag_source = 0; //  опускаем флаг источника данных
    count_snap_cikl = 0; // обнуляем счетчик циклов
  }//  endif (flag_source == 2)
}//все, конец рабочего цикла


////////// подпрограммы ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void go_forward_x(int pwm) // вперед поехали после нажатой X
{
  digitalWrite(motor_r1, LOW); // вперед правый
  digitalWrite(motor_l1, LOW); // вперед левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeX);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

void go_forward_y(int pwm)  // вперед поехали после нажатой Y
{
  digitalWrite(motor_r1, LOW); // вперед правый
  digitalWrite(motor_l1, LOW); // вперед левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeY);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

void go_forward_z(int pwm)  // вперед поехали после нажатой Z
{
  digitalWrite(motor_r1, LOW); // вперед правый
  digitalWrite(motor_l1, LOW); // вперед левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
}

void go_back(int pwm)  // назад поехали
{
  digitalWrite(motor_r1, HIGH); // назад правый
  digitalWrite(motor_l1, HIGH); // назад левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
}

void go_bakward_x(int pwm) // назад поехали после нажатой X
{
  digitalWrite(motor_r1, HIGH); // назад правый
  digitalWrite(motor_l1, HIGH); // назад левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeX);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

void go_bakward_y(int pwm)  // назад поехали после нажатой Y
{
  digitalWrite(motor_r1, HIGH); // назад правый
  digitalWrite(motor_l1, HIGH); // назад левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeY);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

void go_bakward_z(int pwm)  // назад поехали после нажатой Z
{
  digitalWrite(motor_r1, HIGH); // назад правый
  digitalWrite(motor_l1, HIGH); // назад левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
}

void go_left_x(int pwm)  // влево поехали после нажатой X
{
  digitalWrite(motor_r1, LOW); // вперед правый
  digitalWrite(motor_l1, HIGH); // назад левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeX);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

void go_left_y(int pwm)  // влево поехали после нажатой Y
{
  digitalWrite(motor_r1, LOW); // вперед правый
  digitalWrite(motor_l1, HIGH); // назад левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeY);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

void go_left_z(int pwm)  // влево поехали после нажатой Z
{
  digitalWrite(motor_r1, LOW); // вперед правый
  digitalWrite(motor_l1, HIGH); // назад левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
}

void go_right_x(int pwm) // вправо поехали после нажатой X
{
  digitalWrite(motor_r1, HIGH ); // назад правый
  digitalWrite(motor_l1, LOW); // вперед левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeX);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

void go_right_y(int pwm) // вправо поехали после нажатой Y
{

  digitalWrite(motor_r1, HIGH ); // назад правый
  digitalWrite(motor_l1, LOW); // вперед левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);
  delay(timeY);
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);



}

void go_right_z(int pwm) // вправо поехали после нажатой Z
{

  digitalWrite(motor_r1, HIGH ); // назад правый
  digitalWrite(motor_l1, LOW); // вперед левый
  analogWrite(ml_speed, pwm); // скорость
  analogWrite(mr_speed, pwm);

}

void go_speaker(int pwm) // Писк спикера
{
  analogWrite(speaker, pwm - 1); // вкл спикер не знаю почему но так включается
  delay (500);
  analogWrite(speaker, pwm); // выкл спикер
}

void go_blink(int pwm) // моргание диодика
{
  digitalWrite(diod_led, HIGH); // вкл диодик
  delay (1500);
  digitalWrite(diod_led, LOW); // выкл диодик
}

void go_stop(int pwm) // стоп
{
  analogWrite(ml_speed, 0); // скорость стоп
  analogWrite(mr_speed, 0);
}

/////////////////// Подпрограммы езды по ранее введенным командам +++++++++++++++++++++++++++++++++++++++++++++++++++++++
void go_program_a(int pwm, int n_con) // Подпрограмма езды по заданым шагам программы после А
{
  int n;
  for (n = 0; n < n_con; n++) {

    Serial.print(n);
    Serial.print(" np= ");
    Serial.print(program_move[n], HEX);
    Serial.print(" tm= ");
    Serial.println(program_time[n]);

    if     (program_move[n] == 0x0)  { // вперед при нажатой X; T min
      go_forward_x(pwm);
    }
    else if (program_move[n] == 0x2) { // вперед при нажатой Y; T ave
      go_forward_y(pwm);
    }
    else if (program_move[n] == 0x1) { // вперед при нажатой Z; T max
      go_forward_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    else if (program_move[n] == 0x4) {// влево при нажатой X; T min
      go_left_x(pwm);
    }
    else if (program_move[n] == 0x6) {// влево при нажатой Y; T ave
      go_left_y(pwm);
    }
    else if (program_move[n] == 0x5) {// влево при нажатой Z; T max
      go_left_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    else if (program_move[n] == 0x7) {// звук спикера
      go_speaker(pwm);
    }

    else if (program_move[n] == 0x8) {// вправо при нажатой X; T min
      go_right_x(pwm);
    }

    else if (program_move[n] == 0xA) {// вправо при нажатой Y; T ave
      go_right_y(pwm);
    }
    else if (program_move[n] == 0x9) {// вправо при нажатой Z; T max
      go_right_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    else if (program_move[n] == 0xC) {// назад при нажатой X; T min
      go_bakward_x(pwm);
    }
    else if (program_move[n] == 0xE) {// назад при нажатой Y; T ave
      go_bakward_y(pwm);
    }
    else if (program_move[n] == 0xD) {// назад при нажатой Z; T max
      go_bakward_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    else if (program_move[n] == 0xF) {// Моргнуть диодиком
      go_blink(pwm);
    }

    delay(500);
  }
}

void go_program_b(int pwm, int n_con) // Подпрограмма езды по заданым шагам программы после B
{
  int n;
  for (n = n_con; n > 0; n--) {
    Serial.print(n);
    Serial.print(" np= ");
    Serial.print(program_move[n]);
    Serial.print(" tm= ");
    Serial.println(program_time[n]);

    if     (program_move[n] == 0x0)  { // вперед при нажатой X; T min
      go_forward_x(pwm);
    }
    else if (program_move[n] == 0x2) { // вперед при нажатой Y; T ave
      go_forward_y(pwm);
    }
    else if (program_move[n] == 0x1) { // вперед при нажатой Z; T max
      go_forward_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    else if (program_move[n] == 0x4) {// влево при нажатой X; T min
      go_left_x(pwm);
    }
    else if (program_move[n] == 0x6) {// влево при нажатой Y; T ave
      go_left_y(pwm);
    }
    else if (program_move[n] == 0x5) {// влево при нажатой Z; T max
      go_left_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    else if (program_move[n] == 0x8) {// вправо при нажатой X; T min
      go_right_x(pwm);
    }
    else if (program_move[n] == 0xA) {// вправо при нажатой Y; T ave
      go_right_y(pwm);
    }
    else if (program_move[n] == 0x9) {// вправо при нажатой Z; T max
      go_right_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    else if (program_move[n] == 0xC) {// назад при нажатой X; T min
      go_bakward_x(pwm);
    }
    else if (program_move[n] == 0xE) {// назад при нажатой Y; T ave
      go_bakward_y(pwm);
    }
    else if (program_move[n] == 0xD) {// назад при нажатой Z; T max
      go_bakward_z(pwm);
      delay(program_time[n]);
      go_stop(pwm);
    }
    delay(500);
  }
}

int check_cmd() //  функция для проверки коректности принятых байт
{
  int i;
  int flag = 0; // флаг выхода по  признаку правильной команды
  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_forward[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 1; //  распознали  команду
  flag = 0;

  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_bakward[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 2; //  распознали  команду
  flag = 0;

  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_left[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 3; //  распознали  команду
  flag = 0;

  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_right[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 4; //  распознали  команду
  flag = 0;


  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_stop[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 5; //  распознали  команду
  flag = 0;


  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_start[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 6; //  распознали  команду
  flag = 0;


  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_a[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 7; //  распознали  команду
  flag = 0;



  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_b[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 8; //  распознали  команду
  flag = 0;

  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_c[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 9; //  распознали  команду
  flag = 0;

  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_x[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 10; //  распознали  команду
  flag = 0;

  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_y[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 11; //  распознали  команду
  flag = 0;

  for (i = 0; i < inByte_count; i++)
  {
    if (inByte[i] != byte_z[i] ) {
      flag = 1;
      break;
    }
  }
  if (flag == 0) return 12; //  распознали  команду

  return 0;
}
