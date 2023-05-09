/*
 ******************************************************************
 ******************************************************************
 *                                                                *
 * Einrichtung: Hochschule für Angewandten Wissenschaften (HAW)   *
 * Anschrift:  22081 Hamburg/ Finkenau 35                         *
 * Projekt: Effect Light TV                                       *
 *                                                                *
 * Autor: Felix Ohlendorf, Samantha Schoppa                       *
 * Version: 1.0                                                   *
 *                                                                *
 ******************************************************************
 ******************************************************************
 */

/*
 *
 * Teil 1: Einbinden der Bibliotheken als Header-Datei
 *
 */

#include <arduino.h>
#include <esp_dmx.h>            // Bibliothek für den DMX Modi
#include <LiquidCrystal_I2C.h>  // Bibliothek für die LCD Anzeige
#include <LinkedList.h>         // Bibliothek für die Datenstruktur des Menüs
#include <IRremote.h>           // Bibliothek für die Verwendung der IR-Fernbedienung
#include <Wire.h>               // Bibliothek wird für LCD I2C benötigt
#include <FastLED.h>            // Bibliothek für die Nutzung eines LED Streifens

/*
 *
 * Teil 2: Definition wichtiger Makros
 *
 */

// Sensor Makros
#define IRRECIEVER_DATA_PIN 19  // Der Datenpin des IR Receivers liegt auf der 19

// Menü Makros

// Die Namen der Modis wurden hier vorab als Makros definiert.
#define MODI_DMX "DMX"
#define MODI_FUNKY "Funky"
#define MODI_RAINBOW "Rainbow"
#define MODI_RAUM "Raumlicht"
#define MODI_FARBEPALETTE "Farbpalette"
#define MODI_FARBE "Farbe"
#define MODI_SETTINGS "Einstellungen"
#define MODI_PIXELART "Pixelart"

// Licht Makros
#define NUM_LEDS 100      // Anzahl der Pixel muss bei neuem Gehäuse angepasst werden.
#define LED_ROWS 10       // Anzahl der Spalten der LED Matrix
#define LED_LINES 10      // Anzahl der Zeilen der LED Matrix
#define LED_TYPE WS2812B  // Typ der LEDs auf dem Streifen sind vom Typ WS2812B 
#define COLOR_ORDER GRB
#define BRIGHTNESS 192    // Die maximale Brightness sollte generell nicht mehr als 192 von 255 betragen
#define DATA_PIN 5        // Der Datenpin an dem die LED Matrix angeschlossen ist, ist die Nummer 5


/*
 *
 * Teil 3: Definition alle Datenstrukturen und Variablen
 *
 */

// Für die Modis wurde ein enum erstellt.
// Hierbei steht eine Zahl für ein Name innerhalb des enums für eine Zahl und um genauer zu sein für die Position des Modis.
enum e_Modi{
  e_raumlicht,
  e_farbe,
  e_farbpalette,
  e_funky,
  e_rainbow,
  e_pixelart,
  e_settings,
  e_dmx
};

/*

Generell soll dem ganzen System eine Art "Datenbank" zugrunde liegen. Das oberste Ziel eines jeden Programms ist die Modularität.
Das bedeutet zum einen eine einfache Erweiterbarkeit und dennoch Geschlossenheit. Gleichzeitig ist es wichtig innerhalb des Programms
möglichst kleinschrittig zu areiten. Damit sollte das der Code möglichst einfach in Funktionen zerlegt werden.
Damit diese Ziele erreicht werden liegt als Datenstruktur der Datenbank das Modell einer Liste zugrunde. Eine Liste besteht aus beliebig vielen Elementen.
Diese Liste kann jeder Zeit um ein Element ergänzt werden.

Nach der Zerlegen der Funktionen aus dem Pflichtenheft kann die Struktur auf folgende Klassen zurückgeführt werden.
Generell gibt es Modi. Diese lassen sich mit einem Namen und beliebig vielen Parametern beschreiben.
Diese beliebig viele Parameter wurden als Liste realisiert.

Ein Parameter wird zum einen ebenfalls mit einem Namen beschriben. Weitere Attribute sind der Wert des Parameter.
Dabei kann ein Parameter einen minimalen und maximalen Wert aufweisen. Wenn der Nutzende später mit der Fernbedienung die Werte verändert
kann ein Einstellen der Farbe zwischen 0 bis 255 mit einer Schrittweite von 1 sehr lange Dauern.
Aus diesem Grund wird dem dem Parameter eine variable Schrittweite mitgegeben.

*/

class Parameter{
  public:
    String ap_title;
    byte ap_parameter;
    byte ap_max, ap_min, ap_steps;

    Parameter(String title, byte parameter, byte max = 255, byte min = 0, byte steps = 5) {
      // Der Konstruktor beinhaltet die 5 Attribute, die gesetzt werden können.
      // Dabei ist der minimal, maximal Wert sowei die schrittweite Standardmäßig gesetzt.
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

// Variablen für die Hardwarekomponenten
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variablen für die Menustruktur
LinkedList<Modi*> ModiList = LinkedList<Modi*>(); // Bildet die Liste aller Modis
int p_ModiPosition{e_raumlicht};    // Variable für die aktuelle Position in der Modiliste.
                                    // Wird initialisiert mit dem Startmodi Raumlicht.
int p_ParameterPosition{0};         // Die Position in der Parameterliste eines Modis soll standardmäßig auf 0 sein.
bool in_Eingabe{false};

// Variablen für Licht

CRGB leds[NUM_LEDS];  // Bildet den Array über alle LEDS und deren Farbzustand.

//Variablen für die Bildausgabe

// Die Arrays HAW_Logo, Mario und Luigi sind Bilder die auf der LED Matrix dargestellt werden können.
// Jedes Element ist ein 32bit Codierter Farbcode im HEX-Code.
uint32_t HAW_Logo[] = {
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

uint32_t Mario[] = {
  0xFFFFFF, 0xFFFFFF, 0xFF0000, 0xFF0000, 0xFFFFFF, 0xFFFFFF, 0xFF0000, 0xFF0000, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFFFFFF,
  0xFFFFFF, 0x8B4513, 0xDEB887, 0x191970, 0xDEB887, 0xDEB887, 0x191970, 0xDEB887, 0x8B4513, 0xFFFFFF,
  0xFFFFFF, 0xDEB887, 0xDEB887, 0x8B4513, 0x8B4513, 0x8B4513, 0x8B4513, 0xDEB887, 0xDEB887, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0xDEB887, 0xDEB887, 0xDEB887, 0xDEB887, 0xDEB887, 0xDEB887, 0xFFFFFF, 0xFFFFFF,
  0xDEB887, 0xFF0000, 0x0000FF, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0x0000FF, 0xFF0000, 0xDEB887,
  0xDEB887, 0xFF0000, 0xFFFF00, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFF00, 0xFF0000, 0xDEB887,
  0xFFFFFF, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0x8B4513, 0x8B4513, 0xFFFFFF, 0xFFFFFF, 0x8B4513, 0x8B4513, 0xFFFFFF, 0xFFFFFF
};

uint32_t Luigi[] = {
  0xFFFFFF, 0xFFFFFF, 0x00FF00, 0x00FF00, 0xFFFFFF, 0xFFFFFF, 0x00FF00, 0x00FF00, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0xFFFFFF,
  0xFFFFFF, 0x000000, 0xDEB887, 0x191970, 0xDEB887, 0xDEB887, 0x191970, 0xDEB887, 0x000000, 0xFFFFFF,
  0xFFFFFF, 0xDEB887, 0xDEB887, 0x000000, 0x000000, 0x000000, 0x000000, 0xDEB887, 0xDEB887, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0xDEB887, 0xDEB887, 0xDEB887, 0xDEB887, 0xDEB887, 0xDEB887, 0xFFFFFF, 0xFFFFFF,
  0xDEB887, 0x00FF00, 0x0000FF, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x0000FF, 0x00FF00, 0xDEB887,
  0xDEB887, 0x00FF00, 0xFFFF00, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFF00, 0x00FF00, 0xDEB887,
  0xFFFFFF, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0x8B4513, 0x8B4513, 0xFFFFFF, 0xFFFFFF, 0x8B4513, 0x8B4513, 0xFFFFFF, 0xFFFFFF
};

enum e_Bilder {
  e_HAW,
  e_Mario,
  e_Luigi
};

// Bilder wurde an dieser Stelle wieder als separate Klasse angelegt.
// Jedes Bild kann durch einen Titel und eine oben abgebildete Matrix abgebildet werden.
class Bilder {
  public:
    String ab_title;
    uint32_t ab_Pixelarray[NUM_LEDS];
  // Generell soll der Konstruktor wie folgt aussehen:
  Bilder(String title, uint32_t* pixels);
};

// Das zuweisen des Bildarrays zum Pixelarray war im Konstruktor so nicht möglich, wodurch dieser hier nochmal überladen wird.
// Dabei wird jedes Pixel einzeln von A nach B übertragen.
Bilder::Bilder(String title, uint32_t* pixels){
    ab_title = title;

    for(int i = 0; i < NUM_LEDS; i++) {
      ab_Pixelarray[i] = pixels[i];
    }
}

// Hier wird das Bild Array mit allen möglichen Pixelgrafik erzeugt.
// Eine Ergänzung des Pixelart Modis findet somit über ein zusätzliches Element statt.
Bilder* Bilderarray[] = {
  new Bilder("HAW", HAW_Logo),
  new Bilder("Mario", Mario),
  new Bilder("Luigi", Luigi)
};

// Variablen für den Rainboweffekt
uint8_t hue{0};
unsigned long previousTime{0}, currentTime{0};

//Variablen für DMX
int RX_Pin{16};                     // Der Pin auf dem die Serielle Kommunikation über das MAX485 Modul stattfindet ist der Pin 16
dmx_port_t dmxPort{1};              // Der DMX Port ist standardmäßig immer auf 1 zu setzen.
byte dmxdata [DMX_MAX_PACKET_SIZE]; // Hier wird ein Array mit den übermittelten DMX-Daten initialisiert. Dabei meint die Packetsize 513 Elemente.
                                    // Dabei wird immer ein Universium übertragen. 0. Element ist das Startbyte eines Universums.
QueueHandle_t queue;                // Bei der Seriellen Übertragung müssen die Daten in einen Buffer geschrieben werden. Hier als Queue realisiert.
bool dmxIsConnected{false};         // Dieses dient als Flag zur Beschreibung des Verbindungszustands.

// Datenstrukturen für Lichtprogrammierung

//Eine weitere Klasse bildet die Color-Class. Eine Color kann wieder durch einen Namen und eine Farbe CRGB beschrieben werden.
class Color{
  public:
    String am_title;
    CRGB am_color;

  Color(String title, CRGB color) {
    am_title = title;
    am_color = color;
  }
};

// Farbarrays

// In den Farbmodis, bei denen der Nutzende statische Farben gewählt werden können, dienen die Arrays als Vorgabe.
// Jedes Element der Arrays ist eine Farbe, die später ausgewählt werden kann.
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

/*
 *
 * Teil 5: Definition von Funktionsprototypen
 *
 */

// Menu Funktionsprototyp
void InitilizeLCD();
void InitilizeMenu();
void InitilizeLight();
void InitilizeDMX();
void PrintModi(int, int);

// Licht Funktionsprototyp
void printLogo();
void RunModi(byte);
void LichtModiRainbow(byte);
void LichtModiRaum(byte);
void LichtModiFarbe(CRGB);
void LichtModiFarbpalette(byte , byte, byte);
void LichtModiPixelart(uint32_t*);
void Settings(byte);

/*
 *
 * Teil 6: Setup Funktion
 *
 */

void setup() {
  InitilizeLCD();
  InitilizeLight();
  InitilizeMenu();
  InitilizeDMX();
}

/*
 *
 * Teil 7: Loop Funktion
 *
 */

void loop() {

  // In der Loop Funktionen findet die vollständige Steuerung der Lampe statt.

  // Dabei wird nach folgender Logik vorgegangen.
  // Wenn eine Eingabe getätigt wird, dekodiere diese - Ansonsten führe Führe den Modi an der der aktuellen Positionen mit den Parametern aus.
  if(IrReceiver.decode()) { 

      // Navigation durch die ModiListe

      if (IrReceiver.decodedIRData.address == 0x0) { // Wenn die Fernbedienung die Adresse 0x0 besitzt führe folgendes aus...
        
        if (IrReceiver.decodedIRData.command == 0x40) { // Mit dem OK Button wird eine eingabe getätigt.
          in_Eingabe = !in_Eingabe;                     // Nur mit einer Eingabe (== true) kann eine Veränderung der Parameter stattfinden.
        }
        
        if(!in_Eingabe) { 
          // Wenn ich keine Eingabe getätigt habe soll folgendes geschehen:
          // Eine Veränderung der Parameter soll nicht möglich sein.
          switch(IrReceiver.decodedIRData.command) {
                        
            case 0x43: {
              // Mit dem Button "Down" werden die Modi nacheinander abwärts durchgegangen. Dabei wird jeweils beim Ende nicht weitergegangen.
              p_ModiPosition++;
              if (p_ModiPosition >= ModiList.size()){
                p_ModiPosition = ModiList.size() - 1;
              }
              p_ParameterPosition = 0;
              in_Eingabe = false;
              break;              
            }

            case 0x44: {
              // Mit dem Button "Up" werden die Modi nacheinander aufwärtsdurchgegangen. Dabei wird jeweils beim Anfang nicht weitergegangen.
              p_ModiPosition --;
              if (p_ModiPosition <= 0){
                p_ModiPosition = 0;
              }
              p_ParameterPosition = 0;
              in_Eingabe = false;
              break;
            }

            case 0x09: {
              // Navigation in der ParameterListe aufwärts. Dabei sollen die Pfeile nach "Rechts" dienen.
              p_ParameterPosition++;
              if(p_ParameterPosition >= ModiList.get(p_ModiPosition)->am_parameterList.size()) {
                p_ParameterPosition = 0;
              }
              break;
            }

            case 0x07: {
              // Navigation in der ParameterListe abwärts. Dabei sollen die Pfeile nach "Links" dienen.
              p_ParameterPosition--;
              if(p_ParameterPosition <= 0) {
                p_ParameterPosition = ModiList.get(p_ModiPosition)->am_parameterList.size() - 1;
              }
              break;       
            }
          }             
        }

        if(in_Eingabe) {
          // Wenn ich eine Eingabe getätigt habe soll folgendes geschehen:
          // Soll nur eine Veränderung der Parameter soll möglich sein.
          int eingabeWert = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter;
          int maximalerWert = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_max;
          int minimalerWert = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_min;
          int schritte = ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_steps;

          switch(IrReceiver.decodedIRData.command) {
            case 0x46: {
              // Mit dem Vol+ Button soll der Wert des Parameters mit der dazugehörigen Schrittweite Inkrementiert werden.
              eingabeWert = eingabeWert + schritte;
              if(eingabeWert > maximalerWert) {
                eingabeWert = maximalerWert;
              }
              ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter = eingabeWert;
              break;
            }

            case 0x15: {
              // Mit dem Vol- Button soll der Wert des Parameters mit der dazugehörigen Schrittweite Dekrementiert werden.
              eingabeWert = eingabeWert - schritte;
              if(eingabeWert < minimalerWert) {
                eingabeWert = minimalerWert;
              }
              ModiList.get(p_ModiPosition)->am_parameterList.get(p_ParameterPosition)->ap_parameter = eingabeWert;
            }
          }
        }
      }
      // Am Ende soll der aktuelle Menüpunkt auf den LCD Display ausgegeben werden.
      PrintModi(p_ModiPosition, p_ParameterPosition);
      delay(200);
      IrReceiver.resume();
    } else {
      // Wenn keine Navigation stattfindet, soll der aktuelle Modi ablaufen.
      RunModi(p_ModiPosition);
    }
}

/*
 *
 * Teil 8: Die Definierten Funktionen
 *
 */

// Das LCD Display wird initialisiert.
void InitilizeLCD() {
  IrReceiver.begin(IRRECIEVER_DATA_PIN);
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Willkommen beim");
  lcd.setCursor(0, 1);
  lcd.print("Effect Light TV");
}

// Das Menü wird initialisiert. Jeder Menüpunkt wird erzeugt inkl. der Parameter.
// Dabei kann jederzeit ohne große Mühe ein Modi ergänzt werden.
void InitilizeMenu() {

  // MODI 1 - Raumlicht
  // MODI Raumlicht wird beschrieben durch die Farbe hergeleitet durch die Farbtemperatur.
  Modi *raumlicht = new Modi();
  raumlicht->am_title = MODI_RAUM;
  // Notiz: new Parameter ("Titel", Startwert, maximaler Wert, minimaler Wert, Schrittweite);
  raumlicht->am_parameterList.add(new Parameter("Temp.", 0, (sizeof(tempArray)/sizeof(tempArray[0]))-1, 0, 1));
  ModiList.add(raumlicht);

  // MODI 2 - Farbe
  // MODI Farbe wird durch eine Beliebige Farbe beschrieben
  Modi *farbe = new Modi();
  farbe->am_title = MODI_FARBE;
  farbe->am_parameterList.add(new Parameter("Farbe", 0, (sizeof(farbArray)/sizeof(farbArray[0]))-1, 0, 1));
  ModiList.add(farbe);

  // MODI 3 - Farbpalette
  // MODI Farbpalette wird durch die drei Komponenten RGB beschrieben. Dabei kann jeder Parameter einzeln eingestellt werden.
  Modi *farbPalette = new Modi();
  farbPalette->am_title = MODI_FARBEPALETTE;
  farbPalette->am_parameterList.add(new Parameter("Rot", 0));
  farbPalette->am_parameterList.add(new Parameter("Gruen", 0));
  farbPalette->am_parameterList.add(new Parameter("Blau", 0));
  ModiList.add(farbPalette);

  // MODI 4 - Effektlicht
  // MODI Effektlicht 1 bildet den Funky Modi und besitzt den Parameter Tempo.
  Modi *funky = new Modi();
  funky->am_title = MODI_FUNKY;
  funky->am_parameterList.add(new Parameter("Tempo(%)", 10, 100, 10, 10));
  ModiList.add(funky);

  // MODI 5 - Effektlicht
  // MODI Effektlicht 2 bildet den Rainbow Modi und besitzt den Parameter Tempo.
  Modi *rainbow = new Modi();
  rainbow->am_title = MODI_RAINBOW;
  rainbow->am_parameterList.add(new Parameter("Tempo(%)", 10, 100, 10, 10));
  ModiList.add(rainbow);

  // MODI 6 - Pixelart
  // MODI Pixelart kann ein Bild aus dem vorher definierten Array darstellen.
  Modi *pixelart = new Modi();
  pixelart->am_title = MODI_PIXELART;
  pixelart->am_parameterList.add(new Parameter("Figur", 0, (sizeof(Bilderarray)/sizeof(Bilderarray[0])) - 1, 0, 1));
  ModiList.add(pixelart);

  // Einstellungen (Wird aber als Modi der Datenstruktur hinzugefügt)
  // In den Einstellungen besteht die Möglichkeit die Gesamthelligkeit anzupassen.
  Modi *settings = new Modi();
  settings->am_title = MODI_SETTINGS;
  settings->am_parameterList.add(new Parameter("Hellig.(%)", 100, 100, 0, 10));
  ModiList.add(settings);

  // MODI 7 - DMX
  // MODI DMX benötigt eine Anfangsadresse.
  Modi *dmx = new Modi();
  dmx->am_title = MODI_DMX;
  dmx->am_parameterList.add(new Parameter("Adresse", 1, 512, 1, 1));
  ModiList.add(dmx);

  // Am Ende wird die Startposition gesetzt.
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

  // Für die Ausgabe auf dem Display werden hier Sonderregeln definiert.
  switch (positionModi) {
    case e_raumlicht: {
      // Beim Raumlicht sollen die Farbtemperaturen dargestellt werden.
      lcd.print(tempArray[positionLicht]->am_title);
      break;
    }
    case e_farbe: {
      // Beim Farblicht sollen die Namen der Farben dargestellt werden.
      lcd.print(farbArray[positionLicht]->am_title);
      break;   
    }

    case e_pixelart: {
      // Bei Pixelart sollen die Namen der Bilder dargestellt werden.
      lcd.print(Bilderarray[positionLicht]->ab_title);
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
    queueSize,
    &queue,
    interruptPriority);
  printLogo();
}

// Initaliiserung der Strips
void InitilizeLight() {
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);  // maximale Lichtstärke setzen
  previousTime = currentTime;
  printLogo();
  delay(2000);
}

// Ausgabe des Logos auf der Matrix
void printLogo() {

  uint32_t mapLogo[NUM_LEDS]; // Als zwischenspeicher ein leeres Array

  for (int i = 0; i < LED_LINES; i++) {   //Dabei soll erst über jede Element in einer Zeile iteriert werden 
    for (int j = 0; j < LED_ROWS; j++) {  // und das ganze dann Spaltenweise
      if(i % 2) {
        // Jede zweite Zeie soll umgedreht werden.
        mapLogo[(i * LED_ROWS) + j] = HAW_Logo[(i * LED_ROWS) + ((LED_ROWS - 1) - j)];
      } else {
        // Jede andere Zeie soll bleibt unverändert.
        mapLogo[(i * LED_ROWS) + j] = HAW_Logo[(i * LED_ROWS) + j];
      }
    }
  }

  // Am Ende wird das gesamte Bild nochmal gespiegelt und auf den Kopf gestellt.
  // Grund hierfür ist die Verkabelung der LED Streifen als Matrix.
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = mapLogo[(NUM_LEDS - 1) - i];   
  }
  
  FastLED.show();
}

// Hier soll entschieden werden, welcher Modi getriggert werden soll.
void RunModi(byte _position) {

  // In dieser Funktionen wird die Position mithilfe des Anfangsdefinierten Enums aufgelöst und die jeweilige Funktion ausgelöst.
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

    case e_pixelart: {
      byte Bildposition = ModiList.get(e_pixelart)->am_parameterList.get(0)->ap_parameter;
      LichtModiPixelart(Bilderarray[Bildposition]->ab_Pixelarray);
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
  for(int j = 0; j < LED_ROWS; j++) {
    leds[j] = CHSV(hue + (j*10), 255, 255);
  }

  for (int i = 10; i < NUM_LEDS; i++) {
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

// Funktion für den MODI 6 - Pixelart
void LichtModiPixelart(uint32_t* _bildArray) {
  uint32_t mapLogo[NUM_LEDS];

  for (int i = 0; i < LED_LINES; i++) {
    for (int j = 0; j < LED_ROWS; j++) {
      if(i % 2) {   
        mapLogo[(i * LED_ROWS) + j] = _bildArray[(i * LED_ROWS) + ((LED_ROWS - 1) - j)];
      } else {
        mapLogo[(i * LED_ROWS) + j] = _bildArray[(i * LED_ROWS) + j];
      }
    }
  }

  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = mapLogo[(NUM_LEDS - 1) - i];   
  }
  
  FastLED.show();
}

// Funktion für den MODI 5 - DMX
void LichtModiDMX() {
  int anfangsAdresse = ModiList.get(e_dmx)->am_parameterList.get(0)->ap_parameter;
  dmx_event_t packet;
  if (xQueueReceive(queue, &packet, DMX_PACKET_TIMEOUT_TICK)) {
    if (packet.status == DMX_OK) {
      dmx_read_packet(dmxPort, dmxdata, packet.size);
    }
  }
  LichtModiFarbpalette(dmxdata[anfangsAdresse], dmxdata[anfangsAdresse + 1], dmxdata[anfangsAdresse + 2]);
}