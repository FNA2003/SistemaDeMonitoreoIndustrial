/* 
 *  Código para buscar la dirección de una pantalla LCD I2C
 * basada en PCF8574. Se requiere esta librería:
 * 
 * - Wire.h by Arduino
 * 
 * Se debe conectar la pantalla en este esquema:
 * LCD  |  ARDUINO UNO
 * SDA  -> A4 (SDA)
 * SCL  -> A5 (SCL)
 */
#include <Wire.h>


void setup() {
    Serial.begin(9600);
    Wire.begin();
    Serial.println("Escaneando bus I2C...");
    escanearDireccionI2C();
}
void loop() {  }

/* Función para buscar la dirección de una pantalla */
void escanearDireccionI2C(void) {
    bool halloI2C = false;
  
    // La dirección de la pantalla I2C puede estar desde la dirección 1 a 119(dec)
    for (byte direccion = 0x01; direccion < 0x78; direccion++) {
        Wire.beginTransmission(direccion);
        if (Wire.endTransmission() == 0) {
            Serial.print("Dispositivo en 0x");
            Serial.print(direccion, HEX);
            Serial.println();

            halloI2C = true;
        }
    }

    if (!halloI2C)
        Serial.println("No se halló ningún dispositivo I2C!");
}
