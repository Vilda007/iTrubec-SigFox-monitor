float myTeplota1, myTeplota2, myTeplota3, myTlak1, myVlhkost1, Vcc; //programove globalni promenne
char zprava[12]; //SigFox zpráva
int t1, t2, t3, v1, p1, n1; //SigFox proměnné

// potřebné knihovny
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <Adafruit_BME280.h>
#include "Wire.h"
#include <DallasTemperature.h>

// nastavení projojovacích pinů SigFox modulu
#define TX 8
#define RX 7

// inicializace softwarové sériové linky z knihovny
SoftwareSerial Sigfox(RX, TX);
// nastaveni adresy senzoru
#define BME280_ADRESA (0x76)
// inicializace senzoru BME z knihovny
Adafruit_BME280 bme;

//Nastaveni teplotnich cidel DS18B20
#define ONE_WIRE_BUS_PIN 2 // onewire pro cidla DS18B20 na pinu (D)2
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress Probe01 = { 0x28, 0xFF, 0x1C, 0x56, 0x02, 0x17, 0x04, 0x68 }; //Bílá - T2
DeviceAddress Probe02 = { 0x28, 0xFF, 0x12, 0x82, 0x02, 0x17, 0x03, 0x46 }; //Zlatá - T3

// zde se bude ukládat zda přišel impuls z watchdog timeru
volatile int impuls_z_wdt = 1;
// zde se ukládají impulsy
volatile int citac_impulsu = 101; // hodnota 101 simuluje impuls po zapnutí, abychom nečekali
// zde nastavíme potřebný počet impulsů
// podle nastavení WDT viz níže je jeden impuls 8 sekund
volatile int impulsu_ke_spusteni = 101; // ...kazdych cca 15 minut

/* klíčové slovo volatile říká kompilatoru jak má zacházet z proměnou
   načte proměné z paměti RAM a ne z paměťového registru. Vzhledem k
   spánkovému režimu budou tyto hodnoty určitě poté přesné.
*/

// impuls z WATCHDOG TIMERU /////////////////
ISR(WDT_vect)
{
  //když je proměnná impuls_z_wdt na 0
  if (impuls_z_wdt == 0)
  {
    // zapiš do proměnné 1
    impuls_z_wdt = 1;
  }
}

void enterSleep(void)
{
  //nastavení nejúspornějšího módu
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // spánkový režim je povolený
  sleep_enable();
  // spuštění režimu spánku
  sleep_mode();

  // tady bude program pokračovat když se probudí

  // spánek zakázán
  sleep_disable();
  //znovu zapojení všech funkcí
  power_all_enable();
}

void ReadBME() {
  // Cteni vsech dostupnych informaci ze senzoru BMP
  // teplota
  myTeplota1 = (bme.readTemperature());
  myTeplota1 = myTeplota1 - 0.7; //0.7 je kompenzace na obě čidla DS18B20
  // relativni vlhkost
  myVlhkost1 = (bme.readHumidity());
  // tlak s prepoctem na hektoPascaly
  myTlak1 = (bme.readPressure() / 100.0F);
  myTlak1 = myTlak1 + 25; //25 hPA je kompenzace(kalibrace) stanovená porovnáním naměřených hodnot s oficiálním meteo spravodajstvím.
}

int Correct(int nr){
  //pripadne orezaní hodnoty na 0 az 255
  if (nr < 0) nr = 0;
  if (nr > 255) nr = 255;
  return nr;
}

long readVcc() {//zjištění stavu baterie (napájení)
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  uint8_t low = ADCL;
  uint8_t high = ADCH;
  long result = (high << 8) | low;
  result = 1125300L / result; // Výpočet Vcc (mV); 1125300 = 1.1*1023*1000
  return result;
}

void setup()
{
  // nastavení WATCHDOG TIMERU
  MCUSR &= ~(1 << WDRF); // neřešte
  WDTCSR |= (1 << WDCE) | (1 << WDE); // neřešte

  // nastavení času impulsu
  WDTCSR = 1 << WDP0 | 1 << WDP3; // 8 sekund, WDTCSR = B0110 --> 1 sekunda

  WDTCSR |= _BV(WDIE); //neřešte

  sensors.begin();
  Serial.println("Senzors interface inicializovan");

  // nastaveni presnosti senzoru DS18B20 na 11 bitu (muze byt 9 - 12)
  sensors.setResolution(Probe01, 11);
  sensors.setResolution(Probe02, 11);

  // zahajeni komunikace se senzorem BME280,
  // v pripade chyby je vypsana hlaska po seriove lince
  // a zastaven program
  if (!bme.begin(BME280_ADRESA)) {
    Serial.println("BME280 senzor nenalezen, zkontrolujte zapojeni!");
    while (1);
  } else {
    Serial.println("BME280 senzor inicializovan");
  }
}

void loop()
{
  //když je impuls z WATCHDOG TIMERU a zároveň i potřebný jejich počet
  if ((impuls_z_wdt == 1) & (impulsu_ke_spusteni == citac_impulsu))
  {
    ///////////////////////////////////////////////////////////////
    // zde je vlastní kód
    
    //inicializace sériové linky
    Serial.begin(9600);
    Serial.println("Seriova linka inicializovana");

    // zahájení komunikace se SigFox modemem po softwarové sériové lince rychlostí 9600 baud
    Sigfox.begin(9600);
    Serial.println("SigFox linka inicializovana");
  
    //zahajeni komunikace s cidly DS18B20
    Wire.begin();
    Serial.println("Wire interface inicializován");
    
    delay(1000);
    ReadBME(); //Cteni BME280
    sensors.requestTemperatures(); //Cteni DS18B20
    myTeplota2 = sensors.getTempC(Probe01);
    myTeplota3 = sensors.getTempC(Probe02);
    //Vypsání údajů po sériové lince
    Serial.print("Teploty °C: ");
    Serial.print(myTeplota1);
    Serial.print("°C, ");
    Serial.print(myTeplota2);
    Serial.print("°C, ");
    Serial.print(myTeplota3);
    Serial.println("°C");
    Serial.print("Vlhkost: ");
    Serial.print(myVlhkost1);
    Serial.println(" %");
    Serial.print("Tlak: ");
    Serial.print(myTlak1);
    Serial.println(" hPa");
    //Zjisteni stavu baterie (napeti napajeni)
    Vcc=readVcc(); //hodnota v mV
    Vcc=ceil(Vcc/100);//hodnota v desetinách V
    Serial.print("Napajeni: ");
    Serial.print(Vcc);
    Serial.println(" desetin V");
    //Sestavení zpravy pro SigFox
    /*
     * Callback URL
     * http://itrubec.cz/monitor/newdata.php?key=bflmpsvz&dev={device}&t1={customData#t1}&t2={customData#t2}&t3={customData#t3}&v1={customData#v1}&p2={customData#p1}&n1={customData#n1}&rssi={rssi}&seqn={seqNumber}&lat={lat}&lng={lng}
     * Format zpravy
     * t1::uint:8 t2::uint:8 t3::uint:8 v1::uint:8 p1::uint:8 n1::uint:8
    */
    //teploty zaokrouhlíme na celá čísla a přičteme 50 (50 => 0 stupnu celsia)
    t1 = int(round(myTeplota1)+50);
    t1 = Correct(t1);
    t2 = int(round(myTeplota2)+50);
    t2 = Correct(t2);
    t3 = int(round(myTeplota3)+50);
    t3 = Correct(t3);
    //vlhkost zaokrouhlíme na celé číslo
    v1 = int(round(myVlhkost1));
    v1 = Correct(v1);
    //tlak zaokrouhlíme na celé číslo a odečteme od něj 885 (1013-128) => 128 odpovídá 1013 hPa
    p1 = int(round(myTlak1)-885);
    p1 = Correct(p1);
    //napeti napajeni vynasobime 10 a zaokrouhlime na cele cislo
    n1 = int(round(Vcc));
    n1 = Correct(n1);
    //naformatovani zpravy k odeslani pres sigfox
    sprintf(zprava, "%02X%02X%02X%02X%02X%02X", t1, t2, t3, v1, p1, n1);
    Serial.print("Sigfox tvar zpravy: ");
    Serial.println(zprava);
    //odeslani zpravy do SigFox
    Sigfox.print("AT$SF=");
    Sigfox.println(zprava);
    
    delay(2000);
    
    Sigfox.print("AT$P=2"); //SigFox deep sleep mode
    Sigfox.end();
    Serial.end();
    
    // konec kódu, který se v nastaveném intervalu bude provádět
    //////////////////////////////////////////////////////////////

    citac_impulsu = 0;// vynuluj čítač
    impuls_z_wdt = 0; // vynuluj impuls
    enterSleep();// znovu do spánku
  }
  else
  {
    enterSleep();//znovu do spánku
  }
  citac_impulsu++; // inpuls se přičte i když nic neproběhlo
}
