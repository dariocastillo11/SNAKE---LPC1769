/***********************************************************************//**
 * @file        lpc17xx_i2c.h
 * @brief        Contains all macro definitions and function prototypes
 *                 support for I2C firmware library on LPC17xx
 * @version        2.0
 * @date        21. May. 2010
 * @author        NXP MCU SW Application Team
 **************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 **************************************************************************/

/* Peripheral group ----------------------------------------------------------- */
/** @defgroup I2C I2C
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_I2C_H_
#define LPC17XX_I2C_H_

/* Includes ------------------------------------------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"


#ifdef __cplusplus
extern "C"
{
#endif

/* === Traducción (español) del encabezado del archivo ===
 * @file        lpc17xx_i2c.h
 * @brief       Contiene todas las definiciones de macros y prototipos de
 *              funciones para la librería de firmware I2C en LPC17xx
 * @version     2.0
 * @date        21. May. 2010
 * @autor       NXP MCU SW Application Team (documentación traducida)
 *
 * Notas: el texto de licencia y la responsabilidad se mantienen en inglés
 * en el bloque original. Estas líneas son únicamente una traducción
 * orientativa para facilitar la comprensión en español.
 */



/* Private Macros ------------------------------------------------------------- */
/** @defgroup I2C_Private_Macros I2C Private Macros
 * @{
 */

/* --------------------- BIT DEFINITIONS -------------------------------------- */
/*******************************************************************//**
 * I2C Control Set register description
 *********************************************************************/
#define I2C_I2CONSET_AA                ((0x04)) /*!< Assert acknowledge flag */
#define I2C_I2CONSET_SI                ((0x08)) /*!< I2C interrupt flag */
#define I2C_I2CONSET_STO            ((0x10)) /*!< STOP flag */
#define I2C_I2CONSET_STA            ((0x20)) /*!< START flag */
#define I2C_I2CONSET_I2EN            ((0x40)) /*!< I2C interface enable */
/*<! (ES) Indica la bandera de reconocimiento (Acknowledge) */
#define I2C_I2CONSET_SI                ((0x08)) /*!< I2C interrupt flag */
/*<! (ES) Bandera de interrupción I2C */
#define I2C_I2CONSET_STO            ((0x10)) /*!< STOP flag */
/*<! (ES) Bandera de STOP */
#define I2C_I2CONSET_STA            ((0x20)) /*!< START flag */
/*<! (ES) Bandera de START */
#define I2C_I2CONSET_I2EN            ((0x40)) /*!< I2C interface enable */
/*<! (ES) Habilita la interfaz I2C */

/*******************************************************************//**
 * I2C Control Clear register description
 *********************************************************************/
/** Assert acknowledge Clear bit */
#define I2C_I2CONCLR_AAC            ((1<<2))
/** I2C interrupt Clear bit */
#define I2C_I2CONCLR_SIC            ((1<<3))
/** START flag Clear bit */
#define I2C_I2CONCLR_STAC            ((1<<5))
/** I2C interface Disable bit */
#define I2C_I2CONCLR_I2ENC            ((1<<6))
/* (ES) Bit para limpiar la señal de reconocimiento (Acknowledge) */
/** I2C interrupt Clear bit */
#define I2C_I2CONCLR_SIC            ((1<<3))
/* (ES) Bit para limpiar la bandera de interrupción I2C */
/** START flag Clear bit */
#define I2C_I2CONCLR_STAC            ((1<<5))
/* (ES) Bit para limpiar la bandera START */
/** I2C interface Disable bit */
#define I2C_I2CONCLR_I2ENC            ((1<<6))
/* (ES) Bit para deshabilitar la interfaz I2C */

/********************************************************************//**
 * I2C Status Code definition (I2C Status register)
 *********************************************************************/
/* Return Code in I2C status register */
#define I2C_STAT_CODE_BITMASK        ((0xF8))

/* I2C return status code definitions ----------------------------- */

/** No relevant information */
#define I2C_I2STAT_NO_INF                        ((0xF8))
/* (ES) Sin información relevante */

/* Master transmit mode -------------------------------------------- */
/** A start condition has been transmitted */
#define I2C_I2STAT_M_TX_START                    ((0x08))
/** A repeat start condition has been transmitted */
#define I2C_I2STAT_M_TX_RESTART                    ((0x10))
/** SLA+W has been transmitted, ACK has been received */
#define I2C_I2STAT_M_TX_SLAW_ACK                ((0x18))
/** SLA+W has been transmitted, NACK has been received */
#define I2C_I2STAT_M_TX_SLAW_NACK                ((0x20))
/** Data has been transmitted, ACK has been received */
#define I2C_I2STAT_M_TX_DAT_ACK                    ((0x28))
/** Data has been transmitted, NACK has been received */
#define I2C_I2STAT_M_TX_DAT_NACK                ((0x30))
/** Arbitration lost in SLA+R/W or Data bytes */
#define I2C_I2STAT_M_TX_ARB_LOST                ((0x38))
/* (ES) Se ha transmitido una condición START */
/** A repeat start condition has been transmitted */
#define I2C_I2STAT_M_TX_RESTART                    ((0x10))
/* (ES) Se ha transmitido una condición RESTART */
/** SLA+W has been transmitted, ACK has been received */
#define I2C_I2STAT_M_TX_SLAW_ACK                ((0x18))
/* (ES) SLA+W transmitido, se recibió ACK */
/** SLA+W has been transmitted, NACK has been received */
#define I2C_I2STAT_M_TX_SLAW_NACK                ((0x20))
/* (ES) SLA+W transmitido, se recibió NACK */
/** Data has been transmitted, ACK has been received */
#define I2C_I2STAT_M_TX_DAT_ACK                    ((0x28))
/* (ES) Datos transmitidos, se recibió ACK */
/** Data has been transmitted, NACK has been received */
#define I2C_I2STAT_M_TX_DAT_NACK                ((0x30))
/* (ES) Datos transmitidos, se recibió NACK */
/** Arbitration lost in SLA+R/W or Data bytes */
#define I2C_I2STAT_M_TX_ARB_LOST                ((0x38))
/* (ES) Pérdida de arbitraje durante SLA+R/W o transferencia de datos */

/* Master receive mode -------------------------------------------- */
/** A start condition has been transmitted */
#define I2C_I2STAT_M_RX_START                    ((0x08))
/** A repeat start condition has been transmitted */
#define I2C_I2STAT_M_RX_RESTART                    ((0x10))
/** Arbitration lost */
#define I2C_I2STAT_M_RX_ARB_LOST                ((0x38))
/** SLA+R has been transmitted, ACK has been received */
#define I2C_I2STAT_M_RX_SLAR_ACK                ((0x40))
/** SLA+R has been transmitted, NACK has been received */
#define I2C_I2STAT_M_RX_SLAR_NACK                ((0x48))
/** Data has been received, ACK has been returned */
#define I2C_I2STAT_M_RX_DAT_ACK                    ((0x50))
/** Data has been received, NACK has been return */
#define I2C_I2STAT_M_RX_DAT_NACK                ((0x58))
/* (ES) Se ha transmitido una condición START (modo recepción) */
/** A repeat start condition has been transmitted */
#define I2C_I2STAT_M_RX_RESTART                    ((0x10))
/* (ES) Se ha transmitido una condición RESTART (modo recepción) */
/** Arbitration lost */
#define I2C_I2STAT_M_RX_ARB_LOST                ((0x38))
/* (ES) Pérdida de arbitraje */
/** SLA+R has been transmitted, ACK has been received */
#define I2C_I2STAT_M_RX_SLAR_ACK                ((0x40))
/* (ES) SLA+R transmitido, se recibió ACK */
/** SLA+R has been transmitted, NACK has been received */
#define I2C_I2STAT_M_RX_SLAR_NACK                ((0x48))
/* (ES) SLA+R transmitido, se recibió NACK */
/** Data has been received, ACK has been returned */
#define I2C_I2STAT_M_RX_DAT_ACK                    ((0x50))
/* (ES) Datos recibidos, se devolvió ACK */
/** Data has been received, NACK has been return */
#define I2C_I2STAT_M_RX_DAT_NACK                ((0x58))
/* (ES) Datos recibidos, se devolvió NACK */

/* Slave receive mode -------------------------------------------- */
/** Own slave address has been received, ACK has been returned */
#define I2C_I2STAT_S_RX_SLAW_ACK                ((0x60))
/* (ES) Se recibió la propia dirección esclava, se devolvió ACK */

/** Arbitration lost in SLA+R/W as master */
#define I2C_I2STAT_S_RX_ARB_LOST_M_SLA            ((0x68))
/** Own SLA+W has been received, ACK returned */
//#define I2C_I2STAT_S_RX_SLAW_ACK                ((0x68))

/** General call address has been received, ACK has been returned */
#define I2C_I2STAT_S_RX_GENCALL_ACK                ((0x70))
/* (ES) Se recibió la dirección de llamada general, se devolvió ACK */

/** Arbitration lost in SLA+R/W (GENERAL CALL) as master */
#define I2C_I2STAT_S_RX_ARB_LOST_M_GENCALL        ((0x78))
/** General call address has been received, ACK has been returned */
//#define I2C_I2STAT_S_RX_GENCALL_ACK                ((0x78))

/** Previously addressed with own SLV address;
 * Data has been received, ACK has been return */
#define I2C_I2STAT_S_RX_PRE_SLA_DAT_ACK            ((0x80))
/** Previously addressed with own SLA;
 * Data has been received and NOT ACK has been return */
#define I2C_I2STAT_S_RX_PRE_SLA_DAT_NACK        ((0x88))
/** Previously addressed with General Call;
 * Data has been received and ACK has been return */
#define I2C_I2STAT_S_RX_PRE_GENCALL_DAT_ACK        ((0x90))
/** Previously addressed with General Call;
 * Data has been received and NOT ACK has been return */
#define I2C_I2STAT_S_RX_PRE_GENCALL_DAT_NACK    ((0x98))
/* (ES) Previamente dirigida con la dirección esclava propia; datos recibidos, se devolvió ACK */
/** Previously addressed with own SLA;
 * Data has been received and NOT ACK has been return */
#define I2C_I2STAT_S_RX_PRE_SLA_DAT_NACK        ((0x88))
/* (ES) Previamente dirigida con la SLA propia; datos recibidos y NO se devolvió ACK */
/** Previously addressed with General Call;
 * Data has been received and ACK has been return */
#define I2C_I2STAT_S_RX_PRE_GENCALL_DAT_ACK        ((0x90))
/* (ES) Previamente dirigida por Llamada General; datos recibidos, se devolvió ACK */
/** Previously addressed with General Call;
 * Data has been received and NOT ACK has been return */
#define I2C_I2STAT_S_RX_PRE_GENCALL_DAT_NACK    ((0x98))
/* (ES) Previamente dirigida por Llamada General; datos recibidos y NO se devolvió ACK */
/** A STOP condition or repeated START condition has
 * been received while still addressed as SLV/REC
 * (Slave Receive) or SLV/TRX (Slave Transmit) */
#define I2C_I2STAT_S_RX_STA_STO_SLVREC_SLVTRX    ((0xA0))
/* (ES) Se recibió una condición STOP o START repetido mientras aún estaba dirigido como SLV/REC o SLV/TRX */

/** Slave transmit mode */
/** Own SLA+R has been received, ACK has been returned */
#define I2C_I2STAT_S_TX_SLAR_ACK                ((0xA8))
/* (ES) Se recibió SLA+R propio, se devolvió ACK */

/** Arbitration lost in SLA+R/W as master */
#define I2C_I2STAT_S_TX_ARB_LOST_M_SLA            ((0xB0))
/** Own SLA+R has been received, ACK has been returned */
//#define I2C_I2STAT_S_TX_SLAR_ACK                ((0xB0))

/** Data has been transmitted, ACK has been received */
#define I2C_I2STAT_S_TX_DAT_ACK                    ((0xB8))
/** Data has been transmitted, NACK has been received */
#define I2C_I2STAT_S_TX_DAT_NACK                ((0xC0))
/* (ES) Datos transmitidos, se recibió ACK */
/** Data has been transmitted, NACK has been received */
#define I2C_I2STAT_S_TX_DAT_NACK                ((0xC0))
/* (ES) Datos transmitidos, se recibió NACK */
/** Last data byte in I2DAT has been transmitted (AA = 0);
 ACK has been received */
#define I2C_I2STAT_S_TX_LAST_DAT_ACK            ((0xC8))

/** Time out in case of using I2C slave mode */
#define I2C_SLAVE_TIME_OUT                        0x10000UL
/* (ES) Tiempo de espera (timeout) al usar el modo esclavo I2C */

/********************************************************************//**
 * I2C Data register definition
 *********************************************************************/
/** Mask for I2DAT register*/
#define I2C_I2DAT_BITMASK            ((0xFF))
/* (ES) Máscara para el registro I2DAT */

/** Idle data value will be send out in slave mode in case of the actual
 * expecting data requested from the master is greater than its sending data
 * length that can be supported */
#define I2C_I2DAT_IDLE_CHAR            (0xFF)
/* (ES) Valor de datos en reposo que se envía en modo esclavo cuando el dato esperado por el maestro
        es mayor que la longitud de datos que el esclavo puede enviar */

/********************************************************************//**
 * I2C Monitor mode control register description
 *********************************************************************/
#define I2C_I2MMCTRL_MM_ENA            ((1<<0))        /**< Monitor mode enable */
#define I2C_I2MMCTRL_ENA_SCL        ((1<<1))        /**< SCL output enable */
#define I2C_I2MMCTRL_MATCH_ALL        ((1<<2))        /**< Select interrupt register match */
#define I2C_I2MMCTRL_BITMASK        ((0x07))        /**< Mask for I2MMCTRL register */
/* (ES) Habilita el modo monitor */
#define I2C_I2MMCTRL_ENA_SCL        ((1<<1))        /**< SCL output enable */
/* (ES) Habilita la salida SCL */
#define I2C_I2MMCTRL_MATCH_ALL        ((1<<2))        /**< Select interrupt register match */
/* (ES) Selecciona la coincidencia del registro de interrupción */
#define I2C_I2MMCTRL_BITMASK        ((0x07))        /**< Mask for I2MMCTRL register */
/* (ES) Máscara para el registro I2MMCTRL */

/********************************************************************//**
 * I2C Data buffer register description
 *********************************************************************/
/** I2C Data buffer register bit mask */
#define I2DATA_BUFFER_BITMASK        ((0xFF))
/* (ES) Máscara del registro de buffer de datos I2C */

/********************************************************************//**
 * I2C Slave Address registers definition
 *********************************************************************/
/** General Call enable bit */
#define I2C_I2ADR_GC                ((1<<0))
/** I2C Slave Address registers bit mask */
#define I2C_I2ADR_BITMASK            ((0xFF))
/* (ES) Bit para habilitar llamada general */
/** I2C Slave Address registers bit mask */
#define I2C_I2ADR_BITMASK            ((0xFF))
/* (ES) Máscara para registros de dirección esclava I2C */

/********************************************************************//**
 * I2C Mask Register definition
 *********************************************************************/
/** I2C Mask Register mask field */
#define I2C_I2MASK_MASK(n)            ((n&0xFE))
/* (ES) Campo de máscara del registro I2C I2MASK */

/********************************************************************//**
 * I2C SCL HIGH duty cycle Register definition
 *********************************************************************/
/** I2C SCL HIGH duty cycle Register bit mask */
#define I2C_I2SCLH_BITMASK            ((0xFFFF))
/* (ES) Máscara para el registro del ciclo de trabajo alto de SCL */

/********************************************************************//**
 * I2C SCL LOW duty cycle Register definition
 *********************************************************************/
/** I2C SCL LOW duty cycle Register bit mask */
#define I2C_I2SCLL_BITMASK            ((0xFFFF))

/* I2C status values */
#define I2C_SETUP_STATUS_ARBF   (1<<8)    /**< Arbitration false */
#define I2C_SETUP_STATUS_NOACKF (1<<9)    /**< No ACK returned */
#define I2C_SETUP_STATUS_DONE   (1<<10)    /**< Status DONE */
/* (ES) Máscara para el registro del ciclo de trabajo bajo de SCL */

/* I2C status values */
#define I2C_SETUP_STATUS_ARBF   (1<<8)    /**< Arbitration false */
/* (ES) Falla de arbitraje */
#define I2C_SETUP_STATUS_NOACKF (1<<9)    /**< No ACK returned */
/* (ES) No se devolvió ACK */
#define I2C_SETUP_STATUS_DONE   (1<<10)    /**< Status DONE */
/* (ES) Operación completada (DONE) */

/*********************************************************************//**
 * I2C monitor control configuration defines
 **********************************************************************/
#define I2C_MONITOR_CFG_SCL_OUTPUT    I2C_I2MMCTRL_ENA_SCL        /**< SCL output enable */
#define I2C_MONITOR_CFG_MATCHALL    I2C_I2MMCTRL_MATCH_ALL        /**< Select interrupt register match */

/* ---------------- CHECK PARAMETER DEFINITIONS ---------------------------- */
/* Macros check I2C slave address */
#define PARAM_I2C_SLAVEADDR_CH(n)    ((n)<=3)

/** Macro to determine if it is valid SSP port number */
/* (ES) Habilitar salida SCL en modo monitor */
#define I2C_MONITOR_CFG_MATCHALL    I2C_I2MMCTRL_MATCH_ALL        /**< Select interrupt register match */
/* (ES) Seleccionar coincidencia de registro para interrupción en modo monitor */

/* ---------------- CHECK PARAMETER DEFINITIONS ---------------------------- */
/* Macros check I2C slave address */
/* (ES) Macros para comprobar la dirección esclava I2C */
#define PARAM_I2C_SLAVEADDR_CH(n)    ((n)<=3)

/** Macro to determine if it is valid SSP port number */
/* (ES) Macro para determinar si es un puerto I2C válido */
#define PARAM_I2Cx(n)    ((((uint32_t *)n)==((uint32_t *)LPC_I2C0)) \
|| (((uint32_t *)n)==((uint32_t *)LPC_I2C1)) \
|| (((uint32_t *)n)==((uint32_t *)LPC_I2C2)))

/* Macros check I2C monitor configuration type */
/* (ES) Macros para comprobar el tipo de configuración del monitor I2C */
#define PARAM_I2C_MONITOR_CFG(n) ((n==I2C_MONITOR_CFG_SCL_OUTPUT) || (I2C_MONITOR_CFG_MATCHALL))

/**
 * @}
 */



/* Public Types --------------------------------------------------------------- */
/** @defgroup I2C_Public_Types I2C Public Types
 * @{
 */

/**
 * @brief I2C Own slave address setting structure
 */
typedef struct {
    uint8_t SlaveAddrChannel;    /**< Slave Address channel in I2C control,
                                should be in range from 0..3
                                */
    uint8_t SlaveAddr_7bit;        /**< Value of 7-bit slave address */
    uint8_t GeneralCallState;    /**< Enable/Disable General Call Functionality
                                when I2C control being in Slave mode, should be:
                                - ENABLE: Enable General Call function.
                                - DISABLE: Disable General Call function.
                                */
    uint8_t SlaveAddrMaskValue;    /**< Any bit in this 8-bit value (bit 7:1)
                                which is set to '1' will cause an automatic compare on
                                the corresponding bit of the received address when it
                                is compared to the SlaveAddr_7bit value associated with this
                                mask register. In other words, bits in SlaveAddr_7bit value
                                which are masked are not taken into account in determining
                                an address match
 *  Esta estructura contiene la configuración de la dirección esclava propia
 */
typedef struct {
    uint8_t SlaveAddrChannel;    /**< Canal de dirección esclava en el control I2C,
                                debe estar en el rango 0..3
                                */
    uint8_t SlaveAddr_7bit;       /**< Valor de la dirección esclava de 7 bits */

        uint8_t GeneralCallState;    /**< Habilitar/Deshabilitar la funcionalidad de llamada general
                                                                cuando el control I2C está en modo esclavo, debe ser:
                                                                - ENABLE: Habilitar la función de llamada general.
                                                                - DISABLE: Deshabilitar la función de llamada general.
                                                                */
    uint8_t SlaveAddrMaskValue;   /**< Cualquier bit en este valor de 8 bits (bits 7:1)
                                que se ponga a '1' hará que se ignore la comparación
                                automática del bit correspondiente de la dirección recibida
                                al compararla con `SlaveAddr_7bit` asociado a este registro
                                de máscara. En otras palabras, los bits en `SlaveAddr_7bit`
                                que están enmascarados no se tienen en cuenta para determinar
                                una coincidencia de dirección.
                                */
} I2C_OWNSLAVEADDR_CFG_Type;


/**
 * @brief Master transfer setup data structure definitions
 */
typedef struct
{
  uint32_t          sl_addr7bit;                /**< Slave address in 7bit mode */
  uint8_t*          tx_data;                    /**< Pointer to Transmit data - NULL if data transmit
                                                      is not used */
  uint32_t          tx_length;                    /**< Transmit data length - 0 if data transmit
                                                      is not used*/
  uint32_t          tx_count;                    /**< Current Transmit data counter */
  uint8_t*          rx_data;                    /**< Pointer to Receive data - NULL if data receive
                                                      is not used */
  uint32_t          rx_length;                    /**< Receive data length - 0 if data receive is
                                                       not used */
  uint32_t          rx_count;                    /**< Current Receive data counter */
  uint32_t          retransmissions_max;        /**< Max Re-Transmission value */
  uint32_t          retransmissions_count;        /**< Current Re-Transmission counter */
  uint32_t          status;                        /**< Current status of I2C activity */
  void                 (*callback)(void);            /**< Pointer to Call back function when transmission complete
                                                    used in interrupt transfer mode */
} I2C_M_SETUP_Type;

/**
 * @brief (ES) Definiciones de la estructura de datos para la configuración
 *              de transferencias en modo maestro.
 *
 * Campos (ES):
 * - sl_addr7bit: dirección esclava en modo 7 bits.
 * - tx_data: puntero a datos a transmitir (NULL si no se usa transmisión).
 * - tx_length: longitud de datos a transmitir (0 si no se usa transmisión).
 * - tx_count: contador de bytes transmitidos actualmente.
 * - rx_data: puntero a datos a recibir (NULL si no se usa recepción).
 * - rx_length: longitud de datos a recibir (0 si no se usa recepción).
 * - rx_count: contador de bytes recibidos actualmente.
 * - retransmissions_max: número máximo de reintentos.
 * - retransmissions_count: contador de reintentos actuales.
 * - status: estado actual de la actividad I2C.
 * - callback: puntero a función de retorno cuando la transmisión termina
 *             (usado en modo interrupción).
 */
typedef struct
{
  uint32_t          sl_addr7bit;            /* (ES) Dirección del esclavo en modo de 7 bits */
  uint8_t*          tx_data;                /* (ES) Puntero a los datos a transmitir - NULL si no se usa transmisión */
  uint32_t          tx_length;              /* (ES) Longitud de los datos a transmitir - 0 si no se usa transmisión */
  uint32_t          tx_count;               /* (ES) Contador actual de bytes transmitidos */
  uint8_t*          rx_data;                /* (ES) Puntero a los datos a recibir - NULL si no se usa recepción */
  uint32_t          rx_length;              /* (ES) Longitud de los datos a recibir - 0 si no se usa */
  uint32_t          rx_count;               /* (ES) Contador actual de bytes recibidos */
  uint32_t          retransmissions_max;    /* (ES) Número máximo de reintentos (retransmisiones) */
  uint32_t          retransmissions_count;  /* (ES) Contador actual de reintentos */
  uint32_t          status;                 /* (ES) Estado actual de la actividad I2C */
  void             (*callback)(void);       /* (ES) Puntero a función de retorno llamada al completar la transmisión (modo interrupción) */
} I2C_M_SETUP_Type;

/* (ES) Estructura de configuración para transferencias en modo maestro.
 * Véase los campos descritos arriba.
 */


/**
 * @brief Slave transfer setup data structure definitions
 */
/**
 * @brief (ES) Definiciones de la estructura de datos para la configuración
 *              de transferencias en modo esclavo.
 *
 * Campos (ES):
 * - tx_data: puntero a datos a transmitir cuando el maestro lee del esclavo.
 * - tx_length: longitud de los datos a transmitir.
 * - tx_count: contador de bytes transmitidos actualmente.
 * - rx_data: puntero a datos para recibir cuando el maestro escribe al esclavo.
 * - rx_length: longitud de los datos a recibir.
 * - rx_count: contador de bytes recibidos actualmente.
 * - status: estado actual de la transferencia.
 * - callback: puntero a función de retorno (opcional).
 */
typedef struct
{
  uint8_t*          tx_data;
  uint32_t          tx_length;
  uint32_t          tx_count;
  uint8_t*          rx_data;
  uint32_t          rx_length;
  uint32_t          rx_count;
  uint32_t          status;
  void                 (*callback)(void);
} I2C_S_SETUP_Type;

/**
 * @brief Transfer option type definitions
 */
typedef enum {
    I2C_TRANSFER_POLLING = 0,        /**< Transfer in polling mode */
    I2C_TRANSFER_INTERRUPT            /**< Transfer in interrupt mode */
/**
 * @brief (ES) Definición de opciones de transferencia.
 *
 * Valores (ES):
 * - I2C_TRANSFER_POLLING: transferencia en modo sondeo (polling).
 * - I2C_TRANSFER_INTERRUPT: transferencia en modo por interrupciones.
 */
typedef enum {
        I2C_TRANSFER_POLLING = 0,        /**< Transfer in polling mode */
        /* (ES) Transferencia en modo sondeo (polling) */
        I2C_TRANSFER_INTERRUPT            /**< Transfer in interrupt mode */
        /* (ES) Transferencia en modo por interrupciones */
} I2C_TRANSFER_OPT_Type;


/**
 * @}
 */


/* Public Functions ----------------------------------------------------------- */
/** @defgroup I2C_Public_Functions I2C Public Functions
 * @{
 */

/* I2C Init/DeInit functions ---------- */
void I2C_Init(LPC_I2C_TypeDef *I2Cx, uint32_t clockrate);
void I2C_DeInit(LPC_I2C_TypeDef* I2Cx);
//void I2C_SetClock (LPC_I2C_TypeDef *I2Cx, uint32_t target_clock);
void I2C_Cmd(LPC_I2C_TypeDef* I2Cx, FunctionalState NewState);

/* I2C transfer data functions -------- */
Status I2C_MasterTransferData(LPC_I2C_TypeDef *I2Cx, \
        I2C_M_SETUP_Type *TransferCfg, I2C_TRANSFER_OPT_Type Opt);
Status I2C_SlaveTransferData(LPC_I2C_TypeDef *I2Cx, \
        I2C_S_SETUP_Type *TransferCfg, I2C_TRANSFER_OPT_Type Opt);
uint32_t I2C_MasterTransferComplete(LPC_I2C_TypeDef *I2Cx);
uint32_t I2C_SlaveTransferComplete(LPC_I2C_TypeDef *I2Cx);


void I2C_SetOwnSlaveAddr(LPC_I2C_TypeDef *I2Cx, I2C_OWNSLAVEADDR_CFG_Type *OwnSlaveAddrConfigStruct);
uint8_t I2C_GetLastStatusCode(LPC_I2C_TypeDef* I2Cx);

/* I2C Monitor functions ---------------*/
void I2C_MonitorModeConfig(LPC_I2C_TypeDef *I2Cx, uint32_t MonitorCfgType, FunctionalState NewState);
void I2C_MonitorModeCmd(LPC_I2C_TypeDef *I2Cx, FunctionalState NewState);
uint8_t I2C_MonitorGetDatabuffer(LPC_I2C_TypeDef *I2Cx);
BOOL_8 I2C_MonitorHandler(LPC_I2C_TypeDef *I2Cx, uint8_t *buffer, uint32_t size);

/* I2C Interrupt handler functions ------*/
void I2C_IntCmd (LPC_I2C_TypeDef *I2Cx, Bool NewState);
void I2C_MasterHandler (LPC_I2C_TypeDef *I2Cx);
/**
 * @brief (ES) Inicializa el periférico I2C y configura la velocidad de reloj.
 *
 * @param I2Cx   Puntero al periférico I2C (por ejemplo, LPC_I2C0).
 * @param clockrate  Frecuencia de SCL deseada en Hz.
 *
 * @note Esta función configura los registros básicos del controlador I2C.
 *       Asegúrate de haber configurado los pines y el reloj del sistema
 *       antes de llamar. No bloquea: prepara el periférico para su uso.
 */
void I2C_Init(LPC_I2C_TypeDef *I2Cx, uint32_t clockrate);
/**
 * @brief (ES) De-inicializa el periférico I2C, dejando el módulo en estado reset
 *        o deshabilitado.
 *
 * @param I2Cx  Puntero al periférico I2C a desinicializar.
 * el estado
 *       de pines o de alimentación puede requerir acciones adicionales.
 */
void I2C_DeInit(LPC_I2C_TypeDef* I2Cx);
//void I2C_SetClock (LPC_I2C_TypeDef *I2Cx, uint32_t target_clock);
/**
 * @brief (ES) Habilita o deshabilita la interfaz I2C.
 *
 * @param I2Cx      Puntero al periférico I2C.
 * @param NewState  ENABLE para habilitar, DISABLE para deshabilitar.
 *
 * @note Útil para pausar la comunicación o liberar el bus.
 */
void I2C_Cmd(LPC_I2C_TypeDef* I2Cx, FunctionalState NewState);

/* I2C transfer data functions -------- */
/**
 * @brief (ES) Inicia una transferencia I2C en modo maestro.
 *
 * @param I2Cx         Puntero al periférico I2C.
 * @param TransferCfg  Puntero a la estructura I2C_M_SETUP_Type que describe
 *                     la transferencia (dirección, buffers, longitudes, callback).
 * @param Opt          Opción de transferencia: I2C_TRANSFER_POLLING o
 *                     I2C_TRANSFER_INTERRUPT.
 *
 * @return Status      Indica éxito o error (usar macros I2C_SETUP_STATUS_* para
 *                     comprobar el resultado en modo polling).
 *
 * @note En modo polling la función realiza la operación de forma bloqueante.
 *       En modo interrupt retorna inmediatamente y la finalización se notifica
 *       mediante el campo callback de TransferCfg (si se proporciona).
 */
Status I2C_MasterTransferData(LPC_I2C_TypeDef *I2Cx, \
        I2C_M_SETUP_Type *TransferCfg, I2C_TRANSFER_OPT_Type Opt);
/**
 * @brief (ES) Configura y atiende una transferencia I2C en modo esclavo.
 *
 * @param I2Cx         Puntero al periférico I2C.
 * @param TransferCfg  Puntero a la estructura I2C_S_SETUP_Type que contiene
 *                     buffers y estado para la transferencia esclava.
 * @param Opt          Opción de transferencia: I2C_TRANSFER_POLLING o
 *                     I2C_TRANSFER_INTERRUPT.
 *
 * @return Status      Estado de la operación (ver notas del driver para códigos).
 *
 * @note En modo interrupt el driver utilizará el callback (si existe) para
 *       notificar la finalización; en modo polling la función puede bloquear
 *       hasta completarse o hasta detectarse un error.
 */
Status I2C_SlaveTransferData(LPC_I2C_TypeDef *I2Cx, \
        I2C_S_SETUP_Type *TransferCfg, I2C_TRANSFER_OPT_Type Opt);
/**
 * @brief (ES) Devuelve el estado de finalización/errores de la transferencia
 *        maestro iniciada con I2C_MasterTransferData.
 *
 * @param I2Cx  Puntero al periférico I2C.
 * @return uint32_t Flags de estado (I2C_SETUP_STATUS_*).
 */
uint32_t I2C_MasterTransferComplete(LPC_I2C_TypeDef *I2Cx);
/**
 * @brief (ES) Devuelve el estado de finalización/errores de la transferencia
 *        esclava iniciada con I2C_SlaveTransferData.
 *
 * @param I2Cx  Puntero al periférico I2C.
 * @return uint32_t Flags de estado (I2C_SETUP_STATUS_*).
 */
uint32_t I2C_SlaveTransferComplete(LPC_I2C_TypeDef *I2Cx);


/**
 * @brief (ES) Configura las direcciones esclavas propias (canales 0..3).
 *
 * @param I2Cx  Puntero al periférico I2C.
 * @param OwnSlaveAddrConfigStruct  Puntero a la estructura que contiene
 *                                   canal, dirección 7-bit, máscara y estado
 *                                   de General Call.
 */
void I2C_SetOwnSlaveAddr(LPC_I2C_TypeDef *I2Cx, I2C_OWNSLAVEADDR_CFG_Type *OwnSlaveAddrConfigStruct);
/**
 * @brief (ES) Obtiene el último código de estado del registro de estado I2C.
 *
 * @param I2Cx  Puntero al periférico I2C.
 * @return uint8_t Código de estado (valor del registro de estado I2C).
 */
uint8_t I2C_GetLastStatusCode(LPC_I2C_TypeDef* I2Cx);

/* I2C Monitor functions ---------------*/
/**
 * @brief (ES) Configura opciones del modo monitor (por ejemplo salida SCL,
 *        coincidencia total de registros).
 *
 * @param I2Cx         Puntero al periférico I2C.
 * @param MonitorCfgType  Tipo de configuración (I2C_MONITOR_CFG_SCL_OUTPUT o I2C_MONITOR_CFG_MATCHALL).
 * @param NewState     ENABLE para activar la configuración, DISABLE para desactivarla.
 */
void I2C_MonitorModeConfig(LPC_I2C_TypeDef *I2Cx, uint32_t MonitorCfgType, FunctionalState NewState);
/**
 * @brief (ES) Habilita o deshabilita el modo monitor del periférico I2C.
 *
 * @param I2Cx      Puntero al periférico I2C.
 * @param NewState  ENABLE para activar el modo monitor, DISABLE para desactivarlo.
 *
 * @note El modo monitor permite observar el bus I2C sin participar en la comunicación.
 */
void I2C_MonitorModeCmd(LPC_I2C_TypeDef *I2Cx, FunctionalState NewState);
uint8_t I2C_MonitorGetDatabuffer(LPC_I2C_TypeDef *I2Cx);
/**
 * @brief (ES) Lee datos en modo monitor y los copia en un buffer proporcionado.
 *
 * @param I2Cx   Puntero al periférico I2C.
 * @param buffer Puntero al buffer destino donde se copiarán los bytes detectados.
 * @param size   Tamaño máximo (bytes) del buffer.
 * @return BOOL_8 TRUE si se copiaron datos, FALSE si no hay datos disponibles.
 *
 * @note Este handler se utiliza para leer registros capturados en modo monitor
 *       y típicamente se llama desde el código de la aplicación (no desde ISR).
 */
BOOL_8 I2C_MonitorHandler(LPC_I2C_TypeDef *I2Cx, uint8_t *buffer, uint32_t size);

/* I2C Interrupt handler functions ------*/
/**
 * @brief (ES) Habilita o deshabilita las interrupciones I2C globales.
 *
 * @param I2Cx     Puntero al periférico I2C.
 * @param NewState ENABLE para activar interrupciones, DISABLE para desactivarlas.
 */
void I2C_IntCmd (LPC_I2C_TypeDef *I2Cx, Bool NewState);
/**
 * @brief (ES) Manejador de eventos del lado maestro (debe llamarse desde la ISR
 *        del controlador I2C). Implementa la máquina de estados del maestro.
 *
 * @param I2Cx  Puntero al periférico I2C.
 *
 * @note No invocar directamente desde contexto de tarea; está pensado para
 *       ser llamado por la rutina de interrupción que atiende el I2C.
 */
void I2C_MasterHandler (LPC_I2C_TypeDef *I2Cx);
/**
 * @brief (ES) Manejador de eventos del lado esclavo (debe llamarse desde la ISR
 *        del controlador I2C). Atiende condiciones de dirección, lectura y escritura del maestro.
 *
 * @param I2Cx  Puntero al periférico I2C.
 *
 * @note Mantener corto y seguro para ISR; preferible señalar a una tarea para
 *       procesamiento pesado.
 */
void I2C_SlaveHandler (LPC_I2C_TypeDef *I2Cx);


/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LPC17XX_I2C_H_ */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
