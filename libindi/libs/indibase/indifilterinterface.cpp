/*
    Filter Interface
    Copyright (C) 2011 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "indifilterinterface.h"
#include <cstring>
#include "indilogger.h"

// Deprecated
INDI::FilterInterface::FilterInterface()
{
    FilterNameTP = new ITextVectorProperty;
    FilterNameT  = nullptr;
}

INDI::FilterInterface::FilterInterface(DefaultDevice *defaultDevice) : m_defaultDevice(defaultDevice)
{
    FilterNameTP = new ITextVectorProperty;
    FilterNameT  = nullptr;
}

INDI::FilterInterface::~FilterInterface()
{
    delete FilterNameTP;
}

void INDI::FilterInterface::initProperties(const char *groupName)
{
    IUFillNumber(&FilterSlotN[0], "FILTER_SLOT_VALUE", "Filter", "%3.0f", 1.0, 12.0, 1.0, 1.0);
    IUFillNumberVector(&FilterSlotNP, FilterSlotN, 1, m_defaultDevice->getDeviceName(), "FILTER_SLOT", "Filter Slot", groupName, IP_RW, 60,
                       IPS_IDLE);
}

bool INDI::FilterInterface::updateProperties()
{
    if (m_defaultDevice->isConnected())
    {
        // Define the Filter Slot and name properties
        m_defaultDevice->defineNumber(&FilterSlotNP);
        if (FilterNameT != nullptr)
            m_defaultDevice->defineText(FilterNameTP);
    }
    else
    {
        m_defaultDevice->deleteProperty(FilterSlotNP.name);
        m_defaultDevice->deleteProperty(FilterNameTP->name);
    }

    return true;
}

bool INDI::FilterInterface::processNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    INDI_UNUSED(n);

    if (dev && !strcmp(dev, m_defaultDevice->getDeviceName()) && !strcmp(name, FilterSlotNP.name))
    {
        TargetFilter = values[0];

        INumber *np = IUFindNumber(&FilterSlotNP, names[0]);

        if (!np)
        {
            FilterSlotNP.s = IPS_ALERT;
            DEBUGFDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_ERROR, "Unknown error. %s is not a member of %s property.", names[0], FilterSlotNP.name);
            IDSetNumber(&FilterSlotNP, nullptr);
            return false;
        }

        if (TargetFilter < FilterSlotN[0].min || TargetFilter > FilterSlotN[0].max)
        {
            FilterSlotNP.s = IPS_ALERT;
            DEBUGFDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_ERROR, "Error: valid range of filter is from %g to %g", FilterSlotN[0].min, FilterSlotN[0].max);
            IDSetNumber(&FilterSlotNP, nullptr);
            return false;
        }

        FilterSlotNP.s = IPS_BUSY;
        DEBUGFDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_SESSION, "Setting current filter to slot %d", TargetFilter);

        if (SelectFilter(TargetFilter) == false)
        {
            FilterSlotNP.s = IPS_ALERT;
        }

        IDSetNumber(&FilterSlotNP, nullptr);
        return true;
    }

    return false;
}

bool INDI::FilterInterface::processText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev && !strcmp(dev, m_defaultDevice->getDeviceName()) && FilterNameTP && !strcmp(name, FilterNameTP->name))
    {
        FilterNameTP->s = IPS_OK;
        IUUpdateText(FilterNameTP, texts, names, n);

        if (SetFilterNames() == true)
        {
            IDSetText(FilterNameTP, nullptr);
            return true;
        }
        else
        {
            FilterNameTP->s = IPS_ALERT;
            DEBUGDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_ERROR, "Error updating names of filters.");
            IDSetText(FilterNameTP, nullptr);
            return false;
        }
    }

    return false;
}

// Deprecated
void INDI::FilterInterface::initFilterProperties(const char *deviceName, const char *groupName)
{
    IUFillNumber(&FilterSlotN[0], "FILTER_SLOT_VALUE", "Filter", "%3.0f", 1.0, 12.0, 1.0, 1.0);
    IUFillNumberVector(&FilterSlotNP, FilterSlotN, 1, deviceName, "FILTER_SLOT", "Filter Slot", groupName, IP_RW, 60,
                       IPS_IDLE);
}

// Deprecated
void INDI::FilterInterface::processFilterSlot(const char *deviceName, double values[], char *names[])
{
    TargetFilter = values[0];

    INumber *np = IUFindNumber(&FilterSlotNP, names[0]);

    if (!np)
    {
        FilterSlotNP.s = IPS_ALERT;
        DEBUGFDEVICE(deviceName, Logger::DBG_ERROR, "Unknown error. %s is not a member of %s property.", names[0],
                     FilterSlotNP.name);
        IDSetNumber(&FilterSlotNP, nullptr);
        return;
    }

    if (TargetFilter < FilterSlotN[0].min || TargetFilter > FilterSlotN[0].max)
    {
        FilterSlotNP.s = IPS_ALERT;
        DEBUGFDEVICE(deviceName, Logger::DBG_ERROR, "Error: valid range of filter is from %g to %g", FilterSlotN[0].min,
                     FilterSlotN[0].max);
        IDSetNumber(&FilterSlotNP, nullptr);
        return;
    }

    FilterSlotNP.s = IPS_BUSY;
    DEBUGFDEVICE(deviceName, Logger::DBG_SESSION, "Setting current filter to slot %d", TargetFilter);

    if (SelectFilter(TargetFilter) == false)
    {
        FilterSlotNP.s = IPS_ALERT;
    }

    IDSetNumber(&FilterSlotNP, nullptr);
    return;
}

// Deprecated
void INDI::FilterInterface::processFilterName(const char *deviceName, char *texts[], char *names[], int n)
{
    FilterNameTP->s = IPS_OK;
    IUUpdateText(FilterNameTP, texts, names, n);

    if (SetFilterNames() == true)
        IDSetText(FilterNameTP, nullptr);
    else
    {
        FilterNameTP->s = IPS_ALERT;
        DEBUGDEVICE(deviceName, Logger::DBG_ERROR, "Error updating names of filters.");
        IDSetText(FilterNameTP, nullptr);
    }
}

void INDI::FilterInterface::SelectFilterDone(int f)
{
    //  The hardware has finished changing
    //  filters
    FilterSlotN[0].value = f;
    FilterSlotNP.s       = IPS_OK;
    // Tell the clients we are done, and
    //  filter is now useable
    IDSetNumber(&FilterSlotNP, nullptr);
}
