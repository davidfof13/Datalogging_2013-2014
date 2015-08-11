/* Copyright (c) 2010-2012 mbed.org, MIT License
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software
* and associated documentation files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or
* substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
* BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define __DEBUG__ 0
#ifndef __MODULE__
#define __MODULE__ "WANDongleInitializer.cpp"
#endif

#include "core/dbg.h"

#include <cstdint>
using std::uint16_t;

#include "WANDongleInitializer.h"

WANDongleInitializer::WANDongleInitializer(USBHost* pHost) : m_pHost(pHost)
{

}

WANDongleInitializer** WANDongleInitializer::getInitializers(USBHost* pHost)
{
  static VodafoneK3770Initializer vodafoneK3770(pHost);
  static VodafoneK3772ZInitializer vodafoneK3772Z(pHost);
  static VodafoneK3772Initializer vodafoneK3772(pHost);
  static VodafoneK3773Initializer vodafoneK3773(pHost);
  static HuaweiMU509Initializer huaweiMU509(pHost);
  static UbloxLISAU200Initializer ubloxLISAU200(pHost);
  const static WANDongleInitializer* list[] = {
     &vodafoneK3770,
     &vodafoneK3772Z,
     &vodafoneK3772,
     &vodafoneK3773,
     &huaweiMU509,
     &ubloxLISAU200,
     NULL
  };
  return (WANDongleInitializer**)list;
}

//Huawei K3770 (Vodafone)
// Switching from mass storage device string is: "55 53 42 43 12 34 56 78 00 00 00 00 00 00 00 11 06 20 00 00 01 00 00 00 00 00 00 00 00 00 00"
static uint8_t vodafone_k3770_switch_packet[] = {
    0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78, 0, 0, 0, 0, 0, 0, 0, 0x11, 0x06, 0x20, 0, 0, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

VodafoneK3770Initializer::VodafoneK3770Initializer(USBHost* pHost) : WANDongleInitializer(pHost)
{
  
}

uint16_t VodafoneK3770Initializer::getMSDVid()      { return 0x12D1; }
uint16_t VodafoneK3770Initializer::getMSDPid()      { return 0x14D1; }

uint16_t VodafoneK3770Initializer::getSerialVid()   { return 0x12D1; }
uint16_t VodafoneK3770Initializer::getSerialPid()   { return 0x14C9; }

bool VodafoneK3770Initializer::switchMode(USBDeviceConnected* pDev)
{
  for (int i = 0; i < pDev->getNbInterface(); i++) 
  {
    if (pDev->getInterface(i)->intf_class == MSD_CLASS)
    {
      USBEndpoint* pEp = pDev->getEndpoint(i, BULK_ENDPOINT, OUT);
      if ( pEp != NULL ) 
      {
        DBG("MSD descriptor found on device %p, intf %d, will now try to switch into serial mode", (void *)pDev, i);
        m_pHost->bulkWrite(pDev, pEp, vodafone_k3770_switch_packet, 31);
        return true;
      }
    }  
  }
  return false;
}

USBEndpoint* VodafoneK3770Initializer::getEp(USBDeviceConnected* pDev, int serialPortNumber, bool tx)
{
  return pDev->getEndpoint(serialPortNumber, BULK_ENDPOINT, tx?OUT:IN, 0);
}

int VodafoneK3770Initializer::getSerialPortCount()
{
  return 2;
}

/*virtual*/ void VodafoneK3770Initializer::setVidPid(uint16_t vid, uint16_t pid)
{
    if( (vid == getSerialVid() ) && ( pid == getSerialPid() ) )
    {
      m_hasSwitched = true;
      m_currentSerialIntf = 0;
      m_endpointsToFetch = 4;
    }
    else
    {
      m_hasSwitched = false;
      m_endpointsToFetch = 1;
    }
}

/*virtual*/ bool VodafoneK3770Initializer::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
  if( m_hasSwitched )
  {
    if( intf_class == 0xFF )
    {
      if( (m_currentSerialIntf == 0) || (m_currentSerialIntf == 4) )
      {
        m_currentSerialIntf++;
        return true;
      }
      m_currentSerialIntf++;
    }
  }
  else
  {
    if( (intf_nb == 0) && (intf_class == MSD_CLASS) )
    {
      return true;
    }
  }
  return false;
}

/*virtual*/ bool VodafoneK3770Initializer::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
  if( m_hasSwitched )
  {
    if( (type == BULK_ENDPOINT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  else
  {
    if( (type == BULK_ENDPOINT) && (dir == OUT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  return false;
}

/*virtual*/ WAN_DONGLE_TYPE VodafoneK3770Initializer::getType()
{
  return WAN_DONGLE_TYPE_VODAFONE_K3770;
}

//Huawei MU509 (Module)

HuaweiMU509Initializer::HuaweiMU509Initializer(USBHost* pHost) : WANDongleInitializer(pHost)
{
  
}

uint16_t HuaweiMU509Initializer::getMSDVid()      { return 0x12D1; }
uint16_t HuaweiMU509Initializer::getMSDPid()      { return 0x0000; } //No MSD mode (presumably)

uint16_t HuaweiMU509Initializer::getSerialVid()   { return 0x12D1; }
uint16_t HuaweiMU509Initializer::getSerialPid()   { return 0x1001; }

bool HuaweiMU509Initializer::switchMode(USBDeviceConnected* pDev)
{
  return true; //No MSD mode
}

USBEndpoint* HuaweiMU509Initializer::getEp(USBDeviceConnected* pDev, int serialPortNumber, bool tx)
{
  /*
  int sPort = serialPortNumber;
  if(sPort==1)
     sPort = 0;
  if(sPort==2)
     sPort = 1;
  if(sPort==0)
     sPort = 2;
  return pDev->getEndpoint(sPort, BULK_ENDPOINT, tx?OUT:IN, 0);
  */
  return pDev->getEndpoint(serialPortNumber, BULK_ENDPOINT, tx?OUT:IN, 0);
  //return pDev->getEndpoint((serialPortNumber==1)?0:1, BULK_ENDPOINT, tx?OUT:IN, 0);
}

int HuaweiMU509Initializer::getSerialPortCount()
{
  return 2;
}

/*virtual*/ void HuaweiMU509Initializer::setVidPid(uint16_t vid, uint16_t pid)
{
  m_currentSerialIntf = 0;
  m_endpointsToFetch = 4;
}

/*virtual*/ bool HuaweiMU509Initializer::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
  if( intf_class == 0xFF )
  {
    if( (m_currentSerialIntf == 0) || (m_currentSerialIntf == 2) )
    {
      m_currentSerialIntf++;
      return true;
    }
    m_currentSerialIntf++;
  }
  return false;
}

/*virtual*/ bool HuaweiMU509Initializer::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
  if( (type == BULK_ENDPOINT) && m_endpointsToFetch )
  {
    DBG("new endpoint");
    m_endpointsToFetch--;
    return true;
  }
  return false;
}

/*virtual*/ WAN_DONGLE_TYPE HuaweiMU509Initializer::getType()
{
  return WAN_DONGLE_TYPE_HUAWEI_MU509;
}

//Huawei K3773 (Vodafone)
// Switching from mass storage device string is: "55 53 42 43 12 34 56 78 00 00 00 00 00 00 00 11 06 20 00 00 01 00 00 00 00 00 00 00 00 00 00"
static uint8_t vodafone_k3773_switch_packet[] = {
    0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78, 0, 0, 0, 0, 0, 0, 0, 0x11, 0x06, 0x20, 0, 0, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

VodafoneK3773Initializer::VodafoneK3773Initializer(USBHost* pHost) : WANDongleInitializer(pHost)
{
  
}

uint16_t VodafoneK3773Initializer::getMSDVid()      { return 0x12D1; }
uint16_t VodafoneK3773Initializer::getMSDPid()      { return 0x1506; }

uint16_t VodafoneK3773Initializer::getSerialVid()   { return 0x12D1; }
uint16_t VodafoneK3773Initializer::getSerialPid()   { return 0x1506; }

bool VodafoneK3773Initializer::switchMode(USBDeviceConnected* pDev)
{
  for (int i = 0; i < pDev->getNbInterface(); i++) 
  {
    if (pDev->getInterface(i)->intf_class == MSD_CLASS)
    {
      USBEndpoint* pEp = pDev->getEndpoint(i, BULK_ENDPOINT, OUT);
      if ( pEp != NULL ) 
      {
        DBG("MSD descriptor found on device %p, intf %d, will now try to switch into serial mode", (void *)pDev, i);
        m_pHost->bulkWrite(pDev, pEp, vodafone_k3773_switch_packet, 31);
        return true;
      }
    }  
  }
  return false;
}

USBEndpoint* VodafoneK3773Initializer::getEp(USBDeviceConnected* pDev, int serialPortNumber, bool tx)
{
  return pDev->getEndpoint(serialPortNumber, BULK_ENDPOINT, tx?OUT:IN, 0);
}

int VodafoneK3773Initializer::getSerialPortCount()
{
  return 2;
}

/*virtual*/ void VodafoneK3773Initializer::setVidPid(uint16_t vid, uint16_t pid)
{
    if( (vid == getSerialVid() ) && ( pid == getSerialPid() ) )
    {
      m_hasSwitched = true;
      m_currentSerialIntf = 0;
      m_endpointsToFetch = 4;
    }
    else
    {
      m_hasSwitched = false;
      m_endpointsToFetch = 1;
    }
}

/*virtual*/ bool VodafoneK3773Initializer::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
  if( m_hasSwitched )
  {
    if( intf_class == 0xFF )
    {
      if( (m_currentSerialIntf == 0) || (m_currentSerialIntf == 4) )
      {
        m_currentSerialIntf++;
        return true;
      }
      m_currentSerialIntf++;
    }
  }
  else
  {
    if( (intf_nb == 0) && (intf_class == MSD_CLASS) )
    {
      return true;
    }
  }
  return false;
}

/*virtual*/ bool VodafoneK3773Initializer::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
  if( m_hasSwitched )
  {
    if( (type == BULK_ENDPOINT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  else
  {
    if( (type == BULK_ENDPOINT) && (dir == OUT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  return false;
}

/*virtual*/ WAN_DONGLE_TYPE VodafoneK3773Initializer::getType()
{
  return WAN_DONGLE_TYPE_VODAFONE_K3773;
}

// NVIDIA (ICERA) /ZTE K3772-Z (Vodafone)
// Switching from mass storage device string is: "55 53 42 43 12 34 56 78 00 00 00 00 00 00 06 1b 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00"
static uint8_t vodafone_k3772_z_switch_packet[] = {
    0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78, 0, 0, 0, 0, 0, 0, 0x06, 0x1b, 0, 0, 0, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

VodafoneK3772ZInitializer::VodafoneK3772ZInitializer(USBHost* pHost) : WANDongleInitializer(pHost)
{
  
}

uint16_t VodafoneK3772ZInitializer::getMSDVid()      { return 0x19D2; }
uint16_t VodafoneK3772ZInitializer::getMSDPid()      { return 0x1179; }

uint16_t VodafoneK3772ZInitializer::getSerialVid()   { return 0x19D2; }
uint16_t VodafoneK3772ZInitializer::getSerialPid()   { return 0x1181; }

bool VodafoneK3772ZInitializer::switchMode(USBDeviceConnected* pDev)
{
  for (int i = 0; i < pDev->getNbInterface(); i++) 
  {
    if (pDev->getInterface(i)->intf_class == MSD_CLASS)
    {
      USBEndpoint* pEp = pDev->getEndpoint(i, BULK_ENDPOINT, OUT);
      if ( pEp != NULL ) 
      {
        DBG("MSD descriptor found on device %p, intf %d, will now try to switch into serial mode", (void *)pDev, i);
        m_pHost->bulkWrite(pDev, pEp, vodafone_k3772_z_switch_packet, 31);
        return true;
      }
    }  
  }
  return false;
}

USBEndpoint* VodafoneK3772ZInitializer::getEp(USBDeviceConnected* pDev, int serialPortNumber, bool tx)
{
  return pDev->getEndpoint((serialPortNumber==1)?0:1, BULK_ENDPOINT, tx?OUT:IN, 0);
}

int VodafoneK3772ZInitializer::getSerialPortCount()
{
  return 2;
}

/*virtual*/ void VodafoneK3772ZInitializer::setVidPid(uint16_t vid, uint16_t pid)
{
    if( (vid == getSerialVid() ) && ( pid == getSerialPid() ) )
    {
      m_hasSwitched = true;
      m_currentSerialIntf = 0;
      m_endpointsToFetch = 4;
    }
    else
    {
      m_hasSwitched = false;
      m_endpointsToFetch = 1;
    }
}

/*virtual*/ bool VodafoneK3772ZInitializer::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
  if( m_hasSwitched )
  {
    DBG("Interface #%d; Class:%02x; SubClass:%02x; Protocol:%02x", intf_nb, intf_class, intf_subclass, intf_protocol);
    if( intf_class == 0x0A )
    {
      if( (m_currentSerialIntf == 0) || (m_currentSerialIntf == 1) )
      {
        m_currentSerialIntf++;
        return true;
      }
      m_currentSerialIntf++;
    }
  }
  else
  {
    if( (intf_nb == 0) && (intf_class == MSD_CLASS) )
    {
      return true;
    }
  }
  return false;
}

/*virtual*/ bool VodafoneK3772ZInitializer::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
  if( m_hasSwitched )
  {
    DBG("USBEndpoint on Inteface #%d; Type:%d; Direction:%d", intf_nb, type, dir);
    if( (type == BULK_ENDPOINT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  else
  {
    if( (type == BULK_ENDPOINT) && (dir == OUT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  return false;
}


/*virtual*/ WAN_DONGLE_TYPE VodafoneK3772ZInitializer::getType()
{
  return WAN_DONGLE_TYPE_VODAFONE_K3772Z;
}

//Huawei K3772 (Vodafone)
// Switching from mass storage device string is: "55 53 42 43 12 34 56 78 00 02 00 00 80 00 0a 11 06 20 00 00 00 00 00 01 00 00 00 00 00 00 00"
static uint8_t vodafone_k3772_switch_packet[] = {
    0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78, 0, 0x02, 0, 0, 0x80, 0, 0x0a, 0x11, 0x06, 0x20, 0, 0, 0, 0, 0, 0x01, 0, 0, 0, 0, 0, 0, 0
};


VodafoneK3772Initializer::VodafoneK3772Initializer(USBHost* pHost) : WANDongleInitializer(pHost)
{
  
}

uint16_t VodafoneK3772Initializer::getMSDVid()      { return 0x12D1; }
uint16_t VodafoneK3772Initializer::getMSDPid()      { return 0x1526; }

uint16_t VodafoneK3772Initializer::getSerialVid()   { return 0x12D1; }
uint16_t VodafoneK3772Initializer::getSerialPid()   { return 0x14CF; }

bool VodafoneK3772Initializer::switchMode(USBDeviceConnected* pDev)
{
  for (int i = 0; i < pDev->getNbInterface(); i++) 
  {
    if (pDev->getInterface(i)->intf_class == MSD_CLASS)
    {
      USBEndpoint* pEp = pDev->getEndpoint(i, BULK_ENDPOINT, OUT);
      if ( pEp != NULL ) 
      {
        DBG("MSD descriptor found on device %p, intf %d, will now try to switch into serial mode", (void *)pDev, i);
        m_pHost->bulkWrite(pDev, pEp, vodafone_k3772_switch_packet, 31);
        return true;
      }
    }  
  }
  return false;
}

USBEndpoint* VodafoneK3772Initializer::getEp(USBDeviceConnected* pDev, int serialPortNumber, bool tx)
{
  return pDev->getEndpoint(serialPortNumber, BULK_ENDPOINT, tx?OUT:IN, 0);
}

int VodafoneK3772Initializer::getSerialPortCount()
{
  return 2;
}

/*virtual*/ void VodafoneK3772Initializer::setVidPid(uint16_t vid, uint16_t pid)
{
    if( (vid == getSerialVid() ) && ( pid == getSerialPid() ) )
    {
      m_hasSwitched = true;
      m_currentSerialIntf = 0;
      m_endpointsToFetch = 4;
    }
    else
    {
      m_hasSwitched = false;
      m_endpointsToFetch = 1;
    }
}

/*virtual*/ bool VodafoneK3772Initializer::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
  if( m_hasSwitched )
  {
    if( intf_class == 0xFF )
    {
      if( (m_currentSerialIntf == 0) || (m_currentSerialIntf == 2) )
      {
        m_currentSerialIntf++;
        return true;
      }
      m_currentSerialIntf++;
    }
  }
  else
  {
    if( (intf_nb == 0) && (intf_class == MSD_CLASS) )
    {
      return true;
    }
  }
  return false;
}

/*virtual*/ bool VodafoneK3772Initializer::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
  if( m_hasSwitched )
  {
    if( (type == BULK_ENDPOINT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  else
  {
    if( (type == BULK_ENDPOINT) && (dir == OUT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  return false;
}

/*virtual*/ WAN_DONGLE_TYPE VodafoneK3772Initializer::getType()
{
  return WAN_DONGLE_TYPE_VODAFONE_K3772;
}


//-----------------------------------------------------------------------
// mamm, u-blox Modem
//-----------------------------------------------------------------------

UbloxLISAU200Initializer::UbloxLISAU200Initializer(USBHost* pHost) : WANDongleInitializer(pHost)
{
  
}

uint16_t UbloxLISAU200Initializer::getMSDVid()      { return 0x1546; }
uint16_t UbloxLISAU200Initializer::getMSDPid()      { return 0x0000; }

uint16_t UbloxLISAU200Initializer::getSerialVid()   { return 0x1546; }
uint16_t UbloxLISAU200Initializer::getSerialPid()   { return 0x1102; }

bool UbloxLISAU200Initializer::switchMode(USBDeviceConnected* pDev)
{
  for (int i = 0; i < pDev->getNbInterface(); i++) 
  {
    if (pDev->getInterface(i)->intf_class == MSD_CLASS)
    {
      USBEndpoint* pEp = pDev->getEndpoint(i, BULK_ENDPOINT, OUT);
      if ( pEp != NULL ) 
      {
        ERR("MSD descriptor found on device %p, intf %d", (void *)pDev, i);
      }
    }  
  }
  return false;
}

USBEndpoint* UbloxLISAU200Initializer::getEp(USBDeviceConnected* pDev, int serialPortNumber, bool tx)
{
  return pDev->getEndpoint(serialPortNumber, BULK_ENDPOINT, tx?OUT:IN, 0);
}

#define UBX_SERIALCOUNT 7 

int UbloxLISAU200Initializer::getSerialPortCount()
{
  return UBX_SERIALCOUNT;
}

/*virtual*/ void UbloxLISAU200Initializer::setVidPid(uint16_t vid, uint16_t pid)
{
    if( (vid == getSerialVid() ) && ( pid == getSerialPid() ) )
    {
      m_hasSwitched = true;
      m_currentSerialIntf = 0;
      m_endpointsToFetch = UBX_SERIALCOUNT*2;
    }
    else
    {
      m_hasSwitched = false;
      m_endpointsToFetch = 1;
    }
}

/*virtual*/ bool UbloxLISAU200Initializer::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
  if( m_hasSwitched )
  {
    DBG("Interface #%d; Class:%02x; SubClass:%02x; Protocol:%02x", intf_nb, intf_class, intf_subclass, intf_protocol);
    if( intf_class == 0x0A )
    {
      if( (m_currentSerialIntf == 0) || (m_currentSerialIntf == 1) )
      {
        m_currentSerialIntf++;
        return true;
      }
      m_currentSerialIntf++;
    }
  }
  else
  {
    if( (intf_nb == 0) && (intf_class == MSD_CLASS) )
    {
      return true;
    }
  }
  return false;
}

/*virtual*/ bool UbloxLISAU200Initializer::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
  if( m_hasSwitched )
  {
    DBG("USBEndpoint on Interface #%d; Type:%d; Direction:%d", intf_nb, type, dir);
    if( (type == BULK_ENDPOINT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  else
  {
    if( (type == BULK_ENDPOINT) && (dir == OUT) && m_endpointsToFetch )
    {
      m_endpointsToFetch--;
      return true;
    }
  }
  return false;
}

/*virtual*/ WAN_DONGLE_TYPE UbloxLISAU200Initializer::getType()
{
  return WAN_DONGLE_TYPE_UBLOX_LISAU200;
}