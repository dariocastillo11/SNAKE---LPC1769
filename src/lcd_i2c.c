/**
 * @file lcd_i2c.c
 * @brief Implementación de funciones para controlar un LCD por I2C.
 *
 * Este archivo contiene las funciones para inicializar y manipular el LCD usando
 * un modulo I2C y una LPC1769.
 */

#include "lcd_i2c.h"
#include "LPC17xx.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"

// Dirección I2C del LCD y definición de bits de control
#define LCD_DIRECCION        0x27
#define LONG_COLUMNA_LCD         20      //  LCD de 20 columnas
#define LONG_FILA_LCD       4       //  LCD de 4 filas
#define LCD_LUZ_FONDO       0x08    // Bit para luz de fondo
#define LCD_ENABLE      0x04    // Bit para pulso de habilitación
#define MODO_COMANDO    0x00
#define MODO_DATOS      0x01    // Bit para seleccionar registro de datos
// Buffer de un byte para transferencias I2C
static uint8_t i2c_buffer[1];

/**
 * @brief Envía un byte por I2C al LCD. Bloquea hasta que la transferencia termina.
 * @param dato Byte a enviar
 */
static void i2c_enviarByte(uint8_t dato) {
    I2C_M_SETUP_Type cfgMaestro;
    cfgMaestro.sl_addr7bit = LCD_DIRECCION;
    cfgMaestro.tx_data = i2c_buffer;
    cfgMaestro.tx_length = 1;
    cfgMaestro.rx_data = NULL;
    cfgMaestro.rx_length = 0;
    cfgMaestro.retransmissions_max = 3;
    i2c_buffer[0] = dato;
    I2C_MasterTransferData(LPC_I2C0, &cfgMaestro, I2C_TRANSFER_POLLING);
    cfgMaestro.sl_addr7bit = LCD_DIRECCION;//DIRECCION DEL ESCLAVO
    cfgMaestro.tx_data = i2c_buffer;//PUNTERO DE LOS DATOS A TRANSMITIR
    cfgMaestro.tx_length = 1;//LONGITUD DE LOS DATOS A TRANSMITIR 
    cfgMaestro.rx_data = NULL;//punteros de los datos a recibir. como no reibo es null
    cfgMaestro.rx_length = 0;//longitud de datos recibidos. es 0 porque no se usa
    cfgMaestro.retransmissions_max = 3;//nro maximo de reintentos retransmisiones
    i2c_buffer[0] = dato;//cargo en el buffer el dato na enviar
    I2C_MasterTransferData(LPC_I2C1, &cfgMaestro, I2C_TRANSFER_POLLING);
}

/**
 * @brief Genera el pulso de habilitación necesario para que el LCD registre el dato.
 * @param dato Byte a enviar con el pulso
 */
static void lcd_pulso(uint8_t dato) {
    i2c_enviarByte(dato | LCD_ENABLE | LCD_LUZ_FONDO);      // Enable alto
    i2c_enviarByte((dato & ~LCD_ENABLE) | LCD_LUZ_FONDO);   // Enable bajo
}

/**
 * @brief Envía un byte completo al LCD (modo comando o datos).
 * @param dato Byte a enviar
 * @param modo 0 = MODO_COMANDO, 1 = MODO_DATOS
 */
static void lcd_enviarByte(uint8_t dato, uint8_t modo) {
    uint8_t alto = (dato & 0xF0) | modo | LCD_LUZ_FONDO;         // Nibble alto
    //0xf0
    uint8_t bajo  = ((dato << 4) & 0xF0) | modo | LCD_LUZ_FONDO;  // Nibble bajo
    i2c_enviarByte(alto);
    lcd_pulso(alto);
    i2c_enviarByte(bajo);
    lcd_pulso(bajo);
}

/**
 * @brief Envía solo 4 bits al LCD (usado en la inicialización).
 * @param dato Nibble alto a enviar
 */
static void lcd_enviarNibble(uint8_t dato) {
    i2c_enviarByte(dato | LCD_LUZ_FONDO);
    lcd_pulso(dato);
}

/**
 * @brief Inicializa el LCD en modo 4 bits, limpia pantalla y configura parámetros básicos.
 */
void lcd_inicializar(void) {
    lcd_enviarNibble(0x30);
    lcd_enviarNibble(0x30);
    lcd_enviarNibble(0x30);
    lcd_enviarNibble(0x20);
    lcd_enviarByte(0x28, MODO_COMANDO);
    lcd_enviarByte(0x08, MODO_COMANDO);
    lcd_enviarByte(0x01, MODO_COMANDO);
    lcd_enviarByte(0x06, MODO_COMANDO);
    lcd_enviarByte(0x0C, MODO_COMANDO);
}

/**
 * @brief Posiciona el cursor en la fila y columna indicadas.
 * @param fila Fila (0 a 3)
 * @param columna Columna (0 a 19)
 */
void lcd_establecer_cursor(uint8_t fila, uint8_t columna) {
    uint8_t posicion[] = {0x00, 0x40, 0x14, 0x54};
    lcd_enviarByte(0x80 | (posicion[fila] + columna), MODO_COMANDO);
}

/**
 * @brief Escribe una cadena de texto en el LCD desde la posición actual del cursor.
 * @param string Cadena a mostrar
 */
void lcd_escribir(const char *string) {
    while (*string){
        lcd_enviarByte(*string++, MODO_DATOS);
    }
}

/**
 * @brief Limpia toda la pantalla del LCD y posiciona el cursor en (0,0).
 */
void lcd_borrarPantalla(void) {
    lcd_enviarByte(0x01, MODO_COMANDO);
    lcd_establecer_cursor(0, 0);
}

/**
 * @brief Borra una fila específica del LCD (rellena con espacios).
 * @param fila Fila a borrar (0 a 3)
 */
void lcd_borrarFila(uint8_t fila) {
    lcd_establecer_cursor(fila, 0);
    for (uint8_t i = 0; i < 20; i++) lcd_enviarByte(' ', MODO_DATOS);
    lcd_establecer_cursor(fila, 0);
    lcd_setCursor(fila, 0);
    for (uint8_t i = 0; i < LONG_COLUMNA_LCD; i++){
        lcd_enviarByte(' ', MODO_DATOS);
    }
    lcd_setCursor(fila, 0);
}

/**
 * @brief Borra el carácter en la posición actual del cursor (lo reemplaza por espacio).
 */
void lcd_borrarCaracter(void) {
    lcd_enviarByte(' ', MODO_DATOS);
}

/**
 * @brief Desplaza el texto del LCD a la izquierda.
 */
void lcd_desplazarIzquierda(void) {
    lcd_enviarByte(0x18, MODO_COMANDO);
    //0X18 COMANDO PARA DESPLAZAR A LA IZQUIERDA
}

/**
 * @brief Desplaza el texto del LCD a la derecha.
 */
void lcd_desplazarDerecha(void) {
    lcd_enviarByte(0x1C, MODO_COMANDO);
}

/**
 * @brief Activa el parpadeo del cursor en la posición actual.
 */
void lcd_activar_parpadeo_cursor(void) {
    lcd_enviarByte(0x0F, MODO_COMANDO);
}

/**
 * @brief Desactiva el parpadeo del cursor.
 */
void lcd_desactivar_parpadeo_cursor(void) {
    lcd_enviarByte(0x0C, MODO_COMANDO);
}

/**
 * @brief Crea un carácter personalizado en CGRAM (índice 0..7)
 */
void lcd_crear_caracter(uint8_t indice, const uint8_t patron[8]) {
    if (indice > 7) return;
    /* Establecer dirección CGRAM: 0x40 + indice*8 */
    lcd_enviarByte(0x40 | (indice << 3), MODO_COMANDO);
    for (int i = 0; i < 8; i++) {
        lcd_enviarByte(patron[i] & 0x1F, MODO_DATOS);
    }
}

/**
 * @brief Escribe un byte de datos (carácter) en la posición actual.
 */
void lcd_escribir_byte(uint8_t ch) {
    lcd_enviarByte(ch, MODO_DATOS);
}
