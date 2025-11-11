/**
 * @file lcd_i2c.h
 * @brief header para controlar un display LCD basado mediante interfaz I2C..
 * Todas las funciones están diseñadas para facilitar el uso del LCD.
 */

#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>

/**
 * @brief Inicializa el LCD en modo 4 bits y lo deja listo para recibir comandos y datos.
 * Debe llamarse una vez al inicio del programa, después de configurar el I2C.
 */
void lcd_inicializar(void);

/**
 * @brief Posiciona el cursor en la fila y columna indicadas.
 * @param fila Fila (0 a 3, según el tipo de LCD)
 * @param columna Columna (0 a 19 para LCD de 20x4)
 */
void lcd_establecer_cursor(uint8_t fila, uint8_t columna);
/**
 * @brief Escribe una cadena de texto en el LCD desde la posición actual del cursor.
 * @param string Cadena de caracteres a mostrar en el LCD.
 */
void lcd_escribir(const char *string);
/**
 * @brief Limpia toda la pantalla del LCD y posiciona el cursor en la esquina superior izquierda.
 */
void lcd_borrarPantalla(void);

/**
 * @brief Borra una fila específica del LCD, rellenándola con espacios.
 * @param fila Fila a borrar (0 a 3)
 */
void lcd_borrarFila(uint8_t fila);

/**
 * @brief Borra el carácter en la posición actual del cursor (lo reemplaza por un espacio).
 */
void lcd_borrarCaracter(void);

/**
 * @brief Desplaza todo el contenido del LCD una posición a la izquierda.
 */
void lcd_desplazarIzquierda(void);

/**
 * @brief Desplaza todo el contenido del LCD una posición a la derecha.
 */
void lcd_desplazarDerecha(void);

/**
 * @brief Activa el parpadeo del cursor en la posición actual.
 */
void lcd_activar_parpadeo_cursor(void);

/**
 * @brief Desactiva el parpadeo del cursor.
 */
void lcd_desactivar_parpadeo_cursor(void);

/** 
 * @brief Crea un carácter personalizado en la CGRAM del LCD.
 * @param indice 0..7
 * @param patron Arreglo de 8 bytes con los 5 bits inferiores usados por fila.
 */
void lcd_crear_caracter(uint8_t indice, const uint8_t patron[8]);

/** 
 * @brief Escribe un único byte de datos (carácter) en la posición actual del cursor.
 * @param caracter Carácter a escribir
 */
void lcd_escribir_byte(uint8_t caracter);

#endif // LCD_I2C_H