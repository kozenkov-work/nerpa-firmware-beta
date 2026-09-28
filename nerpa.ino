#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <SD.h>
#include <vector>

// Инициализация дисплея
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Пины
const int CS_PIN     = 5;   // SD-карта
const int PIN_UP     = 13;  // ВВЕРХ
const int PIN_DOWN   = 14;  // ВНИЗ
const int PIN_SELECT = 27;  // ВЫБОР
const int PIN_BACK   = 26;  // НАЗАД

// Состояния кнопок для фиксации клика
bool lastUpState     = HIGH;
bool lastDownState   = HIGH;
bool lastSelectState = HIGH;
bool lastBackState   = HIGH;

// Машина состояний (Экраны)
enum AppState {
  STATE_START,       // Стартовое меню
  STATE_SETTINGS,    // Настройки (заглушка)
  STATE_FILE_LIST,   // Выбор ПО (файлы на SD)
  STATE_CONSOLE      // Выполнение программы
};
AppState currentState = STATE_START;

// Настройки UI и геометрии
const int MAX_LINES = 3;     // Максимум строк во внутреннем окне
const int LINE_HEIGHT = 15;  // Шаг строки
const int START_Y = 30;      // Координата Y для первой строки локального окна

// Данные Стартового меню
String startMenuItems[] = {"Настройки", "Выбор программы"};
int startMenuIndex = 0;

// Данные Списка файлов (ПО)
std::vector<String> fileNames;
int fileMenuIndex = 0;
int fileTopIndex = 0;

// Данные Консоли
std::vector<String> consoleLines;
int consoleScrollIndex = 0;
bool consoleAutoScroll = true;

// -----------------------------------------------------------------
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// -----------------------------------------------------------------

bool isButtonClicked(int pin, bool &lastState) {
  bool currentState = digitalRead(pin);
  bool clicked = false;
  if (lastState == HIGH && currentState == LOW) {
    clicked = true;
    delay(40); // Антидребезг
  }
  lastState = currentState;
  return clicked;
}

String getBatteryStub() {
  return "[||||]~"; // Текстовая заглушка батареи
}

// -----------------------------------------------------------------
// МЕТОДЫ РАБОТЫ С ДАННЫМИ И КОНСОЛЬЮ
// -----------------------------------------------------------------

void loadFileList() {
  fileNames.clear();
  File root = SD.open("/");
  if (!root) return;

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      if (name.startsWith("/")) name = name.substring(1);
      fileNames.push_back(name);
    }
    entry.close();
  }
  root.close();
}

// УНИВЕРСАЛЬНЫЙ МЕТОД: Вывод строк в консоль
void printToConsole(String text) {
  consoleLines.push_back(text);
  
  // Ограничитель памяти, чтобы ESP32 не зави  printToConsole("Залупа бобра...");
  printToConsole("Залупа бобра...");
  printToConsole("Залупа бобра...");
  printToConsole("Залупа бобра...");
  printToConsole("Залупа бобра...");
  printToConsole("Залупа бобра...");
  printToConsole("Залупа бобра...");
  printToConsole("Залупа бобра...");
…  printToConsole("Залупа бобра...");сла при бесконечном логе
  if (consoleLines.size() > 15) {
    consoleLines.erase(consoleLines.begin());
  }

  // Если автоскролл включен, мотаем в самый низ
  if (consoleAutoScroll) {
    if (consoleLines.size() > MAX_LINES) {
      consoleScrollIndex = consoleLines.size() - MAX_LINES;
    } else {
      consoleScrollIndex = 0;
    }
  }
}

// Симуляция запуска программы (заглушка для будущей магии с бинарниками)
void runProgram(String fileName) {
  consoleLines.clear();
  consoleScrollIndex = 0;
  consoleAutoScroll = true;
  currentState = STATE_CONSOLE;
  
  printToConsole("Запуск: " + fileName);
  printToConsole("Чтение дампа...");
  printToConsole("ОК: 0x00A45B");
  printToConsole("Исполнение...");
  printToConsole("Залупа бобра...");
}

// -----------------------------------------------------------------
// МЕТОДЫ ОТРИСОВКИ ИНТЕРФЕЙСА (UI)
// -----------------------------------------------------------------

// ГЛОБАЛЬНОЕ ОКНО: Отрисовка статус-бара
void drawStatusBar() {
  String location = "";
  if (currentState == STATE_START) location = "Старт";
  else if (currentState == STATE_SETTINGS) location = "Настройки";
  else if (currentState == STATE_FILE_LIST) location = "ПО";
  else if (currentState == STATE_CONSOLE) location = "Консоль";

  display.setCursor(0, 11);
  display.print(location);

  // Выравнивание батареи по правому краю
  String bat = getBatteryStub();
  int batWidth = display.getStrWidth(bat.c_str());
  display.setCursor(128 - batWidth, 11);
  display.print(bat);

  // Разделительная линия статус-бара
  display.drawLine(0, 14, 128, 14);
}

// ВНУТРЕННЕЕ ОКНО 1: Стартовое меню
void drawStartMenu() {
  for (int i = 0; i < 2; i++) {
    int y = START_Y + i * LINE_HEIGHT;
    if (i == startMenuIndex) {
      display.setCursor(0, y);
      display.print("> " + startMenuItems[i]);
    } else {
      display.setCursor(12, y);
      display.print(startMenuItems[i]);
    }
  }
}

// ВНУТРЕННЕЕ ОКНО 2: Выбор ПО (SD карта)
void drawFileList() {
  if (fileNames.empty()) {
    display.setCursor(0, START_Y);
    display.print("Файлы не найдены");
    return;
  }

  if (fileMenuIndex < fileTopIndex) {
    fileTopIndex = fileMenuIndex;
  } else if (fileMenuIndex >= fileTopIndex + MAX_LINES) {
    fileTopIndex = fileMenuIndex - MAX_LINES + 1;
  }

  for (int i = 0; i < MAX_LINES; i++) {
    int idx = fileTopIndex + i;
    if (idx >= (int)fileNames.size()) break;

    int y = START_Y + i * LINE_HEIGHT;
    if (idx == fileMenuIndex) {
      display.setCursor(0, y);
      display.print("> " + fileNames[idx]);
    } else {
      display.setCursor(12, y);
      display.print(fileNames[idx]);
    }
  }
}

// ВНУТРЕННЕЕ ОКНО 3: Консоль
void drawConsole() {
  for (int i = 0; i < MAX_LINES; i++) {
    int idx = consoleScrollIndex + i;
    if (idx >= (int)consoleLines.size()) break;

    int y = START_Y + i * LINE_HEIGHT;
    display.setCursor(0, y);
    display.print(consoleLines[idx]);
  }
}

// ВНУТРЕННЕЕ ОКНО 4: Заглушка настроек
void drawSettings() {
  display.setCursor(0, START_Y);
  display.print("Тут пока пусто.");
}

// Сборка всего кадра
void updateScreen() {
  display.clearBuffer();
  drawStatusBar(); // Всегда рисуем глобальный UI
  
  // Рисуем нужный внутренний экран
  if (currentState == STATE_START) drawStartMenu();
  else if (currentState == STATE_SETTINGS) drawSettings();
  else if (currentState == STATE_FILE_LIST) drawFileList();
  else if (currentState == STATE_CONSOLE) drawConsole();
  
  display.sendBuffer();
}

// -----------------------------------------------------------------
// ОСНОВНАЯ ЛОГИКА
// -----------------------------------------------------------------

void setup() {
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP);
  pinMode(PIN_BACK, INPUT_PULLUP); // Кнопка Назад

  display.begin();
  display.enableUTF8Print();
  display.setFont(u8g2_font_6x13_t_cyrillic);

  display.clearBuffer();
  display.setCursor(0, 30);
  display.print("Монтирование SD...");
  display.sendBuffer();

  if (!SD.begin(CS_PIN)) {
    display.clearBuffer();
    display.setCursor(0, 30);
    display.print("Ошибка SD-карты!");
    display.sendBuffer();
    while (true);
  }

  updateScreen();
}

void loop() {
  bool clickUp     = isButtonClicked(PIN_UP, lastUpState);
  bool clickDown   = isButtonClicked(PIN_DOWN, lastDownState);
  bool clickSelect = isButtonClicked(PIN_SELECT, lastSelectState);
  bool clickBack   = isButtonClicked(PIN_BACK, lastBackState);

  // === ОБРАБОТКА ЭКРАНА СТАРТ ===
  if (currentState == STATE_START) {
    if (clickUp && startMenuIndex > 0) startMenuIndex--;
    if (clickDown && startMenuIndex < 1) startMenuIndex++;
    if (clickSelect) {
      if (startMenuIndex == 0) {
        currentState = STATE_SETTINGS;
      } else {
        loadFileList();
        fileMenuIndex = 0;
        currentState = STATE_FILE_LIST;
      }
    }
    if (clickUp || clickDown || clickSelect || clickBack) updateScreen();
  } 
  
  // === ОБРАБОТКА ЭКРАНА НАСТРОЕК ===
  else if (currentState == STATE_SETTINGS) {
    if (clickBack) {
      currentState = STATE_START;
      updateScreen();
    }
  } 
  
  // === ОБРАБОТКА ЭКРАНА ВЫБОРА ПО ===
  else if (currentState == STATE_FILE_LIST) {
    if (clickUp && fileMenuIndex > 0) fileMenuIndex--;
    if (clickDown && fileMenuIndex < (int)fileNames.size() - 1) fileMenuIndex++;
    if (clickBack) {
      currentState = STATE_START;
    }
    if (clickSelect && !fileNames.empty()) {
      runProgram(fileNames[fileMenuIndex]); // Имитация запуска
    }
    if (clickUp || clickDown || clickSelect || clickBack) updateScreen();
  } 
  
  // === ОБРАБОТКА ЭКРАНА КОНСОЛИ ===
  else if (currentState == STATE_CONSOLE) {
    if (clickUp && consoleScrollIndex > 0) {
      consoleScrollIndex--;
      consoleAutoScroll = false; // Отключаем слежение при ручном скролле
    }
    if (clickDown && consoleScrollIndex + MAX_LINES < (int)consoleLines.size()) {
      consoleScrollIndex++;
      // Если долистали до самого низа - снова включаем автоскролл
      if (consoleScrollIndex + MAX_LINES >= (int)consoleLines.size()) {
        consoleAutoScroll = true;
      }
    }
    if (clickBack) {
      currentState = STATE_FILE_LIST; // Убиваем процесс, возвращаемся в список
    }
    if (clickUp || clickDown || clickBack) updateScreen();
  }
}