#include <LiquidCrystal.h>
//-----------------------------------------------------------------------
// инициализация экрана
//-----------------------------------------------------------------------
const uint8_t PIN_RS = 6;
const uint8_t PIN_EN = 8;
const uint8_t PIN_DB4 = 9;
const uint8_t PIN_DB5 = 10;
const uint8_t PIN_DB6 = 11;
const uint8_t PIN_DB7 = 12;
//в порядке:       RS   E DB4 DB5 DB6 DB7
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7);
//LiquidCrystal lcd(13, 12, 8, 9, 10, 11);
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// состояния конечного автомата
//-----------------------------------------------------------------------
enum State 
{
    S1, // настройка первой цифры
    S2, // настройка второй цифры
    S3, // настройка третьей цифры
    S4, // настройка четвертой цирфы
    S5, // сигнализация старта
    S6, // отсчета таймера, работа компрессора, проверка нажатия кнопки
    S7, // режим паузы 
    S8  // конечный экран
};
//-----------------------------------------------------------------------

State state = S1;  // объект состояний



uint8_t timer_pos_lcd[] = {9,10,12,13}; // позиция на самом экране
uint8_t timer_val[] = {1,2,3,4};       // значение каждой позиции
uint8_t timer_digit_num = 0;          // номер позиции от нуля до 3 слева направо каждой позиции
bool timer_pause = 0;                // флаг для прерывания таймера на паузу при нажатии кнопки
bool flag = 0;                      // флаг для индикация на экране выбранной цифры

unsigned long time_gone = 0; // отсчет пройденного времени для индикация выбранной цифры
uint32_t led_tmr = 0; // таймер для светодиода
uint8_t dir = 5;    // направление свечения таймера (ярче/тусклее)
uint8_t duty = 0;   // ШИМ значение яркости таймера


#define BUZZ A1 // пин пеьзопищалки
#define RELE A3 // пин реле
#define LED 5  // пин светодиода


class BUTTON // класс обработки кнопок
{
  public:
  
  BUTTON(byte pin) // конструктор класса
  {
     _pin = pin;
     pinMode(_pin, INPUT);
  }

  void tick() // функция обработки нажатия и зажатия
  {
    curr_state = digitalRead(_pin);
    
    // обработка нажатия кнопки
    if (curr_state && !flag && millis() - last_press > 250)
    {
      flag = true;
      press_flag = true; // флаг нажатия кнопки для метода нажатия 
      last_press = millis();
    }
    // обработка отжатия кнопки после нажатия
    if (!curr_state && flag) 
    {
      flag = false;
    }
    // обработка зажатия кнопки
    if (curr_state && !flag_long && millis() - last_press > 1000)
    {
      flag_long = true;
      long_press_flag = true; // флаг зажатия кнопки для метода зажатия 
      last_press = millis();
    }
    // обработка отжатия кнопки после зажатия
    if (!curr_state && flag_long) 
    {
      flag_long = false;
    }
 
  }
  
  bool isPress()
  {
    BUTTON::tick();
    
    if (press_flag) // если флаг поднят
    {
      press_flag = false; // опускает флаг
      return true;         // возвращает единицу
    }
    else
    {
      return false;
    }
    
  }
  
  bool isToggled()
  {
    BUTTON::tick();
    
    if (long_press_flag) // если флаг поднят
    {
      press_flag = false;      // опускает флаг
      long_press_flag = false; // возвращает единицу
      return true;
    }
    else
    {
      return false; 
    }
  }
  
  void block_button_for(uint32_t tmr) // блокировка флагов кнопки на мс
  {
    while (millis() - last_press < tmr)
    {
      curr_state = 0; // мб залочить все фалги надо
    }
  }
    
  
  private:
    byte _pin;
    bool flag = 0;
    bool flag_long = 0;
    bool press_flag = 0;  // флаг нажатия кнопки
    bool long_press_flag = 0; // флаг долго нажатия кнопки
    bool curr_state = 0;    // флаг текущего состояния кнопки
    uint32_t last_press = 0;
};


// инициализируем кнопки на вход (стянуты на землю)
  BUTTON but_1(2);
  BUTTON but_2(3);
  BUTTON but_3(4);


void setup() 
{
  
  Serial.begin(9600);

  lcd_start_screen(); // вызов начального окна
  lcd_menu();         // вызов меню

  // 
  pinMode(BUZZ, OUTPUT);
  pinMode(RELE, OUTPUT);
  pinMode(LED, OUTPUT);
}


void loop() 
{ 

  switch(state)
  {
    case S1: //1 настройка первой цифры
    led_blink_smooth();
    timer_digit_num = 0;
    lcd_print_timer();
    lcd_blink();
    if (but_1.isPress()) lcd_timer_up(); // если первая кнопка нажата, то увеличиваем таймер
    if (but_2.isPress()) lcd_timer_down(); // если вторая кнопка нажата, то уменьшаем таймер
    if (but_3.isToggled()) state = S5; // если третья кнопка зажата, то переходим в отсчет таймера
    if (but_3.isPress())  
    { 
      if (timer_digit_num != 3)
      {
        timer_digit_num++;
      }
      else
      {
        timer_digit_num = 0;
      }
      
      state = S2;// если третья кнопка нажата, то меняем на следующую позицию таймер
    }

  break;

  case S2: //2 настройка второй цифры
    led_blink_smooth();
    lcd_print_timer();
    lcd_blink();
    if (but_1.isPress()) lcd_timer_up(); // если первая кнопка нажата, то увеличиваем таймер
    if (but_2.isPress()) lcd_timer_down(); // если вторая кнопка нажата, то уменьшаем таймер
    if (but_3.isToggled()) state = S5; // если третья кнопка зажата, то переходим в отсчет таймера
    if (but_3.isPress()) 
    { 
      if (timer_digit_num != 3)
      {
        timer_digit_num++;
      }
      else
      {
        timer_digit_num = 0;
      }
      state = S3;// если третья кнопка нажата, то меняем на следующую позицию таймер
    }

  break;

  case S3: //3 настройка третьей цифры
    led_blink_smooth();
    lcd_print_timer();
    lcd_blink();
    if (but_1.isPress()) lcd_timer_up(); // если первая кнопка нажата, то увеличиваем таймер
    if (but_2.isPress()) lcd_timer_down(); // если вторая кнопка нажата, то уменьшаем таймер
    if (but_3.isToggled()) state = S5; // если третья кнопка зажата, то переходим в отсчет таймера
    if (but_3.isPress())  
    { 
      if (timer_digit_num != 3)
      {
        timer_digit_num++;
      }
      else
      {
        timer_digit_num = 0;
      }
      state = S4;// если третья кнопка нажата, то меняем на следующую позицию таймер
    }

  break;

  case S4: //4 настройка четвертой цифры
    led_blink_smooth();
    lcd_print_timer();
    lcd_blink();
    if (but_1.isPress()) lcd_timer_up(); // если первая кнопка нажата, то увеличиваем таймер
    if (but_2.isPress()) lcd_timer_down(); // если вторая кнопка нажата, то уменьшаем таймер
    if (but_3.isToggled()) state = S5; // если третья кнопка зажата, то переходим в отсчет таймера
    if (but_3.isPress()) 
    { 
      if (timer_digit_num != 3)
      {
        timer_digit_num++;
      }
      else
      {
        timer_digit_num = 0;
      }
      state = S1;// если третья кнопка нажата, то меняем на следующую позицию таймер
    }

  break;

  case S5: // бипер издает звук начала 
    delay(150);
    tone(BUZZ, 40, 500);
    //timer_count();
    state = S6;
  break;

  case S6: // отсчет таймера
    Serial.println("State S6");
    digitalWrite(RELE, LOW);
    digitalWrite(LED, HIGH);

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("timer:");
    lcd.setCursor(11, 0);
    lcd.print(":");
    lcd.setCursor(4, 1);
    lcd.print("etching");
    
    timer_count();
    //delay(100);
    if (timer_pause) // если поднят флаг паузы, то переходим в режим паузы
    //if (button_statement(&but_3) == 0)
    {
      digitalWrite(RELE, HIGH);   
      state = S7;   // ПЕРЕХОДИТ В СОСТОЯНИЕ S7
    } else {
    
      digitalWrite(RELE, HIGH);
      state = S8;
    }
    
  break;

  case S7:  // режим паузы
    Serial.println("State S7");
    if (timer_pause)
    {
      lcd.clear();
      lcd_print_timer();
      lcd.setCursor(5, 1);
      lcd.print("pause");
      timer_pause = 0;
      
    }

    led_blink(); // мигаем часто
    buzz_beep();  // пиликаем
    
    if (but_3.isToggled()) state = S5;
    
  break;

  case S8:  // окончание отсчета таймера
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Etching");
      lcd.setCursor(5, 1);
      lcd.print("is done");
      
      tone(BUZZ, 5800, 1000);
      digitalWrite(LED, LOW);
      
      delay(2500);
      lcd.clear();
      lcd_menu();
      state = S1;
      
  break;

  }

}

//-----------------------------------------------------------------------
// стартовый экран устройства
//-----------------------------------------------------------------------
void lcd_start_screen()
{
  lcd.begin(16, 2);
  // Устанавливаем курсор в колонку 0 и строку 0
  lcd.clear();
  lcd.setCursor(0, 0);
  // Печатаем первую строку
  lcd.print("Etching machine");
  lcd.setCursor(0, 1);
  lcd.print("ver 1.1");
  delay(1500);
}
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// меню настройки
//-----------------------------------------------------------------------
void lcd_menu()
{
  lcd.clear();
  /*lcd.setCursor(2, 0);
  lcd.print("timer:");
  lcd.setCursor(11, 0);
  lcd.print(":");
  */
  lcd.setCursor(0, 1);
  lcd.print("toggle OK=start");
  
  lcd_print_timer(); 
}
//-----------------------------------------------------------------------




//-----------------------------------------------------------------------
// функция индикации текущей цифры для настройки
//-----------------------------------------------------------------------
// эту функцию можно поменять на встроенную в библиотеку функцию

void lcd_blink()
{

  //if (millis() - but_1.last_press > 350 & millis() - but_2.last_press > 350 & millis() - but_3.last_press > 350)
 // {

    if (millis() - time_gone > 350)
      {
        time_gone = millis();
        flag = !flag;
      } 
  
      if (flag)
      {
        lcd.setCursor(timer_pos_lcd[timer_digit_num], 0);
        lcd.print(timer_val[timer_digit_num]);
      }
      else
      {
        lcd.setCursor(timer_pos_lcd[timer_digit_num], 0);
        lcd.print(" ");
        delay(5); // задержка, чтобы цирфа успела исчезнуть надо пофиксить хз как
      }
 // }
}



//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// функция отсчета таймера
//-----------------------------------------------------------------------
void timer_count()
{

  while (1)
  {
    if (but_3.isPress())
    {
      timer_pause = 1;
      break; // если нажата 3 кнопка вовремя отсчета, то прерываемся
    }
    if (millis() - time_gone >= 1000)
    {
      time_gone = millis();
      
      if (timer_val[3] == 0) // проверка единиц секунд
      {
        if (timer_val[2] != 0) // проверка десяток секунд
        {
          timer_val[2] --;
          timer_val[3] = 9;
        }
        else if (timer_val[1] != 0) // проверка единиц минут
        {
          timer_val[1] --;
          timer_val[2] = 5;
          timer_val[3] = 9;
        } 
        else if (timer_val[0] != 0) // проверка десяток минут
        {
          timer_val[0] --;
          timer_val[1] = 9;
          timer_val[2] = 5;
          timer_val[3] = 9;
        } 
        else 
        {
          break;
        }

    }
      else
      {
        timer_val[3]--;
      }
      
      lcd_print_timer();     
    }
  }
}
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// функция вывода таймера на экран
//-----------------------------------------------------------------------
void lcd_print_timer()
{
  lcd.setCursor(2, 0);
  lcd.print("timer:");
  lcd.setCursor(11, 0);
  lcd.print(":");
  for (int i = 0; i <= 3; i++)
  {
    lcd.setCursor(timer_pos_lcd[i], 0); // выставляем курсор на нужную позицию
    lcd.print(timer_val[i]);
  }
}
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// счет таймера вверх
//-----------------------------------------------------------------------
void lcd_timer_up()
{
  uint8_t max = 5;
  if (timer_digit_num == 2)
  {
    if (timer_val[timer_digit_num] != max)
    {
      timer_val[timer_digit_num]++;
    }
    else
    {
      timer_val[timer_digit_num] = 0;
    }
  }
  else
  {
    max = 9;
    if (timer_val[timer_digit_num] != max)
    {
      timer_val[timer_digit_num]++;
    }
    else
    {
      timer_val[timer_digit_num] = 0;
    }
  }
  
}
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// счет таймера вниз
//-----------------------------------------------------------------------
void lcd_timer_down()
{
  uint8_t max = 5;
  if (timer_digit_num == 2)
  {
    if (timer_val[timer_digit_num] != 0)
    {
      timer_val[timer_digit_num]--;
    }
    else
    {
      timer_val[timer_digit_num] = max;
    }
  }
  else
  {
    max = 9;
    if (timer_val[timer_digit_num] != 0)
    {
      timer_val[timer_digit_num]--;
    }
    else
    {
      timer_val[timer_digit_num] = max;
    }
  }
  
}


void led_blink()
{
  
  if (millis() - led_tmr > 350)
  {
    led_tmr = millis();
    digitalWrite(LED, !digitalRead(LED));
  }

}

void led_blink_smooth()
{
  
  if (millis() - led_tmr > 20)
  {
    led_tmr = millis();
    duty += dir;
    if (duty >= 255 || duty <= 0) dir = -dir;
    analogWrite(LED, duty);
  }

}

void buzz_beep()
{
  if (millis() - time_gone > 1500)
    {
      time_gone = millis();
      tone(BUZZ, 45, 100);
    }
}

//-----------------------------------------------------------------------

// где то ддублируется вывод текста на экран надо исправить
// сделать енум для вывода разных экранов или запихнуть в функции | сделано, ну вроде как
// сделать глобальный флаг блокировки нажатия кнопки после паузы, чтобы избежать нажатия
