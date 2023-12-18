#ifndef __SYS_PARAMETER_H__
#define __SYS_PARAMETER_H__


typedef struct _SYS_PARAMETER_
{
	char	bt_LocalDeviceName[40];
	char	ble_LocalDeviceName[40];
	uint8_t	bt_TxPowerLevel;
	uint8_t	bt_PagePowerLevel;
	uint8_t	BtTrimECO0;
	uint8_t	BtTrim;
	uint8_t	TwsVolSyncEnable;
	uint8_t	bt_CallinRingType;
	uint8_t	bt_BackgroundType;
	uint8_t	bt_SimplePairingEnable;
	char	bt_PinCode[6];
	uint8_t	bt_ReconnectionEnable;
	uint8_t	bt_ReconnectionTryCounts;
	uint8_t	bt_ReconnectionInternalTime;
	uint8_t	bt_BBLostReconnectionEnable;
	uint8_t	bt_BBLostTryCounts;
	uint8_t	bt_BBLostInternalTime;
	uint8_t	bt_TwsReconnectionEnable;
	uint8_t	bt_TwsReconnectionTryCounts;
	uint8_t	bt_TwsReconnectionInternalTime;
	uint8_t	bt_TwsBBLostReconnectionEnable;
	uint8_t	bt_TwsBBLostTryCounts;
	uint8_t	bt_TwsBBLostInternalTime;
	uint8_t	bt_TwsPairingWhenPhoneConnectedSupport;
	uint8_t	bt_TwsConnectedWhenActiveDisconSupport;
}SYS_PARAMETER;


#endif

