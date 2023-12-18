/**
 *************************************************************************************
 * @file	blue_ns.h
 * @brief	Noise Suppression for Mono Signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v2.1.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __BLUE_NS_H__
#define __BLUE_NS_H__

#include <stdint.h>


 /** error code for noise suppression */
typedef enum _BLUENS_ERROR_CODE
{
	BLUENS_ERROR_ILLEGAL_NS_LEVEL = -256,
	// No Error
	BLUENS_ERROR_OK = 0,					/**< no error              */
} BLUENS_ERROR_CODE;


/** noise suppression core structure for block size 128 */
typedef struct _BlueNSCore128
{
	float x_abs[128 + 1];
	float noise_pow[128 + 1];
	int32_t ph1_mean[128 + 1];
	float g[128 + 1];
	float gamma[128 + 1];
} BlueNSCore128;

/** noise suppression structure for block size 128 */
typedef struct _BlueNSContext128
{
	BlueNSCore128 ns_core;
	int16_t xin_prev[128];
	int16_t xout_prev[128];
	int32_t xv[128 * 2];
} BlueNSContext128;

/** noise suppression core structure for block size 256 */
typedef struct _BlueNSCore256
{
	float x_abs[256 + 1];
	float noise_pow[256 + 1];
	int32_t ph1_mean[256 + 1];
	float g[256 + 1];
	float gamma[256 + 1];
} BlueNSCore256;

/** noise suppression structure for block size 256 */
typedef struct _BlueNSContext256
{
	BlueNSCore256 ns_core;
	int16_t xin_prev[256];
	int16_t xout_prev[256];
	int32_t xv[256 * 2];
} BlueNSContext256;

/** noise suppression core structure for block size 512 */
typedef struct _BlueNSCore512
{
	float x_abs[512 + 1];
	float noise_pow[512 + 1];
	int32_t ph1_mean[512 + 1];
	float g[512 + 1];
	float gamma[512 + 1];
} BlueNSCore512;

/** noise suppression structure for block size 512 */
typedef struct _BlueNSContext512
{
	BlueNSCore512 ns_core;
	int16_t xin_prev[512];
	int16_t xout_prev[512];
	int32_t xv[512 * 2];
} BlueNSContext512;

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize noise suppression module.
 * @param ct Pointer to a BlueNSContext128 object.
 * @return none.
 * @note Only mono signals are supported.
 */
void blue_ns_init_128(BlueNSContext128 *ct);


/**
 * @brief Run noise suppression to a block of PCM data
 * @param ct Pointer to a BlueNSContext128 object.
 * @param xin Input PCM data. The size of xin is equal to 128.
 * @param xout Output PCM data. The size of xout is equal to 128. 
 *        xout can be the same as xin. In this case, the PCM is changed in-place.
 * @param ns_level Noise suppression level. Valid range: 0 ~ 9. Use 0 to disable noise suppression while 9 to apply maximum suppression. 
 * @return error code. BLUENS_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported. 
 */
int32_t blue_ns_run_128(BlueNSContext128 *ct, int16_t *xin, int16_t *xout, int32_t ns_level);


/**
 * @brief Initialize noise suppression module.
 * @param ct Pointer to a BlueNSContext256 object.
 * @return none.
 * @note Only mono signals are supported.
 */
void blue_ns_init_256(BlueNSContext256 *ct);


/**
 * @brief Run noise suppression to a block of PCM data
 * @param ct Pointer to a BlueNSContext256 object.
 * @param xin Input PCM data. The size of xin is equal to 256.
 * @param xout Output PCM data. The size of xout is equal to 256.
 *        xout can be the same as xin. In this case, the PCM is changed in-place.
 * @param ns_level Noise suppression level. Valid range: 0 ~ 9. Use 0 to disable noise suppression while 9 to apply maximum suppression.
 * @return error code. BLUENS_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported.
 */
int32_t blue_ns_run_256(BlueNSContext256 *ct, int16_t *xin, int16_t *xout, int32_t ns_level);


/**
 * @brief Initialize noise suppression module.
 * @param ct Pointer to a BlueNSContext512 object.
 * @return none.
 * @note Only mono signals are supported.
 */
void blue_ns_init_512(BlueNSContext512 *ct);


/**
 * @brief Run noise suppression to a block of PCM data
 * @param ct Pointer to a BlueNSContext512 object.
 * @param xin Input PCM data. The size of xin is equal to 512.
 * @param xout Output PCM data. The size of xout is equal to 512.
 *        xout can be the same as xin. In this case, the PCM is changed in-place.
 * @param ns_level Noise suppression level. Valid range: 0 ~ 9. Use 0 to disable noise suppression while 9 to apply maximum suppression.
 * @return error code. BLUENS_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported.
 */
int32_t blue_ns_run_512(BlueNSContext512 *ct, int16_t *xin, int16_t *xout, int32_t ns_level);


// ----------------------------------------------------------------------------------------------------------------------------
// ---------- The following functions are for internal use only. DONOT call them except you know what you are doing! ----------
// ----------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Initialize noise suppression core.
 * @param core1 Pointer to a BlueNSCore128/BlueNSCore256 object.
 * @return none.
 * @note Only mono signals are supported.
 */
void blue_ns_core_init(void *core1, int32_t blk_len);

/**
 * @brief Run noise suppression core to a block of spectrum data (usually after windowing & FFT process)
 * @param core1 Pointer to a BlueNSCore128/BlueNSCore256 object.
 * @param xfft Spectrum data (usually after windowing & FFT process). The size of xfft is equal to blk_len*2.
 *		Specaially, xfft[0] is for DC component, xfft[1] is for the highest frequency bin.
 *		xfft[2] & xfft[3] are for the 2nd frequency bin's real and imaginary part respectively,
 *		xfft[4] & xfft[5] are for the 3rd frequency bin's real and imaginary part respectively and so on
 * @param ns_level Noise suppression level. Valid range: 0 ~ 9. Use 0 to disable noise suppression while 9 to apply maximum suppression.
 * @return error code. NS_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported.
 */
int32_t blue_ns_core_run(void *core1, int32_t *xfft, int32_t ns_level, int32_t blk_len);
// ----------------------------------------------------------------------------------------------------------------------------
// ---------- The functions above are for internal use only. DONOT call them except you know what you are doing! --------------
// ----------------------------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__BLUE_NS_H__
