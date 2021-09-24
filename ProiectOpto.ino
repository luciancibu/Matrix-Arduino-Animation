#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <FastLED.h>
#define PIN 6

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
 
#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE     1
#define NUMPIXELS      64

//declaram sirul de pixeli folosit cu libraria FastLed
CRGB fastPixels[NUMPIXELS];

//declaram matricea de 8x8
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
                            NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
                            NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB            + NEO_KHZ800);

//declaram modululbluetooth utilizat                            
Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);     

//afisam eroarea si ramanem in bucla infinita pentru a nu se continua cat timp nu este remediata
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
 
// prototipul functiilor din packetparser.cpp utilizate pentru parsarea informatiilor primite prin bluetooth
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);
 
// bufferul in care pastram informatiile primite
extern uint8_t packetbuffer[];

//culoare predefinita
uint8_t red = 100;
uint8_t green = 100;
uint8_t blue = 100;

//starea de inceput
uint8_t animationState = 1;


void setup(void)
{
 
  //adaugam ledurile FastPixels
  LEDS.addLeds<WS2812,PIN,RGB>(fastPixels, NUMPIXELS);
  LEDS.setBrightness(80);

  //initializam starea matricii
  matrix.begin();
  matrix.setBrightness(80);

  //matricea se aprinde ca intampinare
  matrix.fillScreen(matrix.Color(red, green, blue));
  delay(1000);
  matrix.show();

//activam comunicarea seriala
  Serial.begin(115200);
  Serial.println("Proiect: Matrice de neopixeli cu animatii");
  Serial.println("------------------------------------------------");
 

  Serial.print("Initializam modulul Bluefruit LE: ");

 //pentru a se putea initializa modulul acesta trebuie sa fie setat in modul Cmd
 //daca intervine o eroare programul se opreste
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Modulul nu a fost gasit, verificati daca acesta este setat in Command Mode precum si firele!"));
  }
  Serial.println( F("Modul initializat!") );

 //se recomanda efectuarea unui reset din fabrica pentru a ne asigura ca modulul este intr-o stare consistenta, stergand setarile care persistau de la utilizarile anterioare
  if ( FACTORYRESET_ENABLE )
  {

    Serial.println("Se efectueaza resetarea din fabrica");
    if ( ! ble.factoryReset() ){
      error(F("Eroare la incercarea resetarii, va rugam inchideti fereastra si reincercati!"));
    }
  }
//dezactivam comanda echo a modulului deoarece nu o utilizam
  ble.echo(false);
 
  Serial.println("Informatii despre Bluefruit:");

  ble.info();
 
  Serial.println("Porniti aplicatia Bluefruit Connect si accesati Controller");


//setat pe false pentru a ajuta la efectuarea unui eventual debug
  ble.verbose(false);
 
//asteptam pana cand aplicatia mobila se conecteaza la modulul Bluefruit
  while (! ble.isConnected()) {
      delay(500);
  }
 
  Serial.println(F("--------------------------------------"));
  Serial.println("Acum puteti folosi color picker/controller-ul!");
  // modulul Bluefruit trebuie setat in Data mode pentru a prelua informatii
  Serial.println( F("Setati modulul in modul Data,folosind switch-ulacestuia!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);
 
  Serial.println(F("--------------------------------------"));
}

void loop(void)
{
  //citim informatiile primite de la modulul Bluefruit
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);

  //in functie de packetbuffer[1] stabilim daca am folosit color picker sau controlul butoanelor
  //culoare
  if (packetbuffer[1] == 'C') {
    
    //daca am primit o culoare citim valorile RGB si le afisam pe ecran
    red = packetbuffer[2];
    green = packetbuffer[3];
    blue = packetbuffer[4];
    Serial.print ("RGB #");
    if (red < 0x10) Serial.print("0");
    Serial.print(red, HEX);
    if (green < 0x10) Serial.print("0");
    Serial.print(green, HEX);
    if (blue < 0x10) Serial.print("0");
    Serial.println(blue, HEX);
  }
 
  // butoane
  if (packetbuffer[1] == 'B') {
    
    //daca am accesat un buton citim numarul acestuia si verificam cand este apasat
    uint8_t number = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';
    Serial.print("Button ");
    Serial.print(number);

    //starea animatiei se schimba in functie de numarul butonului apasat
    animationState = number;
    
    if (pressed) {
      Serial.println(" pressed");
    } else {
      Serial.println(" released");
    }
  }
    //daca am ajuns aici inseamna ca se cunoaste atat animatia dorita cat si culoarea

    //animatia butonului 1 in forma de ceasca de cafea si culoare primita de la modul
      if (animationState == 1){
        matrix.fillScreen(0);
        coffeeAnimation(matrix.Color(red, green, blue));
        matrix.show();
      }
    //animatia butonului 2 in forma de expresie faciala (fericire) si culoare primita de la modul
      if (animationState == 2){
        matrix.fillScreen(0);
        happyAnimation(matrix.Color(red, green, blue));
        matrix.show(); 
      }

      //animatia butonului 3 (sageata in jos) afiseaza multiple puncte cu culori generate aleator
      if (animationState == 3){ 
        matrix.fillScreen(0);
        dotsAnimation();
        matrix.show();
      }

       //animatia butonului 4 (sageata la stanga)foloseste libraria FastLed si coloreaza succesiv liniile matricii
      if (animationState == 4){ 
        lineAnimation();
      }

         
}


//functii folosite la crearea animatiei de tip cana de cafea
void Coffee1(uint32_t c) {
  matrix.drawLine(3, 1, 3, 7, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(3, 7, 5, 7, c);
  matrix.drawLine(7, 3, 7, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 5, c);
  matrix.drawPixel(4, 6, c);
  matrix.drawPixel(5, 1, c);
}

void Coffee2(uint32_t c) {
  matrix.drawLine(3, 1, 3, 7, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(3, 7, 5, 7, c);
  matrix.drawLine(7, 3, 7, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 5, c);
  matrix.drawPixel(4, 6, c);
  matrix.drawPixel(5, 1, c);
  matrix.drawPixel(2, 2, c);
  matrix.drawPixel(2, 5, c);
}
void Coffee3(uint32_t c) {
  matrix.drawLine(3, 1, 3, 7, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(3, 7, 5, 7, c);
  matrix.drawLine(7, 3, 7, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 5, c);
  matrix.drawPixel(4, 6, c);
  matrix.drawPixel(5, 1, c);
  matrix.drawPixel(2, 2, c);
  matrix.drawPixel(2, 5, c);
  matrix.drawPixel(1, 3, c);
  matrix.drawPixel(1, 6, c);
}

void Coffee4(uint32_t c) {
  matrix.drawLine(3, 1, 3, 7, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(3, 7, 5, 7, c);
  matrix.drawLine(7, 3, 7, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 5, c);
  matrix.drawPixel(4, 6, c);
  matrix.drawPixel(5, 1, c);
  matrix.drawPixel(2, 2, c);
  matrix.drawPixel(2, 5, c);
  matrix.drawPixel(1, 3, c);
  matrix.drawPixel(1, 6, c);
  matrix.drawPixel(0, 2, c);
  matrix.drawPixel(0, 5, c);
}

//animatia 1: la fiecare jumatate de secunda se afiseaza cate o etapa a animatiei
void coffeeAnimation(uint32_t c) {
  matrix.fillScreen(0);
  Coffee1(c);
  matrix.show();
  delay(500);
  matrix.fillScreen(0);
  Coffee2(c);
  matrix.show();
  delay(500);
  matrix.fillScreen(0);
  Coffee3(c);
  matrix.show();
  delay(500);
  matrix.fillScreen(0);
  Coffee4(c);
  matrix.show();
  delay(500);
}

//functii folosite la crearea animatiei de tip expresie faciala
void Face1(uint32_t c) {
  matrix.drawLine(0, 2, 0, 5, c);
  matrix.drawLine(7, 2, 7, 5, c);
  matrix.drawLine(2, 7, 3, 7, c);
  matrix.drawLine(2, 0, 3, 0, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(5, 7, 4, 7, c);
  matrix.drawPixel(1, 1, c);
  matrix.drawPixel(1, 6, c);
  matrix.drawPixel(2, 2, c);
  matrix.drawPixel(2, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 6, c);
}

void Face2(uint32_t c) {
  matrix.drawLine(0, 2, 0, 5, c);
  matrix.drawLine(7, 2, 7, 5, c);
  matrix.drawLine(2, 7, 3, 7, c);
  matrix.drawLine(2, 0, 3, 0, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(5, 7, 4, 7, c);
  matrix.drawPixel(1, 1, c);
  matrix.drawPixel(1, 6, c);
  matrix.drawPixel(2, 2, c);
  matrix.drawPixel(2, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 6, c);
  matrix.drawLine(5, 3, 5, 4, c);
}

void Face3(uint32_t c) {
  matrix.drawLine(0, 2, 0, 5, c);
  matrix.drawLine(7, 2, 7, 5, c);
  matrix.drawLine(2, 7, 3, 7, c);
  matrix.drawLine(2, 0, 3, 0, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(5, 7, 4, 7, c);
  matrix.drawPixel(1, 1, c);
  matrix.drawPixel(1, 6, c);
  matrix.drawPixel(2, 2, c);
  matrix.drawPixel(2, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 6, c);
  matrix.drawLine(5, 3, 5, 4, c);
  matrix.drawPixel(4, 2, c);
  matrix.drawPixel(4, 5, c);
}

void Face4(uint32_t c) {
  matrix.drawLine(0, 2, 0, 5, c);
  matrix.drawLine(7, 2, 7, 5, c);
  matrix.drawLine(2, 7, 3, 7, c);
  matrix.drawLine(2, 0, 3, 0, c);
  matrix.drawLine(4, 0, 5, 0, c);
  matrix.drawLine(5, 7, 4, 7, c);
  matrix.drawPixel(1, 1, c);
  matrix.drawPixel(1, 6, c);
  matrix.drawPixel(2, 2, c);
  matrix.drawPixel(2, 5, c);
  matrix.drawPixel(6, 1, c);
  matrix.drawPixel(6, 6, c);
  matrix.drawLine(5, 3, 5, 4, c);
  matrix.drawPixel(4, 2, c);
  matrix.drawPixel(4, 5, c);
  matrix.drawPixel(5, 2, c);
  matrix.drawPixel(5, 5, c);
}

//animatia 2: folosita la afisarea unei fete care incepe sa zambeasca
void happyAnimation(uint32_t c) {
  matrix.fillScreen(0);
  Face1(c);
  matrix.show();
  delay(500);
  matrix.fillScreen(0);
  Face2(c);
  matrix.show();
  delay(500);
  matrix.fillScreen(0);
  Face3(c);
  matrix.show();
  delay(500);
  matrix.fillScreen(0);
  Face4(c);
  matrix.show();
  delay(500);
}
//animatia 3: de tip puncte genereaza aprinderea ledurilor intr-o culoare si la pozitie aleatoare
void dotsAnimation() {
  for(int j = 0; j < 8; j++)
  for(int i = 0 ; i < 64 ; i++){
      int randX = random(0, 8);
      int randY = random(0, 8);
      int randR1 = random(0, 255);
      int randG1 = random(0, 255);
      int randB1 = random(0, 255);
      matrix.drawPixel(randX, randY, matrix.Color(randR1, randG1, randB1));
  }
}

//animatia 4
void lineAnimation(){
  int hue = 0;
  //pentru fiecare led din cei NUMPIXELS existenti setam culoarea si incrementam nuanta pentru configurarea unui efect de trecere gradata
  for(int i = 0; i < NUMPIXELS; i++) {
    //seteaza culoarea ledului i
    fastPixels[i] = CHSV(hue++, 255, 255);
    //afiseaza schimbarile ledurilor
    FastLED.show(); 
    delay(10);
  }

  //dupa setarea tuturor ledurilor se reincepe in ordine inversa 
  for(int i = NUMPIXELS - 1; i >= 0; i--) {
    fastPixels[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    delay(10);
  }
}
