/*
* Licensee agrees that the example code provided to Licensee has been developed and released by Bosch solely as an example to be used as a potential reference for application development by Licensee. 
* Fitness and suitability of the example code for any use within application developed by Licensee need to be verified by Licensee on its own authority by taking appropriate state of the art actions and measures (e.g. by means of quality assurance measures).
* Licensee shall be responsible for conducting the development of its applications as well as integration of parts of the example code into such applications, taking into account the state of the art of technology and any statutory regulations and provisions applicable for such applications. Compliance with the functional system requirements and testing there of (including validation of information/data security aspects and functional safety) and release shall be solely incumbent upon Licensee. 
* For the avoidance of doubt, Licensee shall be responsible and fully liable for the applications and any distribution of such applications into the market.
* 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are 
* met:
* 
*     (1) Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer. 
* 
*     (2) Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.  
*     
*     (3)The name of the author may not be used to
*     endorse or promote products derived from this software without
*     specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
*  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
*  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
*  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
*  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
*  POSSIBILITY OF SUCH DAMAGE.
*/
/*----------------------------------------------------------------------------*/
/**
 * @ingroup APPS_LIST
 *
 * @defgroup XDK_APPLICATION_TEMPLATE noise-demo
 * @{
 *
 * @brief XDK Application Template
 *
 * @details XDK Application Template without any functionality.
 * Could be used as a starting point to develop new application based on XDK platform.
 *
 * @file
 **/
/* module includes ********************************************************** */

/* own header files */
#include "XdkAppInfo.h"
#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_APP_CONTROLLER

/* own header files */
#include "AppController.h"

/* system header files */
#include <stdio.h>

/* additional interface header files */
#include "BCDS_CmdProcessor.h"
#include "XDK_Utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include "XDK_WLAN.h"
#include "XDK_ServalPAL.h"
#include "XDK_HTTPRestClient.h"
#include "XDK_NoiseSensor.h"

/* constant definitions ***************************************************** */
#define  XDK_APP_DELAY      UINT32_C(1000)
/* local variables ********************************************************** */

static CmdProcessor_T * AppCmdProcessor;/**< Handle to store the main Command processor handle to be used by run-time event driven threads */

static xTaskHandle AppControllerHandle = NULL;/**< OS thread handle for Application controller to be used by run-time blocking threads */

static WLAN_Setup_T WLANSetupInfo =
{
		.IsEnterprise = false,
        .IsHostPgmEnabled = false,
        .SSID = YOUR_SSID_CHANGEME,
        .Username = "",
        .Password = YOUR_PASSWORD_CHANGEME,
        .IsStatic = false,
        .IpAddr = 0,
        .GwAddr = 0,
        .DnsAddr = 0,
        .Mask = 0,
};

static HTTPRestClient_Setup_T HTTPRestClientSetupInfo =
{
		.IsSecure = false,
};

static HTTPRestClient_Config_T HTTPRestClientConfigInfo =
{
		.IsSecure = false,
        .DestinationServerUrl = YOUR_URL_CHANGEME,
        .DestinationServerPort = UINT16_C(80),
        .RequestMaxDownloadSize = UINT32_C(512),
};

static HTTPRestClient_Post_T HTTPRestClientPostInfo =
{
		.Payload = "",
        .PayloadLength = 0U,
        .Url = "/data",
        .RequestCustomHeader0 = "",
        .RequestCustomHeader1 = "",
};

/* global variables ********************************************************* */

/* inline functions ********************************************************* */

/* local functions ********************************************************** */

/**
 * @brief Responsible for controlling application control flow.
 * Any application logic which is blocking in nature or fixed time dependent
 * can be placed here.
 *
 * @param[in] pvParameters
 * FreeRTOS task handle. Could be used if more than one thread is using this function block.
 */
static void AppControllerFire(void* pvParameters)
{
    BCDS_UNUSED(pvParameters);

    float rmsValue = 0.0f;
    char buffer[64];

    HTTPRestClientPostInfo.Payload = buffer;

    while (1)
    {
        NoiseSensor_ReadRmsValue(&rmsValue, 20UL);

        printf("RMS Value: %f", rmsValue);

        HTTPRestClientPostInfo.PayloadLength = snprintf(buffer, 64, " {\"value\" : %f} ", rmsValue);
        HTTPRestClient_Post(&HTTPRestClientConfigInfo, &HTTPRestClientPostInfo, 5000U);

        vTaskDelay(XDK_APP_DELAY);
    }
}

/**
 * @brief To enable the necessary modules for the application
 *
 * @param [in] param1
 * A generic pointer to any context data structure which will be passed to the function when it is invoked by the command processor.
 *
 * @param [in] param2
 * A generic 32 bit value  which will be passed to the function when it is invoked by the command processor..
 */
static void AppControllerEnable(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);
    Retcode_T retcode = RETCODE_OK;

    WLAN_Enable();
    ServalPAL_Enable();
    HTTPRestClient_Enable();
    NoiseSensor_Enable();

    if (RETCODE_OK == retcode)
    {
        if (pdPASS != xTaskCreate(AppControllerFire, (const char * const ) "AppController", TASK_STACK_SIZE_APP_CONTROLLER, NULL, TASK_PRIO_APP_CONTROLLER, &AppControllerHandle))
        {
            retcode = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_OUT_OF_RESOURCES);
        }
    }
    if (RETCODE_OK != retcode)
    {
        printf("AppControllerEnable : Failed \r\n");
        Retcode_RaiseError(retcode);
        assert(0); /* To provide LED indication for the user */
    }
    Utils_PrintResetCause();
}

/**
 * @brief To setup the necessary modules for the application
 *
 * @param [in] param1
 * A generic pointer to any context data structure which will be passed to the function when it is invoked by the command processor.
 *
 * @param [in] param2
 * A generic 32 bit value  which will be passed to the function when it is invoked by the command processor..
 */
static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);
    Retcode_T retcode = RETCODE_OK;

    WLAN_Setup(&WLANSetupInfo);
    ServalPAL_Setup(AppCmdProcessor);
    HTTPRestClient_Setup(&HTTPRestClientSetupInfo);
    NoiseSensor_Setup(20000UL);


    retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerEnable, NULL, UINT32_C(0));
    if (RETCODE_OK != retcode)
    {
        printf("AppControllerSetup : Failed \r\n");
        Retcode_RaiseError(retcode);
        assert(0); /* To provide LED indication for the user */
    }
}

/* global functions ********************************************************* */

/** Refer interface header for description */
void AppController_Init(void * cmdProcessorHandle, uint32_t param2)
{
    BCDS_UNUSED(param2);

    Retcode_T retcode = RETCODE_OK;

    if (cmdProcessorHandle == NULL)
    {
        printf("AppController_Init : Command processor handle is NULL \r\n");
        retcode = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_NULL_POINTER);
    }
    else
    {
        AppCmdProcessor = (CmdProcessor_T *) cmdProcessorHandle;
        retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerSetup, NULL, UINT32_C(0));
    }

    if (RETCODE_OK != retcode)
    {
        Retcode_RaiseError(retcode);
        assert(0); /* To provide LED indication for the user */
    }
}

/**@} */
/** ************************************************************************* */
