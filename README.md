# 📚 Funciones LCD_I2C para LPC1769

Estas funciones permiten controlar un display LCD estándar mediante interfaz I2C una LPC1769.

---

## 🔓 **Funciones públicas**

Estas funciones están disponibles para el usuario y se declaran en `lcd_i2c.h`:

| Función                      | Descripción                                                                 |
|------------------------------|-----------------------------------------------------------------------------|
| `lcd_init()`                 | Inicializa el LCD en modo 4 bits y lo deja listo para recibir comandos y datos. |
| `lcd_setCursor(fila, col)`   | Posiciona el cursor en la fila y columna indicadas.                         |
| `lcd_escribir(string)`       | Escribe una cadena de texto en el LCD desde la posición actual del cursor.  |
| `lcd_borrarPantalla()`       | Limpia toda la pantalla del LCD y posiciona el cursor en la esquina superior izquierda. |
| `lcd_borrarFila(fila)`       | Borra una fila específica del LCD, rellenándola con espacios.               |
| `lcd_borrarCaracter()`       | Borra el carácter en la posición actual del cursor (lo reemplaza por un espacio). |
| `lcd_desplazarIzquierda()`   | Desplaza todo el contenido del LCD una posición a la izquierda.             |
| `lcd_desplazarDerecha()`     | Desplaza todo el contenido del LCD una posición a la derecha.               |
| `lcd_parpadearCursor()`      | Activa el parpadeo del cursor en la posición actual.                        |
| `lcd_parpadearCursorOff()`   | Desactiva el parpadeo del cursor.                                           |

---

## 🔒 **Funciones privadas (internas, static)**

Estas funciones solo se usan dentro de `lcd_i2c.c` para la implementación interna y no están disponibles fuera del módulo:

| Función                         | Descripción                                                                 |
|----------------------------------|-----------------------------------------------------------------------------|
| `i2c_enviarByte(dato)`          | Envía un byte por I2C al LCD. Bloquea hasta que la transferencia termina.   |
| `lcd_pulso(dato)`               | Genera el pulso de habilitación necesario para que el LCD registre el dato. |
| `lcd_enviarByte(dato, modo)`    | Envía un byte completo al LCD (modo comando o datos, nibble alto y bajo).   |
| `lcd_enviarNibble(dato)`        | Envía solo 4 bits al LCD (usado en la inicialización).                      |

---

## 📝 **Funcionamiento general**

- Abstraer el manejo de bajo nivel del LCD por I2C, permitiendo al usuario inicializar el display, escribir texto, limpiar la pantalla, mover el cursor y controlar el parpadeo.
- Las funciones privadas gestionan la comunicación I2C y la secuencia de pulsos y nibbles necesaria para que el LCD registre correctamente los comandos y datos.
- El usuario solo necesita llamar a las funciones públicas desde su programa principal para interactuar con el LCD.

---

> **Ejemplo de uso:**
> ```c
> lcd_init();
> lcd_setCursor(0, 0);
> lcd_escribir("Hola mundo!");
> lcd_parpadearCursor();
> ```

---

## 🛠️ Drivers I2C para LPC17xx

`lpc17xx_i2c` provee funciones y estructuras para configurar y utilizar el periférico I2C en la LPC17xx. Permite operar en modo maestro y esclavo, enviar y recibir datos, y manejar interrupciones.

---

### 🔓 **Funciones públicas principales**

| Función                                      | Descripción                                                                                   |
|-----------------------------------------------|-----------------------------------------------------------------------------------------------|
| `I2C_Init(I2Cx, clockrate)`                  | Inicializa el periférico I2C seleccionado (`I2Cx`) con la frecuencia deseada (`clockrate`).   |
| `I2C_DeInit(I2Cx)`                           | Desinicializa el periférico I2C, dejándolo en estado de reset.                                |
| `I2C_Cmd(I2Cx, NewState)`                    | Habilita o deshabilita el periférico I2C (`ENABLE`/`DISABLE`).                               |
| `I2C_MasterTransferData(I2Cx, *cfg, opt)`    | Realiza una transferencia de datos en modo maestro. Permite enviar y/o recibir datos.         |
| `I2C_SlaveTransferData(I2Cx, *cfg, opt)`     | Realiza una transferencia de datos en modo esclavo.                                           |
| `I2C_MasterTransferComplete(I2Cx)`           | Indica si la transferencia en modo maestro ha finalizado.                                     |
| `I2C_SlaveTransferComplete(I2Cx)`            | Indica si la transferencia en modo esclavo ha finalizado.                                     |
| `I2C_IntCmd(I2Cx, NewState)`                 | Habilita o deshabilita las interrupciones del periférico I2C.                                 |
| `I2C_MasterHandler(I2Cx)`                    | Rutina de atención de interrupción para transferencias en modo maestro.                       |
| `I2C_SlaveHandler(I2Cx)`                     | Rutina de atención de interrupción para transferencias en modo esclavo.                       |
| `I2C_SetOwnSlaveAddr(I2Cx, *cfg)`            | Configura la dirección de esclavo propia del periférico I2C.                                  |
| `I2C_MonitorModeConfig(I2Cx, cfg, state)`    | Configura el modo monitor del I2C (para escuchar el bus).                                     |
| `I2C_MonitorModeCmd(I2Cx, state)`            | Habilita o deshabilita el modo monitor.                                                       |
| `I2C_MonitorGetDatabuffer(I2Cx)`             | Obtiene datos del buffer en modo monitor.                                                     |
| `I2C_MonitorHandler(I2Cx, *buffer, size)`    | Rutina para manejar datos recibidos en modo monitor.                                          |

---

### 📝 **Descripción de uso**

- **Inicialización:**  
  Usa `I2C_Init()` para configurar el periférico con la frecuencia deseada.  
  Ejemplo:  
  ```c
  I2C_Init(LPC_I2C1, 100000); // Inicializa I2C1 a 100kHz
  I2C_Cmd(LPC_I2C1, ENABLE);  // Habilita I2C1
  ```

---

## 🧩 Estructuras principales de los drivers I2C

Estas estructuras se utilizan para configurar y controlar las transferencias I2C en modo maestro y esclavo, y para definir la dirección propia del periférico cuando opera como esclavo.

---

### `I2C_M_SETUP_Type`  
**Estructura para transferencias en modo maestro.**

| Campo                   | Descripción                                                                                   |
|-------------------------|-----------------------------------------------------------------------------------------------|
| `sl_addr7bit`           | Dirección del esclavo en 7 bits.                                                              |
| `tx_data`               | Puntero al buffer de datos a transmitir.                                                      |
| `tx_length`             | Cantidad de bytes a transmitir.                                                               |
| `tx_count`              | Contador interno de bytes transmitidos.                                                       |
| `rx_data`               | Puntero al buffer de datos a recibir.                                                         |
| `rx_length`             | Cantidad de bytes a recibir.                                                                  |
| `rx_count`              | Contador interno de bytes recibidos.                                                          |
| `retransmissions_max`   | Número máximo de reintentos en caso de error.                                                 |
| `retransmissions_count` | Contador interno de reintentos realizados.                                                    |
| `status`                | Estado actual de la transferencia (códigos de error, éxito, etc).                             |
| `callback`              | Puntero a función callback que se ejecuta al finalizar la transferencia (modo interrupción).  |

---

### `I2C_S_SETUP_Type`  
**Estructura para transferencias en modo esclavo.**

| Campo       | Descripción                                                                                   |
|-------------|-----------------------------------------------------------------------------------------------|
| `tx_data`   | Puntero al buffer de datos a transmitir al maestro.                                           |
| `tx_length` | Cantidad de bytes a transmitir.                                                               |
| `tx_count`  | Contador interno de bytes transmitidos.                                                       |
| `rx_data`   | Puntero al buffer de datos a recibir del maestro.                                             |
| `rx_length` | Cantidad de bytes a recibir.                                                                  |
| `rx_count`  | Contador interno de bytes recibidos.                                                          |
| `status`    | Estado actual de la transferencia (códigos de error, éxito, etc).                             |
| `callback`  | Puntero a función callback que se ejecuta al finalizar la transferencia (modo interrupción).  |

---

### `I2C_OWNSLAVEADDR_CFG_Type`  
**Estructura para configurar la dirección propia del periférico I2C en modo esclavo.**

| Campo               | Descripción                                                                                   |
|---------------------|-----------------------------------------------------------------------------------------------|
| `SlaveAddrChannel`  | Canal de dirección de esclavo (0 a 3).                                                        |
| `SlaveAddr_7bit`    | Valor de la dirección de esclavo en 7 bits.                                                   |
| `GeneralCallState`  | Habilita o deshabilita la función "General Call" (aceptar mensajes a todos los esclavos).     |
| `SlaveAddrMaskValue`| Máscara de bits para comparar la dirección recibida con la propia (permite ignorar ciertos bits). |

---# SNAKE---LPC1769
