//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Out of Box
// Application Overview - This Application demonstrates Out of Box Experience 
//                        with CC32xx Launch Pad. It highlights the following 
//                        features:
//                     1. Easy Connection to CC3200 Launchpad
//                        - Direct Connection to LP by using CC3200 device in
//                          Access Point Mode(Default)
//                        - Connection using TI SmartConfigTechnology
//                     2. Easy access to CC3200 Using Internal HTTP server and 
//                        on-board webcontent
//                     3. Attractive Demos
//                        - Home Automation
//                        - Appliance Control
//                        - Security System
//                        - Thermostat
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Out_of_Box_Application
// or
// docs\examples\CC32xx_Out_of_Box_Application.pdf
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup oob
//! @{
//
//****************************************************************************

// Standard includes
#include <stdlib.h>
#include <string.h>

// Simplelink includes
#include "simplelink.h"
#include "netcfg.h"
#include "device.h"

// Driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "utils.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "pin.h"

// OS includes
#include "osi.h"

// Common interface includes
#include "gpio_if.h"
#include "uart_if.h"
#include "i2c_if.h"
#include "common.h"

// App Includes
#include "device_status.h"
#include "smartconfig.h"
#include "tmp006drv.h"
#include "bma222drv.h"
#include "pinmux.h"
//New Includes
#include "mqtt/MQTTClient.h"

#define MQTT_TOPIC_LEDS "ticc3200/leds/#"
#define MQTT_TOPIC_BUTTON1 "ticc3200/buttons/1"
#define MQTT_TOPIC_BUTTON2 "ticc3200/buttons/2"
#define DEVICE_STRING_SUB "ticc3200-sub"
#define DEVICE_STRING_PUB "ticc3200-pub"

// MQTT message buffer size
#define BUFF_SIZE 32

#define min(X,Y) ((X) < (Y) ? (X) : (Y))

#define APPLICATION_VERSION              "1.1.0"
#define APP_NAME                         "Out of Box"
#define OOB_TASK_PRIORITY                2
#define BUTTON_NOTIFY_TASK_PRIORITY      2
#define SPAWN_TASK_PRIORITY              9
#define OSI_STACK_SIZE                   2048
#define AP_SSID_LEN_MAX                 32
#define SH_GPIO_3                       3       /* P58 - Device Mode */
#define AUTO_CONNECTION_TIMEOUT_COUNT   50      /* 5 Sec */
#define SL_STOP_TIMEOUT                 200

//#define USE_SSL

#ifdef USE_SSL
#define SL_SSL_CA_CERT_FILE_NAME        "/cert/129.der"
#endif

typedef enum
{
  LED_OFF = 0,
  LED_ON,
  LED_BLINK
}eLEDStatus;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static const char pcDigits[] = "0123456789";
static unsigned char POST_token[] = "__SL_P_ULD";
static unsigned char GET_token_TEMP[]  = "__SL_G_UTP";
static unsigned char GET_token_ACC[]  = "__SL_G_UAC";
static unsigned char GET_token_UIC[]  = "__SL_G_UIC";
static int g_iInternetAccess = -1;
static unsigned char g_ucDryerRunning = 0;
static unsigned int g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
static unsigned char g_ucLEDStatus = LED_OFF;
static unsigned long  g_ulStatus = 0;//SimpleLink Status
static unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
static unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


#ifdef USE_FREERTOS
//*****************************************************************************
//
//! Application defined hook (or callback) function - the tick hook.
//! The tick interrupt can optionally call this
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationTickHook( void )
{
    // MQTT library needs a millisecond tick too :)
    milliInterrupt();
}

//*****************************************************************************
//
//! Application defined hook (or callback) function - assert
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    while(1)
    {

    }
}

//*****************************************************************************
//
//! Application defined idle task hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationIdleHook( void )
{

}

//*****************************************************************************
//
//! Application provided stack overflow hook function.
//!
//! \param  handle of the offending task
//! \param  name  of the offending task
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationStackOverflowHook( OsiTaskHandle *pxTask, signed char *pcTaskName)
{
    ( void ) pxTask;
    ( void ) pcTaskName;

    for( ;; );
}

void vApplicationMallocFailedHook()
{
    while(1)
  {
    // Infinite loop;
  }
}
#endif

//*****************************************************************************
//
//! itoa
//!
//!    @brief  Convert integer to ASCII in decimal base
//!
//!     @param  cNum is input integer number to convert
//!     @param  cString is output string
//!
//!     @return number of ASCII parameters
//!
//!
//
//*****************************************************************************
static unsigned short itoa(char cNum, char *cString)
{
    char* ptr;
    char uTemp = cNum;
    unsigned short length;

    // value 0 is a special case
    if (cNum == 0)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    // Find out the length of the number, in decimal base
    length = 0;
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    uTemp = cNum;
    ptr = cString + length;
    while (uTemp > 0)
    {
        --ptr;
        *ptr = pcDigits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}


//*****************************************************************************
//
//! ReadAccSensor
//!
//!    @brief  Read Accelerometer Data from Sensor
//!
//!
//!     @return none
//!
//!
//
//*****************************************************************************
void ReadAccSensor()
{
    //Define Accelerometer Threshold to Detect Movement
    const short csAccThreshold    = 5;

    signed char cAccXT1,cAccYT1,cAccZT1;
    signed char cAccXT2,cAccYT2,cAccZT2;
    signed short sDelAccX, sDelAccY, sDelAccZ;
    int iRet = -1;
    int iCount = 0;
      
    iRet = BMA222ReadNew(&cAccXT1, &cAccYT1, &cAccZT1);
    if(iRet)
    {
        //In case of error/ No New Data return
        return;
    }
    for(iCount=0;iCount<2;iCount++)
    {
        MAP_UtilsDelay((90*80*1000)); //30msec
        iRet = BMA222ReadNew(&cAccXT2, &cAccYT2, &cAccZT2);
        if(iRet)
        {
            //In case of error/ No New Data continue
            iRet = 0;
            continue;
        }

        else
        {                       
            sDelAccX = abs((signed short)cAccXT2 - (signed short)cAccXT1);
            sDelAccY = abs((signed short)cAccYT2 - (signed short)cAccYT1);
            sDelAccZ = abs((signed short)cAccZT2 - (signed short)cAccZT1);

            //Compare with Pre defined Threshold
            if(sDelAccX > csAccThreshold || sDelAccY > csAccThreshold ||
               sDelAccZ > csAccThreshold)
            {
                //Device Movement Detected, Break and Return
                g_ucDryerRunning = 1;
                break;
            }
            else
            {
                //Device Movement Static
                g_ucDryerRunning = 0;
            }
        }
    }
       
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(pWlanEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'
            // Applications can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] Device Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT] Device disconnected from the AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on application's "
                           "request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR] Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
        {
            // when device is in AP mode and any client connects to device cc3xxx
            //SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModeStaConnected;
            //

            UART_PRINT("[WLAN EVENT] Station connected to device\n\r");
        }
        break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
        {
            // when client disconnects from device (AP)
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModestaDisconnected;
            //
            UART_PRINT("[WLAN EVENT] Station disconnected from device\n\r");
        }
        break;

        case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
        {
            //SET_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, SSID,
            // Token etc) will be available in 'slSmartConfigStartAsyncResponse_t'
            // - Applications can use it if required
            //
            //  slSmartConfigStartAsyncResponse_t *pEventData = NULL;
            //  pEventData = &pSlWlanEvent->EventData.smartConfigStartResponse;
            //

        }
        break;

        case SL_WLAN_SMART_CONFIG_STOP_EVENT:
        {
            // SmartConfig operation finished
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, padding
            // etc) will be available in 'slSmartConfigStopAsyncResponse_t' -
            // Applications can use it if required
            //
            // slSmartConfigStopAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.smartConfigStopResponse;
            //
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(pNetAppEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));

            UNUSED(pEventData);
        }
        break;

        case SL_NETAPP_IP_LEASED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Leased details(like IP-Leased,lease-time,
            // mac etc) will be available in 'SlIpLeasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpLeasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipLeased;
            //

        }
        break;

        case SL_NETAPP_IP_RELEASED_EVENT:
        {
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Released details (like IP-address, mac
            // etc) will be available in 'SlIpReleasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpReleasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipReleased;
            //
        }
		break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent, 
                                SlHttpServerResponse_t *pSlHttpServerResponse)
{
    switch (pSlHttpServerEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {
            unsigned char *ptr;

            ptr = pSlHttpServerResponse->ResponseData.token_value.data;
            pSlHttpServerResponse->ResponseData.token_value.len = 0;
            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, 
                    GET_token_TEMP, strlen((const char *)GET_token_TEMP)) == 0)
            {
                float fCurrentTemp;
                TMP006DrvGetTemp(&fCurrentTemp);
                char cTemp = (char)fCurrentTemp;
                short sTempLen = itoa(cTemp,(char*)ptr);
                ptr[sTempLen++] = ' ';
                ptr[sTempLen] = 'F';
                pSlHttpServerResponse->ResponseData.token_value.len += sTempLen;

            }

            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, 
                      GET_token_UIC, strlen((const char *)GET_token_UIC)) == 0)
            {
                if(g_iInternetAccess==0)
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"1");
                else
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"0");
                pSlHttpServerResponse->ResponseData.token_value.len =  1;
            }

            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, 
                       GET_token_ACC, strlen((const char *)GET_token_ACC)) == 0)
            {

                ReadAccSensor();
                if(g_ucDryerRunning)
                {
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"Running");
                    pSlHttpServerResponse->ResponseData.token_value.len += strlen("Running");
                }
                else
                {
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"Stopped");
                    pSlHttpServerResponse->ResponseData.token_value.len += strlen("Stopped");
                }
            }



        }
            break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {
            unsigned char led;
            unsigned char *ptr = pSlHttpServerEvent->EventData.httpPostData.token_name.data;

            //g_ucLEDStatus = 0;
            if(memcmp(ptr, POST_token, strlen((const char *)POST_token)) == 0)
            {
                ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
                if(memcmp(ptr, "LED", 3) != 0)
                    break;
                ptr += 3;
                led = *ptr;
                ptr += 2;
                if(led == '1')
                {
                    if(memcmp(ptr, "ON", 2) == 0)
                    {
                        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
                        g_ucLEDStatus = LED_ON;
                    }
                    else if(memcmp(ptr, "Blink", 5) == 0)
                    {
                        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
                        g_ucLEDStatus = LED_BLINK;
                    }
                    else
                    {
                        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
                        g_ucLEDStatus = LED_OFF;
                    }
                }
                else if(led == '2')
                {
                    if(memcmp(ptr, "ON", 2) == 0)
                    {
                        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
                    }
                    else if(memcmp(ptr, "Blink", 5) == 0)
                    {
                        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
                        g_ucLEDStatus = 1;
                    }
                    else
                    {
                        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
                    }
                }

            }
        }
            break;
        default:
            break;
    }
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    if(pDevEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(pSock == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    //
    // This application doesn't work w/ socket - Events are not expected
    //
       switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->EventData.status )
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                               "failed to transmit all queued packets\n\n",
                               pSock->EventData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , "
                               "reason (%d) \n\n",
                               pSock->EventData.sd, pSock->EventData.status);
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n", \
                        pSock->Event);
    }
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    None
//!
//! \return None
//!
//*****************************************************************************
static void InitializeAppVariables()
{
    g_ulStatus = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    g_iInternetAccess = -1;
    g_ucDryerRunning = 0;
    g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
    g_ucLEDStatus = LED_OFF;    
}


//****************************************************************************
//
//! Confgiures the mode in which the device will work
//!
//! \param iMode is the current mode of the device
//!
//!
//! \return   SlWlanMode_t
//!                        
//
//****************************************************************************
static int ConfigureMode(int iMode)
{
    long   lRetVal = -1;

    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);

    return sl_Start(NULL,NULL,NULL);
}


//****************************************************************************
//
//!    \brief Connects to the Network in AP or STA Mode - If ForceAP Jumper is
//!                                             Placed, Force it to AP mode
//!
//! \return  0 - Success
//!            -1 - Failure
//
//****************************************************************************
long ConnectToNetwork()
{
    long lRetVal = -1;
    unsigned int uiConnectTimeoutCnt =0;

    // staring simplelink
    UART_PRINT("\t\tStarting SimpleLink...\n\r");
    lRetVal =  sl_Start(NULL,NULL,NULL);
    UART_PRINT("\t\tDone!\n\r");
    ASSERT_ON_ERROR( lRetVal);

    //UART_PRINT("\t\tClearing WLAN profiles (debug!)...\n\r");
    //lRetVal = sl_WlanProfileDel(0xff);
    //ASSERT_ON_ERROR(lRetVal);
    //UART_PRINT("\t\tDone!\n\r");

    // Device is in AP Mode and Force AP Jumper is not Connected
    if(ROLE_STA != lRetVal && g_uiDeviceModeConfig == ROLE_STA )
    {
        UART_PRINT("\t\tDevice is in AP Mode and Force AP Jumper is not Connected\n\r");
        if (ROLE_AP == lRetVal)
        {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
            #ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
            #endif
            }
        }
        //Switch to STA Mode
        UART_PRINT("\t\tSwitch to STA Mode\n\r");
        lRetVal = ConfigureMode(ROLE_STA);
        ASSERT_ON_ERROR( lRetVal);
    }

    //Device is in STA Mode and Force AP Jumper is Connected
    if(ROLE_AP != lRetVal && g_uiDeviceModeConfig == ROLE_AP )
    {
         UART_PRINT("\t\tDevice is in STA Mode and Force AP Jumper is Connected\n\r");
         //Switch to AP Mode
         UART_PRINT("\t\tSwitch to AP Mode\n\r");
         lRetVal = ConfigureMode(ROLE_AP);
         ASSERT_ON_ERROR( lRetVal);

    }

    //No Mode Change Required
    if(lRetVal == ROLE_AP)
    {
        UART_PRINT("\t\tNo Mode Change Required (ROLE_AP)\n\r");
        //waiting for the AP to acquire IP address from Internal DHCP Server
        // If the device is in AP mode, we need to wait for this event 
        // before doing anything 
        UART_PRINT("\t\tWaiting for IP...\n\r");
        while(!IS_IP_ACQUIRED(g_ulStatus))
        {
        #ifndef SL_PLATFORM_MULTI_THREADED
            _SlNonOsMainLoopTask(); 
        #endif
        }
        UART_PRINT("\t\tDone!\n\r");
        //Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

        //Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

       char cCount=0;
       
       //Blink LED 3 times to Indicate AP Mode
       for(cCount=0;cCount<3;cCount++)
       {
           //Turn RED LED On
           GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           osi_Sleep(400);
           
           //Turn RED LED Off
           GPIO_IF_LedOff(MCU_RED_LED_GPIO);
           osi_Sleep(400);
       }

       char ssid[32];
	   unsigned short len = 32;
	   unsigned short config_opt = WLAN_AP_OPT_SSID;
	   sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (unsigned char* )ssid);
	   UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r",ssid);
    }
    else
    {
        UART_PRINT("\t\tNo Mode Change Required (ROLE_STA)\n\r");
        //Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

        //Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

    	//waiting for the device to Auto Connect
        UART_PRINT("\t\twaiting for the device to Auto Connect\n\r");
        while(uiConnectTimeoutCnt<AUTO_CONNECTION_TIMEOUT_COUNT &&
            ((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))) 
        {
            //Turn RED LED On
            GPIO_IF_LedOn(MCU_RED_LED_GPIO);
            osi_Sleep(50);
            
            //Turn RED LED Off
            GPIO_IF_LedOff(MCU_RED_LED_GPIO);
            osi_Sleep(50);
            
            uiConnectTimeoutCnt++;
        }
        //Couldn't connect Using Auto Profile
        if(uiConnectTimeoutCnt == AUTO_CONNECTION_TIMEOUT_COUNT)
        {
            UART_PRINT("\t\t\tCouldn't connect Using Auto Profile\n\r");
            //Blink Red LED to Indicate Connection Error
            GPIO_IF_LedOn(MCU_RED_LED_GPIO);
            
            CLR_STATUS_BIT_ALL(g_ulStatus);

            //Connect Using Smart Config
            UART_PRINT("\t\t\tConnect Using Smart Config\n\r");
            lRetVal = SmartConfigConnect();
            ASSERT_ON_ERROR(lRetVal);
            UART_PRINT("\t\t\tDone!\n\r");

            //Waiting for the device to Auto Connect
            UART_PRINT("\t\t\tWaiting for the device to Auto Connect\n\r");
            while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
            {
                MAP_UtilsDelay(500);              
            }
            UART_PRINT("\t\t\tDone!\n\r");
    }
    //Turn RED LED Off
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);

    UART_PRINT("\t\tTesting connection...\n\r");
    g_iInternetAccess = ConnectionTest();
    UART_PRINT("\t\tDone (%d)\n\r", g_iInternetAccess);

    }
    return SUCCESS;
}


//****************************************************************************
//
//!    \brief Read Force AP GPIO and Configure Mode - 1(Access Point Mode)
//!                                                  - 0 (Station Mode)
//!
//! \return                        None
//
//****************************************************************************
static void ReadDeviceConfiguration()
{
    unsigned int uiGPIOPort;
    unsigned char pucGPIOPin;
    unsigned char ucPinValue;
        
    //Read GPIO
    GPIO_IF_GetPortNPin(SH_GPIO_3,&uiGPIOPort,&pucGPIOPin);
    ucPinValue = GPIO_IF_Get(SH_GPIO_3,uiGPIOPort,pucGPIOPin);
        
    //If Connected to VCC, Mode is AP
    if(ucPinValue == 1)
    {
        //AP Mode
        g_uiDeviceModeConfig = ROLE_AP;
    }
    else
    {
        //STA Mode
        g_uiDeviceModeConfig = ROLE_STA;
    }

}

//****************************************************************************
//
//!    \brief MQTT message received callback - Called when a subscribed topic
//!                                            receives a message.
//! \param[in]                  data is the data passed to the callback
//!
//! \return                        None
//
//****************************************************************************
static void messageArrived(MessageData* data) {
	char buf[BUFF_SIZE];
	unsigned char pinNum;
	unsigned char ucPin;
	unsigned int uiGPIOPort;
	unsigned char ucGPIOPin;
	unsigned char ucGPIOValue;

	char *tok;

	// Check for buffer overflow
	if (data->topicName->lenstring.len >= BUFF_SIZE) {
		UART_PRINT("Topic name too long!\n\r");
		return;
	}
	if (data->message->payloadlen >= BUFF_SIZE) {
		UART_PRINT("Payload too long!\n\r");
		return;
	}

	strncpy(buf, data->topicName->lenstring.data,
		min(BUFF_SIZE, data->topicName->lenstring.len));
	buf[data->topicName->lenstring.len] = 0;


	UART_PRINT("topic:%.*s\n\r",
		data->topicName->lenstring.len,
		data->topicName->lenstring.data);
	UART_PRINT("payload:%.*s\n\r",
		data->message->payloadlen,
		data->message->payload);

	// Find pin number from topic string
	tok = strtok(buf, "/");

	for (int i=0;i<3&&tok!=NULL;i++) {
		if (i == 2) {
			if (strcmp(tok,"red")==0) {
				UART_PRINT("Red:");
				pinNum = 9;
			} else if (strcmp(tok,"green")==0) {
				UART_PRINT("Green:");
				pinNum = 11;
			} else if (strcmp(tok,"yellow")==0) {
				UART_PRINT("Yellow:");
				pinNum = 10;
			} else {
				UART_PRINT("Unrecognized led!\n\r");
				return;
			}
		}
		tok = strtok(NULL, "/");
	}

	strncpy(buf, data->message->payload,
		min(BUFF_SIZE, data->message->payloadlen));
	buf[data->message->payloadlen] = 0;
	
	// Find pin state from payload string
	if (strcmp(buf,"on")==0) {
		UART_PRINT("on\n\r");
		ucGPIOValue = 1;
	} else if (strcmp(buf,"off")==0) {
		UART_PRINT("off\n\r");
		ucGPIOValue = 0;
	} else {
		UART_PRINT("undefined\n\r");
		return;
	}

	GPIO_IF_GetPortNPin(pinNum, &uiGPIOPort, &ucGPIOPin);
	GPIO_IF_Set(pinNum, uiGPIOPort, ucGPIOPin, ucGPIOValue);

	return;
}

//****************************************************************************
//
//!    \brief OOB Application Main Task - Initializes SimpleLink Driver and
//!                                              Handles HTTP Requests
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void OOBTask(void *pvParameters)
{
    Network n;
    Client hMQTTClient;

    int rc = 0;
    unsigned char buf[100];
    unsigned char readbuf[100];

    long   lRetVal = -1;

    //Read Device Mode Configuration
    UART_PRINT("\tReading device configuration...\n\r");
    ReadDeviceConfiguration();
    UART_PRINT("\tDone!\n\r");

    //Connect to Network
    UART_PRINT("\tConnecting to network...\n\r");
    lRetVal = ConnectToNetwork();
    UART_PRINT("\tDone!\n\r");
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    NewNetwork(&n);
    UART_PRINT("Connecting Socket\n\r");
#ifdef USE_SSL	
    //Setup Secure connection information
    SlSockSecureFiles_t sockSecureFiles;
    sockSecureFiles.secureFiles[0] = 0;
    sockSecureFiles.secureFiles[1] = 0;
    sockSecureFiles.secureFiles[2] = 129;
    sockSecureFiles.secureFiles[3] = 0;
    rc = TLSConnectNetwork(&n, "test.mosquitto.org", 8883,
                           &sockSecureFiles,
			   SL_SO_SEC_METHOD_SSLv3_TLSV1_2,
			   SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA, 0);
#else	
    //Works - Unsecured
    rc = ConnectNetwork(&n, "test.mosquitto.org", 1883);
#endif
    UART_PRINT("Opened TCP Port to mqtt broker with return code %d\n\rConnecting to MQTT broker\n\r", rc);

    MQTTClient(&hMQTTClient, &n, 1000, buf, 100, readbuf, 100);
    MQTTPacket_connectData cdata = MQTTPacket_connectData_initializer;
    cdata.MQTTVersion = 3;
    cdata.keepAliveInterval = 10;
    cdata.clientID.cstring = (char*) DEVICE_STRING_SUB;
    rc = MQTTConnect(&hMQTTClient, &cdata);
    if (rc != 0) {
        UART_PRINT("rc from MQTT server is %d\n\r", rc);
    } else {
        UART_PRINT("connected to MQTT broker\n\r");
    }
    UART_PRINT("\tsocket=%d\n\r", n.my_socket);

    rc = MQTTSubscribe(&hMQTTClient, MQTT_TOPIC_LEDS, QOS0, messageArrived);
    if (rc != 0)
        UART_PRINT("rc from MQTT subscribe is %d\n\r", rc);
    //Handle Async Events
    while(1) {
        rc = MQTTYield(&hMQTTClient, 10);
        if (rc != 0) {
            UART_PRINT("rc = %d\n\r", rc);
        }
        osi_Sleep(50);
        UART_PRINT("Loop\n\r");
    }
}

//****************************************************************************
//
//! \brief ButtonNotify Task - Reads button states and publishes data to MQTT
//!                            broker.
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void ButtonNotifyTask(void *pvParameters)
{
    static unsigned int msg_id = 0;
    Network n;
    Client hMQTTClient;
    int rc = 0;
    unsigned char buf[100];
    unsigned char readbuf[100];

    unsigned char sw2_pinNum = 22;
    unsigned char sw2_ucPin;
    unsigned int sw2_uiGPIOPort;
    unsigned char sw2_ucGPIOPin;
    unsigned char sw2_ucGPIOValue;
    unsigned char sw2_ucSent;
    GPIO_IF_GetPortNPin(sw2_pinNum, &sw2_uiGPIOPort, &sw2_ucGPIOPin);
    
    unsigned char sw3_pinNum = 13;
    unsigned char sw3_ucPin;
    unsigned int sw3_uiGPIOPort;
    unsigned char sw3_ucGPIOPin;
    unsigned char sw3_ucGPIOValue;
    unsigned char sw3_ucSent;
    GPIO_IF_GetPortNPin(sw3_pinNum, &sw3_uiGPIOPort, &sw3_ucGPIOPin);

    // Wait for a connection
    while (1) {
        while (g_iInternetAccess != 0) {}

        NewNetwork(&n);
        UART_PRINT("Connecting Socket\n\r");
#ifdef USE_SSL  
        //Setup Secure connection information
        SlSockSecureFiles_t sockSecureFiles;
        sockSecureFiles.secureFiles[0] = 0;
        sockSecureFiles.secureFiles[1] = 0;
        sockSecureFiles.secureFiles[2] = 129;
        sockSecureFiles.secureFiles[3] = 0;
        rc = TLSConnectNetwork(&n, "test.mosquitto.org", 8883,
                               &sockSecureFiles,
                               SL_SO_SEC_METHOD_SSLv3_TLSV1_2,
                               SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA, 0);
#else
        //Works - Unsecured
        rc = ConnectNetwork(&n, "test.mosquitto.org", 1883);
#endif
        UART_PRINT("Opened TCP Port to mqtt broker with return code %d\n\rConnecting to MQTT broker\n\r", rc);

        MQTTClient(&hMQTTClient, &n, 1000, buf, 100, readbuf, 100);
        MQTTPacket_connectData cdata = MQTTPacket_connectData_initializer;
        cdata.MQTTVersion = 3;
        cdata.keepAliveInterval = 10;
        cdata.clientID.cstring = (char*) DEVICE_STRING_PUB;
        rc = MQTTConnect(&hMQTTClient, &cdata);
        if (rc != 0) {
            UART_PRINT("rc from MQTT server is %d\n\r", rc);
        } else {
            UART_PRINT("connected to MQTT broker\n\r");
        }
	UART_PRINT("\tsocket=%d\n\r", n.my_socket);
    
        while (1) {
            sw2_ucGPIOValue = GPIO_IF_Get(sw2_pinNum, sw2_uiGPIOPort, sw2_ucGPIOPin);
            sw3_ucGPIOValue = GPIO_IF_Get(sw3_pinNum, sw3_uiGPIOPort, sw3_ucGPIOPin);
            if (sw2_ucGPIOValue) {
                if (!sw2_ucSent) {
	            UART_PRINT("SW2\n\r");
                    sw2_ucSent = 1;
                    MQTTMessage msg;
                    msg.dup = 0;
                    msg.id = msg_id++;
                    msg.payload = "pushed";
                    msg.payloadlen = 7;
                    msg.qos = QOS0;
                    msg.retained = 0;
                    int rc = MQTTPublish(&hMQTTClient, MQTT_TOPIC_BUTTON1, &msg);
                    UART_PRINT("SW2, rc=%d\n\r", rc);
		    if (rc != 0)
                        break;
                }
            } else {
                sw2_ucSent = 0;
            }
            if (sw3_ucGPIOValue) {
                if (!sw3_ucSent) {
	            UART_PRINT("SW3\n\r");
	            sw3_ucSent = 1;
                    MQTTMessage msg;
                    msg.dup = 0;
                    msg.id = msg_id++;
                    msg.payload = "pushed";
                    msg.payloadlen = 7;
                    msg.qos = QOS0;
                    msg.retained = 0;
                    int rc = MQTTPublish(&hMQTTClient, MQTT_TOPIC_BUTTON2, &msg);
                    UART_PRINT("SW3, rc=%d\n\r", rc);
		    if (rc != 0)
                        break;
                }
            } else {
                sw3_ucSent = 0;
            }
            rc = MQTTYield(&hMQTTClient, 10);
            if (rc != 0) {
                UART_PRINT("rc = %d\n\r", rc);
		break;
            }
            osi_Sleep(50);
        }
    }
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t     CC3200 %s Application       \n\r", AppName);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif  //ccs
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif  //ewarm
    
#endif  //USE_TIRTOS
    
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//****************************************************************************
//                            MAIN FUNCTION
//****************************************************************************
void main()
{
    long   lRetVal = -1;

    //
    // Board Initilization
    //
    BoardInit();
    
    //
    // Configure the pinmux settings for the peripherals exercised
    //
    PinMuxConfig();

    PinConfigSet(PIN_58,PIN_STRENGTH_2MA|PIN_STRENGTH_4MA,PIN_TYPE_STD_PD);

    // Initialize Global Variables
    InitializeAppVariables();
    
    //
    // UART Init
    //
    InitTerm();
    
    DisplayBanner(APP_NAME);

    //
    // LED Init
    //
    GPIO_IF_LedConfigure(LED1);
      
    //Turn Off the LEDs
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);

    
    //
    // Simplelinkspawntask
    //
    UART_PRINT("Spawning tasks...\n\r");
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }    
    
    //
    // Create OOB Task
    //
    UART_PRINT("Creating OOB task...\n\r");
    lRetVal = osi_TaskCreate(OOBTask, (signed char*)"OOBTask", \
                                OSI_STACK_SIZE, NULL, \
                                OOB_TASK_PRIORITY, NULL );
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }    

    //
    // Create ButtonNotify Task
    //
    UART_PRINT("Creating ButtonNotify task...\n\r");
    lRetVal = osi_TaskCreate(ButtonNotifyTask, (signed char*)"BtnTask", \
                                OSI_STACK_SIZE, NULL, \
                                BUTTON_NOTIFY_TASK_PRIORITY, NULL );
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start OS Scheduler
    //
    UART_PRINT("Starting OS scheduler...\n\r");
    osi_start();

    while (1)
    {

    }

}
