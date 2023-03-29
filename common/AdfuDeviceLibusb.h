//=================================================================================================
// AdfuDeviceLibusb
//
// descr.   adfu device implementation: access s1mp3 players via LibUsb
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#ifndef _REMOVE_ADFU_DEVICE_LIBUSB_

#ifdef LINUX
#else
#pragma once
#include <windows.h>
#endif
#include <string>
#include <list>
#include <cstdio>

#include "AdfuDevice.h"
#include "AdfuException.h"
#include "AdfuCbw.h"
#include "AdfuCsw.h"

#ifdef LINUX
#include <libusb.h>
#else
#include "libusb/libusb.h"  //also needs "libusb/libusb.lib"
#pragma comment(lib, "libusb.lib")
#endif


//-------------------------------------------------------------------------------------------------
#define	LIBUSB_VENDOR_ID   0x10D6
#define	LIBUSB_PRODUCT_ID  0xFF51 //NOTE: not verified anymore since there are multiple values


//-------------------------------------------------------------------------------------------------
// implement interface
//-------------------------------------------------------------------------------------------------
class AdfuDeviceLibusb : AdfuDevice {

///////////////////////////////////////////////////////////////////////////////////////////////////
public:AdfuDeviceLibusb() : hDevice(NULL) {/*::usb_set_debug(0);*/ ::libusb_init(NULL);}
public:~AdfuDeviceLibusb() {close(); ::libusb_exit(NULL);}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void close()
{
  if(hDevice != NULL)
  {
    ::libusb_release_interface(hDevice, 0);
    ::libusb_attach_kernel_driver(hDevice, 0);
    ::libusb_close(hDevice);
    hDevice = NULL;
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void open(std::string &strName)
{
  struct libusb_device **lppDevs, *lpDev;
  int i = 0;

  close();

  if (::libusb_get_device_list(NULL, &lppDevs) < 0) throw AdfuException(ERROR_OPEN_FAILED);

  while((lpDev = lppDevs[i++]) != NULL)
  {
    if(strName.compare(device_name(lpDev)) == 0)
    {
      if (::libusb_open(lpDev, &hDevice) != 0) hDevice = NULL;
      ::libusb_free_device_list(lppDevs, 1);
      if(hDevice == NULL) throw AdfuException(ERROR_OPEN_FAILED);
      ::libusb_detach_kernel_driver(hDevice, 0);
      if(::libusb_claim_interface(hDevice, 0) != 0) throw AdfuException(ERROR_OPEN_FAILED);
      return; //success
    }
  }
  ::libusb_free_device_list(lppDevs, 1);

  throw AdfuException(ERROR_OPEN_FAILED);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:unsigned int io(DIR nDir, const void *lpCdb, unsigned char uCdbLength,
                       void *lpData, unsigned int uDataLength, unsigned int uTimeout)
{
  unsigned int uResult = 0;

  AdfuCbw cbw(lpCdb, uCdbLength, 
    (nDir == IO_WRITE)? AdfuCbw::HOST_TO_DEVICE : AdfuCbw::DEVICE_TO_HOST, uDataLength);

  write(cbw.get(), cbw.size());
  if(lpData != NULL && uDataLength > 0) 
  {
    if(nDir == IO_WRITE) uResult = write(lpData, uDataLength);
    else if(nDir == IO_READ) uResult = read(lpData, uDataLength, uTimeout);
  }

  AdfuCsw csw;
  read(csw.get(), csw.size(), uTimeout);
  if(!csw.verify()) throw AdfuException(ERROR_FUNCTION_FAILED);

  return uResult;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void enumerate(std::list<std::string> &lstDevice)
{
  struct libusb_device **lppDevs, *lpDev;
  int i = 0;

  lstDevice.clear();
  
  if (::libusb_get_device_list(NULL, &lppDevs) < 0) return;

  while((lpDev = lppDevs[i++]) != NULL)
  {
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(lpDev, &desc);
    if(desc.idVendor == LIBUSB_VENDOR_ID /*&& desc.idProduct == LIBUSB_PRODUCT_ID*/)
    {
      std::string str = device_name(lpDev);
      lstDevice.push_back(str);
    }
  }
  ::libusb_free_device_list(lppDevs, 1);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:unsigned int write(const void *lp, unsigned int u, unsigned int uTimeout =3)
{
  if(hDevice == NULL) throw AdfuException(ERROR_INVALID_HANDLE);
  int nTransferred, nResult = 
    ::libusb_bulk_transfer(hDevice, 0x01, (unsigned char *)lp, u, &nTransferred, uTimeout * 1000);
  if(nResult < 0) throw AdfuException(ERROR_WRITE_FAULT);
  return (unsigned int)nTransferred;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:unsigned int read(const void *lp, unsigned int u, unsigned int uTimeout =3)
{
  if(hDevice == NULL) throw AdfuException(ERROR_INVALID_HANDLE);
  int nTransferred, nResult = 
    ::libusb_bulk_transfer(hDevice, 0x82, (unsigned char *)lp, u, &nTransferred, uTimeout * 1000);
  if(nResult < 0) throw AdfuException(ERROR_READ_FAULT);
  return (unsigned int)nTransferred;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:std::string device_name(libusb_device *lpDev)
{
  char cstr[9];
  std::snprintf(cstr, sizeof(cstr), "%d-%d",
    libusb_get_bus_number(lpDev),
    libusb_get_device_address(lpDev)
  );
  return std::string(cstr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:libusb_device_handle *hDevice;
};


#endif
