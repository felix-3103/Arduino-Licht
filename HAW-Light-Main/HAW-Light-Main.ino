//Libraries für das Menü
#include <LiquidCrystal_I2C.h>
#include <LinkedList.h>
#include <IRremote.h>
#include <Wire.h>
#include <FastLED.h>

//Sensor Makros
#define IRRECIEVER_DATA_PIN 19

//Menu Makros
#define MODI_DMX "DMX"
#define MODI_EFFEKT "Effektlicht"
#define MODI_RAUM "Raumlicht"
#define MODI_FARBE "Farbe"

//Light Makro
#define NUM_LEDS 60      // Anzahl der Pixel
#define LED_TYPE WS2812B // IC Typ
#define COLOR_ORDER GRB  // typische Farbreihenfolge GRB
#define BRIGHTNESS 127   // Lichtstärke x von 255
#define DATA_PIN 5     // Data In


//Variablen und Datenstrukturen für die Menüstruktur
enum e_Modi{
  e_raumlicht,
  e_farblicht,
  e_effekt,
  e_dmx
};

class Parameter{
  public:
    String ap_title;
    int ap_parameter;

    Parameter(String title, int parameter) {
      ap_title = title;
      ap_parameter = parameter;
    }
};

class Modi{
  public:
    String am_title;
    LinkedList<Parameter*> am_parameterList;
};

//Variablen für die Hardwarekomponenten
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Variablen für die Menustruktur
LinkedList<Modi*> ModiList = LinkedList<Modi*>();
int p_ModiPosition = e_raumlicht;
int p_ParameterPosition = 0;
bool in_Eingabe = false;

//Variablen für Licht
CRGB leds[NUM_LEDS];     // Initialisierung Array CRGB

//Menu Funktionsprototyp
void InitilizeLCD();
void InitilizeMenu();
void InitilizeLight();
void PrintModi(int, int);

//TaskHandler für die Parallelisierung
TaskHandle_t TaskMenu;
TaskHandle_t TaskLight;

int counter = 0;


// the setup function runs once when you press reset or power the board
void setup() {
  //Initialiserung der Hardware
  Serial.begin(115200);
  InitilizeLCD();
  InitilizeMenu();
  InitilizeLight();
}

void loop() {

  if(IrReceiver.decode()) { 

      //Navigation durch die ModiListe
      if (IrReceiver.decodedIRData.address == 0x0) {
        
        if (IrReceiver.decodedIRData.command == 0x15) {
          in_Eingabe = !in_Eingabe;
        }
        
        if(!in_Eingabe) {
          if (IrReceiver.decodedIRData.command == 0x19) {
            //Mit dem Button "Down" werden die Modi nacheinander abwärts durchgegangen. Dabei wird jeweils beim Ende nicht weitergegangen.
            p_ModiPosition++;

            if (p_ModiPosition >= ModiList.size()){
              p_ModiPosition = ModiList.size() - 1;
            }

            p_ParameterPosition = 0;
            in_Eingabe = false;
          }

          if (IrReceiver.decodedIRData.command == 0x40) {
            //Mit dem Button "Up" werden die Modi nacheinander aufwärtsdurchgegangen. Dabei wird jeweils beim Anfang nicht weitergegangen.
            p_ModiPosition --;

            if (p_ModiPosition <= 0){
              p_ModiPosition = 0;
            }

            p_ParameterPosition = 0;
            in_Eingabe = false;
          }

          if (IrReceiver.decodedIRData.command == 0x09) {
            //Navigation in der ParameterListe. Soll nur in eine Richtung gehen.
            p_ParameterPosition++;
            if(p_ParameterPosition >= ModiList.get(p_ModiPosition)->am_parameterList.size()) {
              p_ParameterPosition = 0;
            }
          }          
        }


        if(in_Eingabe) {
          int eingabeWert = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter;

          if(IrReceiver.decodedIRData.command == 0x40) {
            eingabeWert = eingabeWert + 5;

            if(eingabeWert > 255) {
              eingabeWert = 255;
            }

            ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter = eingabeWert;
          }

          if(IrReceiver.decodedIRData.command == 0x19) {
            eingabeWert = eingabeWert - 5;

            if(eingabeWert < 0) {
              eingabeWert = 0;
            }

            ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter = eingabeWert;
          }
        }
      }

      //Am Ende soll der aktuelle Menüpunkt auf den LCD Display ausgegeben werden.
      PrintModi(p_ModiPosition, p_ParameterPosition);
      delay(100);
      IrReceiver.resume();
    } else {
      switch (p_ModiPosition) {
        case e_effekt:
          LichtModiRainbow();
          break;
        case e_raumlicht:
          LichtModiRaum();
          break;
        default:
          break;
      }
    }
}

//Hier soll die LCD Anzeige Initalisiert werden.
void InitilizeLCD() {
  Serial.begin(9600);
  IrReceiver.begin(IRRECIEVER_DATA_PIN);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Willkommen ;)");
  lcd.setCursor(0, 1);
  lcd.print("HAW-Light");
  delay(2000);
}

//Hier soll das Menü initalisiert werden. Jeder Menüpunkt wird erzeugt inkl. der Parameter.
//Dabei kann jederzeit ohne große Mühe ein Modi ergänzt werden.
void InitilizeMenu() {

  //MODI 1 Raumlicht
  Modi *raumlicht = new Modi();
  raumlicht->am_title = MODI_RAUM;
  raumlicht->am_parameterList.add(new Parameter("Helligkeit", 100));
  ModiList.add(raumlicht);

  //MODI 2 Farblicht
  Modi *farbe = new Modi();
  farbe->am_title = MODI_FARBE;
  farbe->am_parameterList.add(new Parameter("Rot", 0));
  farbe->am_parameterList.add(new Parameter("Gruen", 0));
  farbe->am_parameterList.add(new Parameter("Blau", 0));
  farbe->am_parameterList.add(new Parameter("Dimmen", 0));
  ModiList.add(farbe);

  //MODI 3 Effektlicht
  Modi *effekt = new Modi();
  effekt->am_title = MODI_EFFEKT;
  effekt->am_parameterList.add(new Parameter("Tempo", 0));
  ModiList.add(effekt);

  //MODI 4 DMX
  Modi *dmx = new Modi();
  dmx->am_title = MODI_DMX;
  ModiList.add(dmx);

  //Startposition setzen
  PrintModi(p_ModiPosition, p_ParameterPosition);
}

//Menu Funktionen
//Hiermit soll der Modi über das Display ausgegeben werden.
void PrintModi(int positionModi, int positionParameter) {
  lcd.clear();
  lcd.print("MODI: " + ModiList.get(positionModi)->am_title);
    
  if(positionModi != e_dmx){
    lcd.setCursor(0,1);
    lcd.print(ModiList.get(positionModi)->am_parameterList.get(positionParameter)->ap_title + ":");
    lcd.setCursor(16-4,1);
    lcd.print(ModiList.get(positionModi)->am_parameterList.get(positionParameter)->ap_parameter);
  }
}

//Lichtfunktionen
//Initaliiserung der Strips
void InitilizeLight() {
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);// Lichtstärke setzen
}

void LichtModiRainbow() {
  uint8_t beatA = beatsin8(30, 0, 255);
  uint8_t beatB = beatsin8(30, 0, 255);
  fill_rainbow(leds, NUM_LEDS, (beatA+beatB)/2, 8);
  FastLED.show();
}

void LichtModiRaum() {
  fill_solid(leds, NUM_LEDS, CRGB::AntiqueWhite);
  FastLED.show();
}