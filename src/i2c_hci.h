
#ifndef __I2CHCI_H_
#define __I2CHCI_H_

void I2CHCI_Init(const uint8_t ownAddr);
void I2CHCI_SetStatus(uint16_t status);
void I2CHCI_SetAdcValue(uint16_t status, uint8_t group);

#endif /* __I2CHCI_H_ */
