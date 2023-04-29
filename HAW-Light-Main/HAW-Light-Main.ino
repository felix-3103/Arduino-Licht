#include <arduino.h>
#include <esp_dmx.h>
#include <LiquidCrystal_I2C.h>  // Bibliothek für die LCD Anzeige
#include <LinkedList.h>         // Bibliothek für die Datenstruktur des Menüs
#include <IRremote.h>           // Bibliothek für die Verwendung der IR-Fernbedienung
#include <Wire.h>               // Bibliothek wird für LCD I2C benötigt
#include <FastLED.h>            // Bibliothek für die Nutzung eines LED Streifens


// Sensor Makros
#define IRRECIEVER_DATA_PIN 19

// Menü Makros
#define MODI_DMX "DMX"
#define MODI_FUNKY "Funky"
#define MODI_RAINBOW "Rainbow"
#define MODI_RAUM "Raumlicht"
#define MODI_FARBEPALETTE "Farbpalette"
#define MODI_FARBE "Farbe"
#define MODI_SETTINGS "Einstellungen"

// Licht Makros
#define NUM_LEDS 100      // Anzahl der Pixel muss bei neuem Gehäuse angepasst werden.
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 192
#define DATA_PIN 5


// Datenstrukturen für die Menüstruktur
enum e_Modi{
  e_raumlicht,
  e_farbe,
  e_farbpalette,
  e_funky,
  e_rainbow,
  e_dmx,
  e_settings
};

class Parameter{
  public:
    String ap_title;
    byte ap_parameter;
    byte ap_max, ap_min, ap_steps;

    Parameter(String title, byte parameter, byte max = 255, byte min = 0, byte steps = 5) {
      ap_title = title;
      ap_parameter = parameter;
      ap_max = max;
      ap_min = min;
      ap_steps = steps;
    }
};

class Modi{
  public:
    String am_title;
    LinkedList<Parameter*> am_parameterList;
};

// Datenstrukturen für Lichtprogrammierung
class Color{
  public:
    String am_title;
    CRGB am_color;

  Color(String title, CRGB color) {
    am_title = title;
    am_color = color;
  }
};

// Variablen für die Hardwarekomponenten
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variablen für die Menustruktur
LinkedList<Modi*> ModiList = LinkedList<Modi*>();
int p_ModiPosition = e_raumlicht;
int p_ParameterPosition = 0;
bool in_Eingabe = false;

// Variablen für Licht
CRGB leds[NUM_LEDS];

uint8_t HAW_Logo[] = {
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000, 0x0f46a3, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000, 0x000000, 0x000000, 0x0f46a3,
  0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x0f46a3, 0x0f46a3, 0x000000, 0x000000, 0x000000, 0x0f46a3,
  0x000000, 0x0f46a3, 0x0f46a3, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000, 0x000000, 0x000000, 0x0f46a3,
  0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000, 0x0f46a3,
  0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000, 0x000000, 0x0f46a3, 0x000000, 0x0f46a3, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};

uint8_t hue = 0;
unsigned long previousTime = 0;
unsigned long currentTime = 0;

//Variablen für DMX
int RX_Pin = 16;
dmx_port_t dmxPort = 1;
byte dmxdata [7];
QueueHandle_t queue;
bool dmxIsConnected = false;

// Farbarrays
Color* tempArray[] = {
  new Color("1800K", CRGB(215,65,10)),
  new Color("2000K", CRGB(255,80,15)), 
  new Color("2500K", CRGB(255,125,45)),
  new Color("2850K", CRGB(255,214,170)),
  new Color("3200K", CRGB(255,241,224)),
  new Color("5200K", CRGB(255,250,244)),
  new Color("6000K", CRGB(255,255,255))
};

Color* farbArray[] = {
  new Color("Rot", CRGB::Red),
  new Color("Gruen", CRGB::Green),
  new Color("Blau", CRGB::Blue),
  new Color("Gelb", CRGB::Yellow),
  new Color("Lila", CRGB::Purple),
  new Color("Orange", CRGB::Orange)
};

// Menu Funktionsprototyp
void InitilizeLCD();
void InitilizeMenu();
void InitilizeLight();
void InitilizeDMX();
void PrintModi(int, int);

// Licht Funktionsprototyp
void RunModi(byte);
void LichtModiRainbow(byte);
void LichtModiRaum(byte);
void LichtModiFarbe(CRGB);
void LichtModiFarbpalette(byte , byte, byte);
void Settings(byte);

// the setup function runs once when you press reset or power the board
void setup() {
  // Initialiserung der Hardware
  Serial.begin(115200);
  InitilizeLCD();
  InitilizeLight();
  InitilizeMenu();
  InitilizeDMX();
}

void loop() {

  if(IrReceiver.decode()) { 

    Serial.println(IrReceiver.decodedIRData.command, HEX);

      // Navigation durch die ModiListe
      if (IrReceiver.decodedIRData.address == 0x0) {
        
        if (IrReceiver.decodedIRData.command == 0x40) {
          in_Eingabe = !in_Eingabe;
        }
        
        if(!in_Eingabe) {
          if (IrReceiver.decodedIRData.command == 0x43) {
            // Mit dem Button "Down" werden die Modi nacheinander abwärts durchgegangen. Dabei wird jeweils beim Ende nicht weitergegangen.
            p_ModiPosition++;

            if (p_ModiPosition >= ModiList.size()){
              p_ModiPosition = ModiList.size() - 1;
            }

            p_ParameterPosition = 0;
            in_Eingabe = false;
          }

          if (IrReceiver.decodedIRData.command == 0x44) {
            // Mit dem Button "Up" werden die Modi nacheinander aufwärtsdurchgegangen. Dabei wird jeweils beim Anfang nicht weitergegangen.
            p_ModiPosition --;

            if (p_ModiPosition <= 0){
              p_ModiPosition = 0;
            }

            p_ParameterPosition = 0;
            in_Eingabe = false;
          }

          if (IrReceiver.decodedIRData.command == 0x09) {
            // Navigation in der ParameterListe. Soll nur in eine Richtung gehen.
            p_ParameterPosition++;
            if(p_ParameterPosition >= ModiList.get(p_ModiPosition)->am_parameterList.size()) {
              p_ParameterPosition = 0;
            }
          }

          if (IrReceiver.decodedIRData.command == 0x07) {
            // Navigation in der ParameterListe. Soll nur in eine Richtung gehen.
            p_ParameterPosition--;
            if(p_ParameterPosition <= 0) {
              p_ParameterPosition = ModiList.get(p_ModiPosition)->am_parameterList.size() - 1;
            }
          }              
        }


        if(in_Eingabe) {
          int eingabeWert = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter;
          int maximalerWert = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_max;
          int minimalerWert = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_min;
          int schritte = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_steps;

          if(IrReceiver.decodedIRData.command == 0x46) {
            eingabeWert = eingabeWert + schritte;

            if(eingabeWert > maximalerWert) {
              eingabeWert = maximalerWert;
            }

            ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter = eingabeWert;
          }

          if(IrReceiver.decodedIRData.command == 0x15) {
            eingabeWert = eingabeWert - schritte;

            if(eingabeWert < minimalerWert) {
              eingabeWert = minimalerWert;
            }

            ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter = eingabeWert;
          }
        }
      }

      // Am Ende soll der aktuelle Menüpunkt auf den LCD Display ausgegeben werden.
      PrintModi(p_ModiPosition, p_ParameterPosition);
      delay(100);
      IrReceiver.resume();
    } else {
      RunModi(p_ModiPosition);
    }
}

// Hier soll die LCD Anzeige Initalisiert werden.
void InitilizeLCD() {
  Serial.begin(9600);
  IrReceiver.begin(IRRECIEVER_DATA_PIN);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Willkommen ;)");
  lcd.setCursor(0, 1);
  lcd.print("HAW-Light");
}

// Hier soll das Menü initalisiert werden. Jeder Menüpunkt wird erzeugt inkl. der Parameter.
// Dabei kann jederzeit ohne große Mühe ein Modi ergänzt werden.
void InitilizeMenu() {

  // MODI 1 - Raumlicht
  Modi *raumlicht = new Modi();
  raumlicht->am_title = MODI_RAUM;
  // new Parameter ("Titel", Startwert, maximaler Wert, minimaler Wert, Schrittweite);
  raumlicht->am_parameterList.add(new Parameter("Temp.", 0, (sizeof(tempArray)/sizeof(tempArray[0]))-1, 0, 1));
  ModiList.add(raumlicht);

  // MODI 2 - Farbe
  Modi *farbe = new Modi();
  farbe->am_title = MODI_FARBE;
  farbe->am_parameterList.add(new Parameter("Farbe", 0, (sizeof(farbArray)/sizeof(farbArray[0]))-1, 0, 1));
  ModiList.add(farbe);

  // MODI 3 - Farbpalette
  Modi *farbPalette = new Modi();
  farbPalette->am_title = MODI_FARBEPALETTE;
  farbPalette->am_parameterList.add(new Parameter("Rot", 0));
  farbPalette->am_parameterList.add(new Parameter("Gruen", 0));
  farbPalette->am_parameterList.add(new Parameter("Blau", 0));
  ModiList.add(farbPalette);

  // MODI 4 - Effektlicht
  Modi *funky = new Modi();
  funky->am_title = MODI_FUNKY;
  funky->am_parameterList.add(new Parameter("Tempo(%)", 10, 100, 10, 10));
  ModiList.add(funky);

  // MODI 5 - Effektlicht
  Modi *rainbow = new Modi();
  rainbow->am_title = MODI_RAINBOW;
  rainbow->am_parameterList.add(new Parameter("Tempo(%)", 10, 100, 10, 10));
  ModiList.add(rainbow);

  // MODI 6 - DMX
  Modi *dmx = new Modi();
  dmx->am_title = MODI_DMX;
  dmx->am_parameterList.add(new Parameter("Adress:", 0, 512-4, 0, 1));
  ModiList.add(dmx);

  // Einstellungen (Wird aber als Modi der Datenstruktur hinzugefügt)
  Modi *settings = new Modi();
  settings->am_title = MODI_SETTINGS;
  settings->am_parameterList.add(new Parameter("Hellig.(%)", 100, 100, 0, 10));
  ModiList.add(settings);

  // Startposition setzen
  PrintModi(p_ModiPosition, p_ParameterPosition);
}

// Menu Funktionen
// Hiermit soll der Modi über das Display ausgegeben werden.
void PrintModi(int positionModi, int positionParameter) {
  lcd.clear();

  // In einem ersten Schritt sollen die Modi auf dem LCD Display dargestellt werden. Eine wichige Unterscheidung ist jedoch das Thema "Einstellungen".
  switch (positionModi) {
    case e_settings:
    //Einstellungen sollen nicht als Modi bezeichnet werden.
      lcd.print(ModiList.get(positionModi)->am_title);
      break;
    default:
      lcd.print("MODI: " + ModiList.get(positionModi)->am_title);
      break;
  }
  
  // Danach werden die Parameter dargestellt. Dabei hat aber DMX aktuell keine Parameter und wird als Ausnahme deklariert.

  lcd.setCursor(0,1);
  lcd.print(ModiList.get(positionModi)->am_parameterList.get(positionParameter)->ap_title + ":");

  lcd.setCursor(16-6,1);
  byte positionLicht = ModiList.get(positionModi)->am_parameterList.get(positionParameter)->ap_parameter;

  switch (positionModi) {
    case e_raumlicht: {
      lcd.print(tempArray[positionLicht]->am_title);
      break;
    }
    case e_farbe: {
      lcd.print(farbArray[positionLicht]->am_title);
      break;   
    }
    default: {
      lcd.setCursor(16-4,1);
      lcd.print(ModiList.get(positionModi)->am_parameterList.get(positionParameter)->ap_parameter);
      break;
    }
  }
}

// Lichtfunktionen
void InitilizeDMX() {
  dmx_config_t dmxConfig = DMX_DEFAULT_CONFIG;
  dmx_param_config(dmxPort, &dmxConfig);
  dmx_set_pin(dmxPort, DMX_PIN_NO_CHANGE, RX_Pin, DMX_PIN_NO_CHANGE);
  int queueSize = 1;
  int interruptPriority = 1;
  dmx_driver_install(
    dmxPort, DMX_MAX_PACKET_SIZE,
    queueSize, &queue,
    interruptPriority);
}

// Initaliiserung der Strips
void InitilizeLight() {
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);// Lichtstärke setzen
  previousTime = currentTime;
  printLogo();
  delay(2000);
}

// Ausgabe des Logos auf der Matrix
void printLogo() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = HAW_Logo[i];
  }
  
  FastLED.show();
}

// Hier soll entschieden werden, welcher Modi getriggert werden soll.
void RunModi(byte _position) {

  switch (_position) {
    case e_funky: {
      LichtModiFunky(ModiList.get(e_funky)->am_parameterList.get(0)->ap_parameter);
      break;
    }

    case e_rainbow: {
      LichtModiRainbow(ModiList.get(e_rainbow)->am_parameterList.get(0)->ap_parameter);
      break;
    }
      
    case e_raumlicht: {
      byte _tempIndex = ModiList.get(e_raumlicht)->am_parameterList.get(0)->ap_parameter;
      LichtModiFarbe(tempArray[_tempIndex]->am_color);
      break;
    }
      
    case e_settings: {
      Settings(ModiList.get(e_settings)->am_parameterList.get(0)->ap_parameter);
      break;
    }

    case e_dmx: {
      LichtModiDMX();
      break;
    }
      
    case e_farbpalette: {
      byte rotAnteil = ModiList.get(e_farbpalette)->am_parameterList.get(0)->ap_parameter;
      byte gruenAnteil = ModiList.get(e_farbpalette)->am_parameterList.get(1)->ap_parameter;
      byte blauAnteil = ModiList.get(e_farbpalette)->am_parameterList.get(2)->ap_parameter;
      LichtModiFarbpalette(rotAnteil, gruenAnteil, blauAnteil);
      break;
    }
      
    case e_farbe: {
      int LichtNummer = ModiList.get(e_farbe)->am_parameterList.get(0)->ap_parameter;
      LichtModiFarbe(farbArray[LichtNummer]->am_color);
      break;
    }

  }
}

// Funktion für den MODI 4 - Effektlicht
void LichtModiFunky(byte _tempo) {
  currentTime = millis();
  unsigned long newtempo = map(_tempo, 0, 100, 30, 0);

  for (int i =0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue + (i*10), 255, 255);
  }
  
  if(currentTime - previousTime >= newtempo){
    previousTime = currentTime;
    hue++;
  }

  FastLED.show();
}

// Funktion für den MODI 5 - Effektlicht
void LichtModiRainbow(byte _tempo) {
  currentTime = millis();
  unsigned long newtempo = map(_tempo, 0, 100, 30, 0);

  //farbe vergeben
  for(int j = 0; j < 10; j++) {
    leds[j] = CHSV(hue + (j*10), 255, 255);
  }

  for (int i =10; i < NUM_LEDS; i++) {
    int Zeilennummer = (i / 10) % 2;

    //Grade Zeilen füllen
    if(Zeilennummer == 0){
      leds[i] = leds[i%10];
    }

    //ungerade Zeilen füllen
    if(Zeilennummer == 1){
      leds[i] = leds[(10-1) - (i%10)];
    }
  }
  
  if(currentTime - previousTime >= newtempo){
    previousTime = currentTime;
    hue++;
  }

  FastLED.show();
}

// Funktion für das verändern zentraler Parameter (Einstellungen)
void Settings(byte _helligkeit) {
  FastLED.setBrightness(map(_helligkeit, 0, 100, 0, BRIGHTNESS));
  FastLED.show();
}

// Funktion für den MODI 2 - Farbe
void LichtModiFarbe(CRGB _color) {
  fill_solid(leds, NUM_LEDS, _color);
  FastLED.show();
}

// Funktion für den MODI 3 - Farbpalette
void LichtModiFarbpalette(byte _rot, byte _gruen, byte _blau) {

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(_rot, _gruen, _blau);
  }
  FastLED.show();
}

// Funktion für den MODI 5 - DMX
void LichtModiDMX() {
  int anfangsAdresse = ModiList.get(e_dmx)->am_parameterList.get(0)->ap_parameter;
  dmx_event_t packet;
  if (xQueueReceive(queue, &packet, DMX_PACKET_TIMEOUT_TICK))
    if (packet.status == DMX_OK)
      dmx_read_packet(dmxPort, dmxdata, packet.size);
  LichtModiFarbpalette(dmxdata[anfangsAdresse], dmxdata[anfangsAdresse+1], dmxdata[anfangsAdresse+2]);
}
