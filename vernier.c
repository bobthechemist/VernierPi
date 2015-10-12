/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

/*
Desired functions:

1. get library version
2. get device name and description
3. get measurement units
4. get measurement


*/

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>
#include <sys/time.h>

#define MAX_NUM_MEASUREMENTS 100
#define GOIO_MAX_SIZE_DEVICE_NAME 260

#include "GoIO_DLL_interface.h"
#include "mathlink.h"

char *deviceDesc[8] = {"?", "?", "Go! Temp", "Go! Link", "Go! Motion", "?", "?", "Mini GC"};
bool GetAvailableDeviceName(char *deviceName, gtype_int32 nameLength, gtype_int32 *pVendorId, gtype_int32 *pProductId);

// My functions

void getlibraryversion();        //Done
void getdeviceinfo();            //Done
void getsimplemeasurement();
//void initialize(); 


int main(int argc, char* argv[])
{
	return MLMain(argc, argv);
}


void getlibraryversion() {
  gtype_uint16 MajorVersion;
	gtype_uint16 MinorVersion;
  char buff[75];
  GoIO_Init();
  GoIO_GetDLLVersion(&MajorVersion, &MinorVersion);
  sprintf(buff,"This app is linked to GoIO lib version %d.%d .", MajorVersion, MinorVersion);
  GoIO_Uninit();
  MLPutString(stdlink, buff);
   
  return;
}

void getdeviceinfo() {
  char deviceName[GOIO_MAX_SIZE_DEVICE_NAME];
	gtype_int32 vendorId;		//USB vendor id
	gtype_int32 productId;		//USB product id
  char tmpstring[100];
  char out[512];
  
  GoIO_Init();
  bool bFoundDevice = GetAvailableDeviceName(deviceName, GOIO_MAX_SIZE_DEVICE_NAME, &vendorId, &productId);
	if (!bFoundDevice)
		sprintf(out, "No Go devices found.\n");
  else 
  {
    GOIO_SENSOR_HANDLE hDevice = GoIO_Sensor_Open(deviceName, vendorId, productId, 0);
		if (hDevice != NULL)
		{    
      unsigned char charId;
			GoIO_Sensor_DDSMem_GetSensorNumber(hDevice, &charId, 0, 0);
      GoIO_Sensor_DDSMem_GetLongName(hDevice, tmpstring, sizeof(tmpstring));
			
      //Do we really need all this just to get the units?
      char units[20];
	    char equationType = 0;
      GoIO_Sensor_DDSMem_GetCalibrationEquation(hDevice, &equationType);
			gtype_real32 a, b, c;
			unsigned char activeCalPage = 0;
			GoIO_Sensor_DDSMem_GetActiveCalPage(hDevice, &activeCalPage);
			GoIO_Sensor_DDSMem_GetCalPage(hDevice, activeCalPage, &a, &b, &c, units, sizeof(units));
      //End of unit acquisition
      
      GoIO_Sensor_Close(hDevice);
      sprintf(out, "%s,%s,%d,%s,%s", deviceDesc[productId], deviceName, charId, tmpstring,units);
    }
  }
  GoIO_Uninit();
  MLPutString(stdlink,out);
  return;
}

//
void getsimplemeasurement(){
  char deviceName[GOIO_MAX_SIZE_DEVICE_NAME];
	gtype_int32 vendorId;		//USB vendor id
	gtype_int32 productId;		//USB product id
	char tmpstring[100];
	gtype_uint16 MajorVersion;
	gtype_uint16 MinorVersion;
	char units[20];
	char equationType = 0;

	gtype_int32 rawMeasurements[MAX_NUM_MEASUREMENTS];
	gtype_real64 volts[MAX_NUM_MEASUREMENTS];
	gtype_real64 calbMeasurements[MAX_NUM_MEASUREMENTS];
	gtype_int32 numMeasurements, i;
	gtype_real64 averageCalbMeasurement;
  
  
  GoIO_Init();
  bool bFoundDevice = GetAvailableDeviceName(deviceName, GOIO_MAX_SIZE_DEVICE_NAME, &vendorId, &productId);
	if (!bFoundDevice){
		MLPutDouble(stdlink,0);
    return;
  }
  else 
  {
    GOIO_SENSOR_HANDLE hDevice = GoIO_Sensor_Open(deviceName, vendorId, productId, 0);
		if (hDevice != NULL)
		{
      GoIO_Sensor_SetMeasurementPeriod(hDevice, 0.040, SKIP_TIMEOUT_MS_DEFAULT);//40 milliseconds measurement period.
			GoIO_Sensor_SendCmdAndGetResponse(hDevice, SKIP_CMD_ID_START_MEASUREMENTS, NULL, 0, NULL, NULL, SKIP_TIMEOUT_MS_DEFAULT);
			sleep(1); //Wait 1 second.

			numMeasurements = GoIO_Sensor_ReadRawMeasurements(hDevice, rawMeasurements, MAX_NUM_MEASUREMENTS);
      averageCalbMeasurement = 0.0;
			for (i = 0; i < numMeasurements; i++)
			{
				volts[i] = GoIO_Sensor_ConvertToVoltage(hDevice, rawMeasurements[i]);
				calbMeasurements[i] = GoIO_Sensor_CalibrateData(hDevice, volts[i]);
				averageCalbMeasurement += calbMeasurements[i];
			}
			if (numMeasurements > 1)
				averageCalbMeasurement = averageCalbMeasurement/numMeasurements;

			GoIO_Sensor_DDSMem_GetCalibrationEquation(hDevice, &equationType);
			gtype_real32 a, b, c;
			unsigned char activeCalPage = 0;
			GoIO_Sensor_DDSMem_GetActiveCalPage(hDevice, &activeCalPage);
			GoIO_Sensor_DDSMem_GetCalPage(hDevice, activeCalPage, &a, &b, &c, units, sizeof(units));
			
			GoIO_Sensor_Close(hDevice);
    }
  }
  GoIO_Uninit();
  MLPutDouble(stdlink,averageCalbMeasurement);
  return;
}

// cut and paste from SDK
bool GetAvailableDeviceName(char *deviceName, gtype_int32 nameLength, gtype_int32 *pVendorId, gtype_int32 *pProductId)
{
	bool bFoundDevice = false;
	deviceName[0] = 0;
	int numSkips = GoIO_UpdateListOfAvailableDevices(VERNIER_DEFAULT_VENDOR_ID, SKIP_DEFAULT_PRODUCT_ID);
	int numJonahs = GoIO_UpdateListOfAvailableDevices(VERNIER_DEFAULT_VENDOR_ID, USB_DIRECT_TEMP_DEFAULT_PRODUCT_ID);
	int numCyclopses = GoIO_UpdateListOfAvailableDevices(VERNIER_DEFAULT_VENDOR_ID, CYCLOPS_DEFAULT_PRODUCT_ID);
	int numMiniGCs = GoIO_UpdateListOfAvailableDevices(VERNIER_DEFAULT_VENDOR_ID, MINI_GC_DEFAULT_PRODUCT_ID);

	if (numSkips > 0)
	{
		GoIO_GetNthAvailableDeviceName(deviceName, nameLength, VERNIER_DEFAULT_VENDOR_ID, SKIP_DEFAULT_PRODUCT_ID, 0);
		*pVendorId = VERNIER_DEFAULT_VENDOR_ID;
		*pProductId = SKIP_DEFAULT_PRODUCT_ID;
		bFoundDevice = true;
	}
	else if (numJonahs > 0)
	{
		GoIO_GetNthAvailableDeviceName(deviceName, nameLength, VERNIER_DEFAULT_VENDOR_ID, USB_DIRECT_TEMP_DEFAULT_PRODUCT_ID, 0);
		*pVendorId = VERNIER_DEFAULT_VENDOR_ID;
		*pProductId = USB_DIRECT_TEMP_DEFAULT_PRODUCT_ID;
		bFoundDevice = true;
	}
	else if (numCyclopses > 0)
	{
		GoIO_GetNthAvailableDeviceName(deviceName, nameLength, VERNIER_DEFAULT_VENDOR_ID, CYCLOPS_DEFAULT_PRODUCT_ID, 0);
		*pVendorId = VERNIER_DEFAULT_VENDOR_ID;
		*pProductId = CYCLOPS_DEFAULT_PRODUCT_ID;
		bFoundDevice = true;
	}
	else if (numMiniGCs > 0)
	{
		GoIO_GetNthAvailableDeviceName(deviceName, nameLength, VERNIER_DEFAULT_VENDOR_ID, MINI_GC_DEFAULT_PRODUCT_ID, 0);
		*pVendorId = VERNIER_DEFAULT_VENDOR_ID;
		*pProductId = MINI_GC_DEFAULT_PRODUCT_ID;
		bFoundDevice = true;
	}

	return bFoundDevice;
}
