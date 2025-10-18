/* 
 *  Este código se funcionará en LCD's I2C de 20x4
 * basados en PCF8574. Las librerías que se requieren son:
 * 
 * Para instalar una, desde Arduino IDE:
 *  Programa > Incluir Librería > Administrar Bibliotecas y busca las faltantes
 * 
 * - LiquidCrystal_I2C by Marco Schwartz
 * - SoftwareSerial by Arduino
 * - PZEM004Tv30 by Jakub Mandula (https://github.com/mandulaj/PZEM-004T-v30)
 * - Wire by Arduino
 * 
 * Se debe conectar la pantalla en este esquema:
 * LCD  |  ARDUINO UNO
 * SDA  -> A4 (SDA)
 * SCL  -> A5 (SCL)
 *
 * Y, para cada uno de los módulos PZEM004T:
 * PZEM 1 |  ARDUINO UNO
 * RX 	  -> Digital Pin 2
 * TX 	  -> Digital Pin 3
 *
 * PZEM 2 |  ARDUINO UNO
 * RX 	  -> Digital Pin 4
 * TX 	  -> Digital Pin 5
 *
 * PZEM 3 |  ARDUINO UNO
 * RX 	  -> Digital Pin 6
 * TX 	  -> Digital Pin 7
 * 
 * NOTA: La conexión de estos últimos puede alterar en la instancia de las clases
 * pzemNSerial correspondiente. Pero no de la pantalla, pues para esto, deberá modificar
 * la lectura de wire.h, ya no depende de este módulo.
 */
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <Wire.h>


// Pantalla I2C 20x4
// CARGAR la dirección de la pantalla en el primer parámetro
LiquidCrystal_I2C lcd(0X20, 20, 4);

// Intervalo de cambio de parámetros (ms)
const unsigned long INTERVALO = 5000UL;

unsigned long ultimoCambio = 0; // Duración de la iteración actual

// Pines UART por SoftwareSerial
SoftwareSerial pzem1Serial(2, 3);
SoftwareSerial pzem2Serial(4, 5);
SoftwareSerial pzem3Serial(6, 7);
// Instancias para la lectura de cada PZEM
PZEM004Tv30 pzem1(pzem1Serial), pzem2(pzem2Serial), pzem3(pzem3Serial);


// Parámetros a mostrar
const char* medidas[] = {
	"CORRIENTE", "POTENCIA", "ENERGIA", "VOLTAJE", "FRECUENCIA", "FACTOR DE POTENCIA"
};
const char* unidades[] = { 
	"A", "W", "kWh", "V", "Hz", " " 
};
// Punteros a cada método para cada parámetro eléctrico
typedef float (PZEM004Tv30::*PZEMMethod)();
PZEMMethod metodos[] = {
	&PZEM004Tv30::current,
	&PZEM004Tv30::power,
	&PZEM004Tv30::energy,
	&PZEM004Tv30::voltage,
	&PZEM004Tv30::frequency,
	&PZEM004Tv30::pf
};
uint8_t indice = 0; // Índice del parámetro a mostrar


void setup() {
  // Debemos iniciar la instancia de wire para poder ejecutar CrystalI2C
  Wire.begin();

  // Inicializamos la instancia de la pantalla con la dirección del header
  lcd.init();
  lcd.backlight();
}

void loop() {
	unsigned long tiempoActual = millis();

	// Si ya pasó el intervalo, mostramos la siguiente medición
	if (tiempoActual - ultimoCambio >= INTERVALO) {
		mostrarLectura(indice);
   
		// Ajusto el último intervalo hecho según lo que se tardó en escribir en el lcd
		ultimoCambio = millis();

		// Siguiente parámetro y retorno al rango 0–5
		indice = (indice + 1) % 6;
	}
}

/* Muestro los valores eléctricos pedidos, en cada sensor */
void mostrarLectura(uint8_t id) {
	float valor = 0.0;

  lcd.clear(); // Borro lo que tuviese la pantalla

	// Título
	lcd.setCursor(0, 0);
	lcd.print('[');
	lcd.print(medidas[id]);
	lcd.print(']');

  // Si lo que se quiere medir es potencia o energía, se medirá únicamente su suma
  if (id == 1 || id == 2) {
    pzem1Serial.listen();
    valor += (pzem1.*metodos[id])();
    pzem2Serial.listen();
    valor += (pzem2.*metodos[id])();
    pzem3Serial.listen();
    valor += (pzem3.*metodos[id])();

    lcd.setCursor(0, 1);
    lcd.print("R + S + T");

    lcd.setCursor(0, 3);
    lcd.print(valor, 1);
    lcd.print(unidades[id]);

    return;
  }

	// Sensor PZEM 1
	pzem1Serial.listen();
	valor = (pzem1.*metodos[id])();
	lcd.setCursor(0, 1);
	lcd.print("R: ");
	lcd.print(valor, 1); // Máxima resolución 1 decimal
	lcd.print(unidades[id]);

	// Sensor PZEM 2
	pzem2Serial.listen();
	valor = (pzem2.*metodos[id])();
	lcd.setCursor(0, 2);
	lcd.print("S: ");
	lcd.print(valor, 1);
	lcd.print(unidades[id]);

	// Sensor PZEM 3
	pzem3Serial.listen();
	valor = (pzem3.*metodos[id])();
	lcd.setCursor(0, 3);
	lcd.print("T: ");
	lcd.print(valor, 1);
  lcd.print(unidades[id]);
}
