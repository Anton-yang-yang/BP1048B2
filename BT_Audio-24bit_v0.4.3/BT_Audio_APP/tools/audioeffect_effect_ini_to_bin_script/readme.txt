2022/5/16
本脚本实现功能：
用户上位机导出的音效参数ini文件, 以hex格式生成bin文件。
多个ini文件会按照顺序拼接成一个大的flash bin文件。
flash bin文件包含引导头部分

说明
输入：目标ini文件
输出：bin文件

注意事项：
1、当个音效的有效长度小于4096字节
2、有效的音效文件目前最多支持10个
3、source目录下的文件需要按照数字编号开头来排布文件结构。
数字编号的顺序和SDK中的音效顺序表一致：
	EFFECT_MODE_FLASH_HFP_AEC = 0,
	EFFECT_MODE_FLASH_USBPHONE0_AEC = 1,
	EFFECT_MODE_FLASH_Music=2,
	EFFECT_MODE_FLASH_Movice=3,
	EFFECT_MODE_FLASH_News=4,
	EFFECT_MODE_FLASH_Game=5,
//	EFFECT_MODE_FLASH_NORMAL6,
//	EFFECT_MODE_FLASH_NORMAL7,
//	EFFECT_MODE_FLASH_NORMAL8,
//	EFFECT_MODE_FLASH_NORMAL9,

4、生成的flash bin 文件大小不能超过16K。文件大小可以调整。