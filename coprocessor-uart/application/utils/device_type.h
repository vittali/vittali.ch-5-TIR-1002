/******************************************************************************

 @file device_type.h

 @brief This file contains definitions for device type identification

 @detail The module assigns a DeviceType_ID macro based on the defined
         device type in ti_drivers_config.h. If no board or device type
         is defined in ti_drivers_config.h, a generic DeviceType_ID is
         assigned.

 Group: LPRF SW RND
 $Target Device: DEVICES $

 ******************************************************************************
 $License: BSD3 2019 $
 ******************************************************************************
 $Release Name: PACKAGE NAME $
 $Release Date: PACKAGE RELEASE DATE $
 *****************************************************************************/

#ifndef DEVICE_TYPE_H
#define DEVICE_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 Includes
 *****************************************************************************/
#include "ti_drivers_config.h"

/******************************************************************************
 Constants and definitions
 *****************************************************************************/

/* DeviceType_ID_XYZ values */
#define DeviceType_ID_GENERIC       0
#define DeviceType_ID_CC1310        1
#define DeviceType_ID_CC1350        2
#define DeviceType_ID_CC2640R2      3
#define DeviceType_ID_CC1312R1      4
#define DeviceType_ID_CC1352R1      5
#define DeviceType_ID_CC1352P1      6
#define DeviceType_ID_CC1352P_2     7
#define DeviceType_ID_CC1352P_4     8
#define DeviceType_ID_CC2642R1      9
#define DeviceType_ID_CC2652R1      10
#define DeviceType_ID_CC2652RB      11

/*
 * Lookup table that sets DeviceType_ID based on the defined device type
 * in ti_drivers_config.h
 */
#if defined(CONFIG_CC1310_LAUNCHXL)
    #define DeviceType_ID       DeviceType_ID_CC1310
#elif defined(CONFIG_CC1350_LAUNCHXL)
    #define DeviceType_ID       DeviceType_ID_CC1350
#elif defined(CONFIG_CC2640R2_LAUNCHXL)
    #define DeviceType_ID       DeviceType_ID_CC2640R2
#elif defined(CONFIG_CC1312R1_LAUNCHXL) || defined(CONFIG_CC1312R1F3RGZ)
    #define DeviceType_ID       DeviceType_ID_CC1312R1
#elif defined(CONFIG_CC1352R1_LAUNCHXL) || defined(CONFIG_CC1352R1F3RGZ)
    #define DeviceType_ID       DeviceType_ID_CC1352R1
#elif defined(CONFIG_CC1352P1_LAUNCHXL) || defined(CONFIG_CC1352P1F3RGZ)
    #define DeviceType_ID       DeviceType_ID_CC1352P1
#elif defined(CONFIG_CC1352P_2_LAUNCHXL)
    #define DeviceType_ID       DeviceType_ID_CC1352P_2
#elif defined(CONFIG_CC1352P_4_LAUNCHXL)
    #define DeviceType_ID       DeviceType_ID_CC1352P_4
#elif defined(CONFIG_CC2642R1FRGZ)
    #define DeviceType_ID       DeviceType_ID_CC2642R1
#elif defined(CONFIG_CC26X2R1_LAUNCHXL) || defined(CONFIG_CC2652R1FRGZ)
    #define DeviceType_ID       DeviceType_ID_CC2652R1
#elif defined(CONFIG_CC2652RB_LAUNCHXL) || defined(CONFIG_CC2652RB)
    #define DeviceType_ID       DeviceType_ID_CC2652RB
#else
    #define DeviceType_ID       DeviceType_ID_GENERIC
#endif

/* Ensure that only one DeviceType was specified */
#if (defined(CONFIG_CC1310_LAUNCHXL) + defined(CONFIG_CC1350_LAUNCHXL)  \
    + defined(CONFIG_CC2640R2_LAUNCHXL)                                 \
    + defined(CONFIG_CC1312R1_LAUNCHXL) + defined(CONFIG_CC1312R1F3RGZ) \
    + defined(CONFIG_CC1352R1_LAUNCHXL) + defined(CONFIG_CC1352R1F3RGZ) \
    + defined(CONFIG_CC1352P1_LAUNCHXL) + defined(CONFIG_CC1352P1F3RGZ) \
    + defined(CONFIG_CC2642R1FRGZ)                                      \
    + defined(CONFIG_CC26X2R1_LAUNCHXL) + defined(CONFIG_CC2652R1FRGZ)  \
    + defined(CONFIG_CC2652RB_LAUNCHXL) + defined(CONFIG_CC2652RB)      \
    ) > 1
    #error "More then one DeviceType has been defined!"
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_TYPE_H */
