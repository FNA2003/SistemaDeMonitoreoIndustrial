/*
 * Código para pantallas LCD I2C 20x4 con chip PCF8574.
 * Librerías necesarias (instalables desde Arduino IDE):
 * - Wire              (de Arduino, Instalada por defecto)
 * - SoftwareSerial    (de Arduino, Instalada por defecto)
 * - LiquidCrystal_I2C (de Marco Schwartz, https://github.com/johnrickman/LiquidCrystal_I2C/)
 * - PZEM004Tv30       (de Jakub Mandula, https://github.com/mandulaj/PZEM-004T-v30)

 * Conexiones:
 * LCD I2C:
 *   SDA -> A4
 *   SCL -> A5

 * Módulos PZEM004T:
 *   PZEM 1: RX -> D3, TX -> D2
 *   PZEM 2: RX -> D5, TX -> D4
 *   PZEM 3: RX -> D7, TX -> D6

 * Nota: Las conexiones de cada sensor PZEM puede ser alterada en la instancia 'pzemX'.
 * La pantalla no depende de estos módulos, sino de Wire.h, su pin no puede ser alterado en Arduino UNO.
 */
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <Wire.h>


// Pantalla I2C 20x4
// CARGAR la dirección de la pantalla en el primer parámetro
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Instancias para la lectura de cada PZEM (R, S y T)
PZEM004Tv30 pzems[] = {
  PZEM004Tv30 (2, 3, 0x00), // Pin de recepción y transmisión de arduino.
  PZEM004Tv30 (4, 5, 0x00),
  PZEM004Tv30 (6, 7, 0x00)
};

// Estructura usada para definir los parámetros a medir, su nombre y unidad
typedef struct {
  const char nombre[18];
  const char unidad[5];
  bool esAcumulativo; // Si es verdadero, se suma este parámetro en TODOS los sensores en lugar de mostrar el parametro de cada uno
  float (PZEM004Tv30::*metodo)(); // Método de la librería pzem para obtener el parámetro especificado
} ParametroElectrico;

// Lista de parámetros a medir en cada PZEM
const ParametroElectrico ParametrosElectricos[] = {
  { "VOLTAJE",            "V",   false, &PZEM004Tv30::voltage },
  { "CORRIENTE",          "A",   false, &PZEM004Tv30::current },
  { "FRECUENCIA",         "Hz",  false, &PZEM004Tv30::frequency },
  { "POTENCIA",           "W",   true,  &PZEM004Tv30::power   },  
  { "ENERGIA",            "kWh", true,  &PZEM004Tv30::energy },  
  { "FACTOR DE POTENCIA", " ",   false, &PZEM004Tv30::pf },
};

// Intervalo de cambio de parámetros (ms)
const unsigned long INTERVALO = 5000UL;
unsigned long ultimoCambio = 0UL;

uint8_t indiceParametros = 0; // Usado para ir variando los parámetros eléctricos a mostrar


void setup() {
  // Debemos iniciar la instancia de wire para poder ejecutar CrystalI2C
  Wire.begin();

  // Inicializamos la instancia de la pantalla con la dirección del header
  lcd.init();
  lcd.backlight();
}

void loop() {
	// Si ya pasó el intervalo, mostramos la siguiente medición
	if (millis() - ultimoCambio >= INTERVALO) {
		mostrarLectura(indiceParametros);
   
		// Ajusto el último intervalo hecho según lo que se tardó en escribir en el lcd
		ultimoCambio = millis();

    // Recorro la lista de parámetros como un "vector circular"
		size_t cantidadParametrosElectricos = sizeof(ParametrosElectricos) / sizeof(ParametroElectrico);
		indiceParametros = (indiceParametros + 1) % cantidadParametrosElectricos;
	}
}

/* Muestro los valores eléctricos pedidos, en cada sensor */
void mostrarLectura(uint8_t id) {
	float auxiliar = 0.0;

  lcd.clear(); // Borro lo que tuviese la pantalla anteriormente

	// Título, ¿Qué parámetro se muestra?
	lcd.setCursor(0, 0);
	lcd.print('['); lcd.print(ParametrosElectricos[id].nombre); lcd.print(']');

   // POTENCIA ó ENERGÍA, se muestra la suma
  if (ParametrosElectricos[id].esAcumulativo) {
    
    // Leo cada PZEM y voy sumando su lectura de potencia o energía
    for (auto& pzem : pzems)
      auxiliar += (pzem.*ParametrosElectricos[id].metodo)();

    // Etiqueta, "Se está mostrando la suma, no las individuales"
    lcd.setCursor(0, 1); lcd.print("R + S + T");
    // En la última línea, muestro el resultado de la suma con 1 decimal de precisión y su unidad
    lcd.setCursor(0, 3); lcd.print(auxiliar, 1); lcd.print(" "); lcd.print(ParametrosElectricos[id].unidad);
    return;
  }

  const uint8_t cantidadSensores = sizeof(pzems) / sizeof(pzems[0]);
  
  // Si no se quiere leer potencia o energía, mostramos cada parámetro individual
  for (uint8_t i = 0; i < cantidadSensores; i++) { // NOTA; Acá recorremos por índice para usar el número de 'i'

    // Medida de uno del parámetro de uno de los PZEM
    auxiliar = (pzems[i].*ParametrosElectricos[id].metodo)();

    // Desde la fila 1 hasta la 3, vamos imprimiendo
    lcd.setCursor(0, i + 1);
    // El nombre de la línea (NOTA; Usamos ASCII para obtener las letras siguientes R->S->T)
    lcd.print((char)('R' + i)); lcd.print(": ");
    // Mostramos el valor con 1 decimal de precisión y su unidad
    lcd.print(auxiliar, 1); lcd.print(" "); lcd.print(ParametrosElectricos[id].unidad);
  }
}
