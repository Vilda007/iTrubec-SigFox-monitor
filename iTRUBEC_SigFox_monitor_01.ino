float myTeplota1, myTeplota2, myTeplota3, myTlak1, myVlhkost1;

// potřebné knihovny
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <Adafruit_BME280.h>
#include "Wire.h"
#include <DallasTemperature.h>

// nastaveni adresy senzoru
#define BME280_ADRESA (0x76)
// inicializace senzoru BME z knihovny
Adafruit_BME280 bme;

//Nastaveni teplotnich cidel DS18B20
#define ONE_WIRE_BUS_PIN 2 // onewire pro cidla DS18B20 na pinu (D)2
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress Probe01 = { 0x28, 0xFF, 0x1C, 0x56, 0x02, 0x17, 0x04, 0x68 }; //Bílá
DeviceAddress Probe02 = { 0x28, 0xFF, 0x12, 0x82, 0x02, 0x17, 0x03, 0x46 }; //Zlatá

// zde se bude ukládat zda přišel impuls z watchdog timeru
// hodnota 1 simuluje impuls po zapnutí, aby jsme nečekali
volatile int impuls_z_wdt = 1;
// zde se ukládají impulsy
volatile int citac_impulsu = 2;
// zde nastavíme potřebný počet impulsů
// podle nastavení WDT viz níže je jeden impuls 8 sekund
volatile int impulsu_ke_spusteni = 2;

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
  // relativni vlhkost
  myVlhkost1 = (bme.readHumidity());
  // tlak s prepoctem na hektoPascaly
  myTlak1 = (bme.readPressure() / 100.0F);
}

void setup()
{
  // nastavení WATCHDOG TIMERU
  MCUSR &= ~(1 << WDRF); // neřešte
  WDTCSR |= (1 << WDCE) | (1 << WDE); // neřešte

  // nastavení času impulsu
  WDTCSR = 1 << WDP0 | 1 << WDP3; // 8 sekund, WDTCSR = B0110 --> 1 sekunda

  WDTCSR |= _BV(WDIE); //neřešte

  //inicializace sériové linky
  Serial.begin(9600);
  Serial.println("Seriova linka inicializovana");
 
  //zahajeni komunikace s cidly DS18B20
  Wire.begin();
  Serial.println("Wire interface inicializován");
  sensors.begin();
  Serial.println("Senzors interface inicializovan");

  // nastaveni presnosti senzoru DS18B20 na 11 bitu (muze byt 9 - 12)
  sensors.setResolution(Probe01, 11);
  sensors.setResolution(Probe02, 11);
  Serial.println("0");

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
    delay(3000);
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
    delay(3000);
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