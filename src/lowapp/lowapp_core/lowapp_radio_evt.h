/**
 * @file lowapp_radio_evt.h
 * @brief LoWAPP radio event
 *
 * Defines the functions that are to be called from the radio layer.
 *
 * @author Nathan Olff
 * @date August 22, 2016
 */

#ifndef LOWAPP_CORE_RADIO_EVT_H_
#define LOWAPP_CORE_RADIO_EVT_H_

/** Bit flag for TxDone */
#define RADIOFLAGS_TXDONE	0x01
/** Bit flag for TxError */
#define RADIOFLAGS_TXTIMEOUT	0x02
/** Bit flag for RxDone */
#define RADIOFLAGS_RXDONE	0x04
/** Bit flag for RxError or RxTimeout */
#define RADIOFLAGS_RXERROR	0x08

void cadDone ( bool channelActivityDetected );
void rxDone ( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void rxError ( void );
void rxTimeout ( void );
void txDone ( void );
void txTimeout ( void );

void noSmRxDone ( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void noSmRxError(void);
void noSmRxTimeout(void);
void noSmTxDone(void);
void noSmTxTimeout(void);

#endif /* LOWAPP_CORE_RADIO_EVT_H_ */
