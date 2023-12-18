#include <string.h>
#include "type.h"
#include "sys_param.h"
#include "spi_flash.h"
#include "flash_table.h"

SYS_PARAMETER sys_parameter;

static const SYS_PARAMETER default_parameter = 
{
	.bt_LocalDeviceName 	= BT_NAME,
	.ble_LocalDeviceName	= BT_NAME,
	.bt_TxPowerLevel		= BT_TX_POWER_LEVEL,
	.bt_PagePowerLevel		= BT_PAGE_TX_POWER_LEVEL,
	.BtTrimECO0				= BT_TRIM_ECO0,
	.BtTrim					= BT_TRIM,
	.TwsVolSyncEnable		= TRUE,	//主从之间音量控制同步
	.bt_CallinRingType		= USE_LOCAL_AND_PHONE_RING,
	.bt_BackgroundType		= BT_BACKGROUND_FAST_POWER_ON_OFF,
	.bt_SimplePairingEnable	= TRUE,
	.bt_PinCode				= "0000",
	.bt_ReconnectionEnable			= TRUE,
	.bt_ReconnectionTryCounts		= 5,
	.bt_ReconnectionInternalTime	= 3,
	.bt_BBLostReconnectionEnable	= TRUE,
	.bt_BBLostTryCounts				= 90,
	.bt_BBLostInternalTime			= 5,
	.bt_TwsReconnectionEnable		= TRUE,
	.bt_TwsReconnectionTryCounts	= 3,
	.bt_TwsReconnectionInternalTime	= 3,
	.bt_TwsBBLostReconnectionEnable	= TRUE,
	.bt_TwsBBLostTryCounts			= 3,
	.bt_TwsBBLostInternalTime		= 5,
	.bt_TwsConnectedWhenActiveDisconSupport = FALSE,
	.bt_TwsPairingWhenPhoneConnectedSupport	= TRUE,
};

void sys_parameter_init(void)
{
	SPI_FLASH_ERR_CODE ret = 0;
	uint32_t addr = get_sys_parameter_addr();	
	
	memset(&sys_parameter,0,sizeof(sys_parameter));

	if(addr > 0)
	{
		ret = SpiFlashRead(addr, (uint8_t*)&sys_parameter, sizeof(SYS_PARAMETER), 0);
	}

	if(ret != FLASH_NONE_ERR || !flash_table_is_valid())
	{
		memcpy(&sys_parameter,&default_parameter,sizeof(SYS_PARAMETER));
	}
	else
	{
		//检查值是否有效
		if(sys_parameter.BtTrim > 0x1f)
			sys_parameter.BtTrim = default_parameter.BtTrim;
		if(sys_parameter.BtTrimECO0 > 0x1d || sys_parameter.BtTrimECO0 < 0x07)
			sys_parameter.BtTrimECO0 = default_parameter.BtTrimECO0;	
		if(sys_parameter.bt_TxPowerLevel > 23)
			sys_parameter.bt_TxPowerLevel = default_parameter.bt_TxPowerLevel;		
		if(sys_parameter.bt_PagePowerLevel > 23)
			sys_parameter.bt_PagePowerLevel = default_parameter.bt_PagePowerLevel;			
	}

}

