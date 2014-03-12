/**
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmsysOM3912Instance.c
 *                %version: 1 %
 *
 * \date          %date_modified%
 *
 * \brief         Describe briefly the purpose of this file.
 *
 * REFERENCE DOCUMENTS :
 *
 * Detailed description may be added here.
 *
 * \section info Change Information
 *
 * \verbatim
   Date          Modified by CRPRNr  TASKNr  Maintenance description
   -------------|-----------|-------|-------|-----------------------------------
   -------------|-----------|-------|-------|-----------------------------------
   -------------|-----------|-------|-------|-----------------------------------
   \endverbatim
 *
*/

#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmbslFrontEndCfgItem.h"
#include "tmFrontEnd.h"
#include "tmUnitParams.h"

#include "tmbslFrontEndTypes.h"
#include "tmsysFrontEndTypes.h"


#include "tmsysOM3912.h"
#include "tmsysOM3912local.h"
#include "tmsysOM3912Instance.h"


//-----------------------------------------------------------------------------
// Global data:
//-----------------------------------------------------------------------------
//
//
tmOM3912Object_t gOM3912Instance[] =
{
    {
        (tmUnitSelect_t)(-1),                                       /* OM3912 unit number */
        (tmUnitSelect_t)(-1),                                       /* OM3912 unit number temporary */
        (tmUnitSelect_t)(-1),                                       /* Tuner unit number */
        Null,                                                       /* pMutex */
        False,                                                      /* init */
        {                                                           /* sRWFunc */
            Null,
            Null
        },
        {                                                           /* sTime */
            Null,
            Null
        },
        {                                                           /* sDebug */
            Null
        },
        {                                                           /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        tmPowerStandby,                                             /* Power State */
        False,                                                      /* resetDone */
        {                                                           /* uHwAddress */
            0x00,                                                   /* tmOM3912UnitDeviceTypeUnknown */
            0xC0,                                                   /* tmOM3912UnitDeviceTypeTuner */
        },
    },
    {
        (tmUnitSelect_t)(-1),                                       /* OM3912 unit number */
        (tmUnitSelect_t)(-1),                                       /* OM3912 unit number temporary */
        (tmUnitSelect_t)(-1),                                       /* Tuner unit number */
        Null,                                                       /* pMutex */
        False,                                                      /* init */
        {                                                           /* sRWFunc */
            Null,
            Null
        },
        {                                                           /* sTime */
            Null,
            Null
        },
        {                                                           /* sDebug */
            Null
        },
        {                                                           /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        tmPowerStandby,                                             /* Power State */
        False,                                                      /* resetDone */
        {                                                           /* uHwAddress */
            0x00,                                                   /* tmOM3912UnitDeviceTypeUnknown */
            0xC6,                                                   /* tmOM3912UnitDeviceTypeTuner */
        },
    }
};


//-----------------------------------------------------------------------------
// FUNCTION:    OM3912AllocInstance:
//
// DESCRIPTION: allocate new instance
//
// RETURN:      
//
// NOTES:       
//-----------------------------------------------------------------------------
//
tmErrorCode_t
OM3912AllocInstance
(
    tmUnitSelect_t      tUnit,      /* I: Unit number */
    pptmOM3912Object_t  ppDrvObject /* I: Device Object */
)
{ 
    tmErrorCode_t       err = OM3912_ERR_BAD_UNIT_NUMBER;
    ptmOM3912Object_t   pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* Find a free instance */
    for(uLoopCounter = 0; uLoopCounter<OM3912_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gOM3912Instance[uLoopCounter];
        if(pObj->init == False)
        {
            pObj->tUnit = tUnit;
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}

//-----------------------------------------------------------------------------
// FUNCTION:    OM3912DeAllocInstance:
//
// DESCRIPTION: deallocate instance
//
// RETURN:      always TM_OK
//
// NOTES:       
//-----------------------------------------------------------------------------
//
tmErrorCode_t
OM3912DeAllocInstance
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{     
    tmErrorCode_t       err = OM3912_ERR_NOT_INITIALIZED;
    ptmOM3912Object_t   pObj = Null;

    /* check input parameters */
    err = OM3912GetInstance(tUnit, &pObj);

    /* check driver state */
    if (err == TM_OK)
    {
        if (pObj == Null || pObj->init == False)
        {
            err = OM3912_ERR_NOT_INITIALIZED;
        }
    }

    if ((err == TM_OK) && (pObj != Null)) 
    {
        pObj->init = False;
    }

    /* return value */
    return err;
}

//-----------------------------------------------------------------------------
// FUNCTION:    OM3912GetInstance:
//
// DESCRIPTION: get the instance
//
// RETURN:      always True
//
// NOTES:       
//-----------------------------------------------------------------------------
//
tmErrorCode_t
OM3912GetInstance
(
    tmUnitSelect_t      tUnit,      /* I: Unit number */
    pptmOM3912Object_t  ppDrvObject /* I: Device Object */
)
{     
    tmErrorCode_t       err = OM3912_ERR_NOT_INITIALIZED;
    ptmOM3912Object_t   pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* get instance */
    for(uLoopCounter = 0; uLoopCounter<OM3912_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gOM3912Instance[uLoopCounter];
        if(pObj->init == True && pObj->tUnit == GET_INDEX_TYPE_TUNIT(tUnit))
        {
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}
