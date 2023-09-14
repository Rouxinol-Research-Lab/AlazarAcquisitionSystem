/**
 * @file
 *
 * @author Alazar Technologies Inc
 *
 * @copyright Copyright (c) 2016-2022 Alazar Technologies Inc. All Rights
 * Reserved.  Unpublished - rights reserved under the Copyright laws
 * of the United States And Canada.
 * This product contains confidential information and trade secrets
 * of Alazar Technologies Inc. Use, disclosure, or reproduction is
 * prohibited without the prior express written permission of Alazar
 * Technologies Inc
 */

#ifndef ALAZARRC_H
#define ALAZARRC_H

#include "AlazarError.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * @file AlazarRC.h
 *
 * Interface to the configuration file for the AlazarTech API.
 */

/**
 *  @cond INTERNAL_DECLARATIONS
 */
#ifndef PATH_MAX
#    define PATH_MAX 4096
#endif
/**
 *  @endcond
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns the logging state for the running application.
 *
 * Tries to read the logging information from the alazar configuration
 * file. If it fails, a default value is written to the file then
 * returned.
 *
 * @returns ApiFileIoError if writting the default value to disk fails.
 *
 * @returns ApiSuccess upon success
 */
RETURN_CODE AlazarGetLogState(bool *logState);

/**
 * @brief Return the log file path for the running application
 *
 * Tries to read the log file from the alazar configuration file. If
 * it fails, a default value is written to the file then returned.
 *
 * @returns ApiFileIoError if writting the default value to disk fails.
 *
 * @returns ApiSuccess upon success
 */
RETURN_CODE AlazarGetLogFileName(char *name, size_t max_len);

/**
 * @brief Returns the maximum log file size setting
 *
 * Tries to read the maximum log file size from the alazar
 * configuration file. If it fails, a default value is written to the
 * file then returned.
 *
 * @returns ApiFileIoError if writing the default value to disk fails.
 *
 * @returns ApiSuccess upin success
 */
RETURN_CODE AlazarGetMaxLogFileSize(size_t *max_file_size_bytes);

#ifdef __cplusplus
}
#endif

#endif // ALAZARRC_H
