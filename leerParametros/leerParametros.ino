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
 *   PZEM 1: RX -> D5, TX -> D4
 *   PZEM 2: RX -> D7, TX -> D6
 *   PZEM 3: RX -> D9, TX -> D8

 * Sensores de Temperatura:
 *   SENSOR 1: SALIDA ANALÓGICA -> A0
 *   SENSOR 2: SALIDA ANALÓGICA -> A1

 * Sensor de Efecto de Campo (Hall):
 *   SENSOR: SALIDA DIGITAL -> D2

 * Nota: Las conexiones de cada sensor puede ser alterada.
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
// RX, TX de Arduino
PZEM004Tv30 pzems[] = {
  PZEM004Tv30 (4, 5, 0x00),
  PZEM004Tv30 (6, 7, 0x00),
  PZEM004Tv30 (8, 9, 0x00)
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

uint8_t CANTIDAD_PARAMETROS_ELECTRICOS;
uint8_t CANTIDAD_SENSORES_ELECTRICOS;

/* ------------------ Sensores: temperatura y Hall ------------------ */
// Pines de temperatura (analógicos)
const int SEN_TEMP_1 = A0;
const int SEN_TEMP_2 = A1;
const float V_REF = 5.0;
const int PASOS_ADC = 1024; // 2^10, con 10 los bits de presición del ADC de Arduino UNO

// Sensor Hall en pin D2
const uint8_t SEN_HALL_PIN = 2;
volatile unsigned long contPulsos = 0; // Cuenta las veces que se recibió una interrupción por el sensor Hall


void contadorPulsos() { contPulsos++; }
void tarjetaExtra();

void setup() {
  // Debemos iniciar la instancia de wire para poder ejecutar CrystalI2C
  Wire.begin();

  // Inicializamos la instancia de la pantalla con la dirección del header
  lcd.init();
  lcd.backlight();
  
  // Cuento la cantidad parámetros y sensores dados
  CANTIDAD_PARAMETROS_ELECTRICOS = sizeof(ParametrosElectricos) / sizeof(ParametroElectrico);
  CANTIDAD_SENSORES_ELECTRICOS = sizeof(pzems) / sizeof(PZEM004Tv30);

  // Configuro pin del Hall (pin 2) y attachInterrupt en FALLING
  // Usamos INPUT_PULLUP para tener estado alto estable y detectar la bajada del sensor
  pinMode(SEN_HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SEN_HALL_PIN), contadorPulsos, FALLING);
}

void loop() {
	// Si ya pasó el intervalo, mostramos la siguiente medición
	if (millis() - ultimoCambio >= INTERVALO) {
		mostrarLectura(indiceParametros);
   
		// Ajusto el último intervalo hecho según lo que se tardó en escribir en el lcd
		ultimoCambio = millis();

    // Recorro la lista de parámetros como un "vector circular"
		indiceParametros = (indiceParametros + 1) % CANTIDAD_PARAMETROS_ELECTRICOS;

    // Si se terminó de mostrar los parámetros, mostramos una última tarjeta, la extra
    if (!indiceParametros) tarjetaExtra();
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


  // Si no se quiere leer potencia o energía, mostramos cada parámetro individual
  for (uint8_t i = 0; i < CANTIDAD_SENSORES_ELECTRICOS; i++) { // NOTA; Acá recorremos por índice para usar el número de 'i'

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

void tarjetaExtra() {
  // A partir de este momento, vale lo que cuente el sensor Hall para hacer el cálculo
  noInterrupts(); contPulsos = 0; interrupts();  
  unsigned long tiempoActual;
  
  // Espera activa a que termine el tiempo de la tarjeta anterior (Permitimos que se suspenda por si SoftwareSerial tiene que cambiar de instancia y necesita sincronizarse)
  while((tiempoActual = millis()) - ultimoCambio < INTERVALO) { delay(10); }

  // Inicio de Sección crítica, no se medirá más al sensor Hall
  noInterrupts();
  // RPM = frecuencia x 60seg, en nuestro caso, usamos milisegundos para verificar si se excedio el tiempo del intervalo
  // NOTA; Asumimos hay un solo imán en el eje, en caso de haber más, variamos la frecuencia.
  unsigned long rpms = (contPulsos * 60000.0) / (tiempoActual - ultimoCambio);
  // Salida inmediata de sección crítica
  interrupts();
  
  // Leemos los valores "crudos" de cada sensor
  int rawTemp1 = analogRead(SEN_TEMP_1);
  int rawTemp2 = analogRead(SEN_TEMP_2);

  // Los sensores de temperatura que usamos tienen una resolución de 0°C a 100°C con un factor lineal de 10mV/°C (Y, 0°C = 0V)
  float temperatura1 = ((V_REF / PASOS_ADC) * rawTemp1) / 0.01; // 10mV/°C = 0.01V/°C
  float temperatura2 = ((V_REF / PASOS_ADC) * rawTemp2) / 0.01;

  // A partir de acá, solamente mostramos por el LCD los datos tomados
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("[VELOCIDAD ANGULAR]"); lcd.setCursor(0, 1); lcd.print(rpms);
  lcd.setCursor(0, 2); lcd.print("[Tem.Rod.1] "); lcd.print(temperatura1, 1); lcd.print(" C");
  lcd.setCursor(0, 3); lcd.print("[Tem.Rod.2] "); lcd.print(temperatura2, 1); lcd.print(" C");

  ultimoCambio = millis(); // Actualizamos el "nuevo" último cambio y nos vamos ;)
}
