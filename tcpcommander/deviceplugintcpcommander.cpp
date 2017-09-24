/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "deviceplugintcpcommander.h"
#include "plugininfo.h"


DevicePluginTcpCommander::DevicePluginTcpCommander()
{
}

DeviceManager::HardwareResources DevicePluginTcpCommander::requiredHardware() const
{
    return DeviceManager::HardwareResourceNone;
}


DeviceManager::DeviceSetupStatus DevicePluginTcpCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == tcpOutputDeviceClassId) {
        QTcpSocket *tcpSocket = new QTcpSocket(this);
        m_tcpSockets.insert(tcpSocket, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == tcpInputDeviceClassId) {
        int port = device->paramValue(portParamTypeId).toInt();
        TcpServer *tcpServer = new TcpServer(port, this);
        //TODO Connect TCP Server request received
        if (tcpServer->isValid()) {
            m_tcpServer.insert(tcpServer, device);
            connect(tcpServer, &TcpServer::connected, this, &DevicePluginTcpCommander::onTcpServerConnected);
            connect(tcpServer, &TcpServer::disconnected, this, &DevicePluginTcpCommander::onTcpServerDisconnected);
            return DeviceManager::DeviceSetupStatusSuccess;
        } else {
            qDebug(dcTCPCommander()) << "Could not open TCP Server";
        }
    }
    return DeviceManager::DeviceSetupStatusFailure;
}


DeviceManager::DeviceError DevicePluginTcpCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == tcpOutputDeviceClassId) {

        if (action.actionTypeId() == outputDataActionTypeId) {
            int port = device->paramValue(portParamTypeId).toInt();
            QHostAddress address= QHostAddress(device->paramValue(ipv4addressParamTypeId).toString());
            QTcpSocket *tcpSocket = m_tcpSockets.key(device);
            QByteArray data = device->paramValue(outputDataAreaParamTypeId).toByteArray();
            tcpSocket->connectToHost(address, port);
            tcpSocket->write(data);
            tcpSocket->close();
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginTcpCommander::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == tcpOutputDeviceClassId){
        m_tcpSockets.remove(m_tcpSockets.key(device));
    }else if(device->deviceClassId() == tcpInputDeviceClassId){
        TcpServer *tcpServer = m_tcpServer.key(device);
        m_tcpServer.remove(tcpServer);
        tcpServer->deleteLater();
    }
}


void DevicePluginTcpCommander::onTcpServerConnected()
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Tcp Server Client connected" ;
    device->setStateValue(connectedStateTypeId, true);
    connect(tcpServer, &TcpServer::textMessageReceived, this, &DevicePluginTcpCommander::onTcpServerTextMessageReceived);
    //send signal device Setup was successfull
}


void DevicePluginTcpCommander::onTcpServerDisconnected()
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Tcp Server Client disconnected" ;
    device->setStateValue(connectedStateTypeId, false);
}

void DevicePluginTcpCommander::onTcpServerTextMessageReceived(QByteArray data)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Message received" << data;
    device->setStateValue(responseStateTypeId, data); //TODO change wording

    if (device->paramValue(comparisionParamTypeId).toString() == "Is exactly") {
        qDebug(dcTCPCommander()) << "is exacly";
        if (data == device->paramValue(inputDataParamTypeId)) {
            qDebug(dcTCPCommander()) << "comparison successful";
            emitEvent(Event(commandReceivedEventTypeId, device->id()));
        }

    } else if (device->paramValue(comparisionParamTypeId).toString() == "Contains") {
        if (data.contains(device->paramValue(inputDataParamTypeId).toByteArray())) {
            emitEvent(Event(commandReceivedEventTypeId, device->id()));
        }

    } else if (device->paramValue(comparisionParamTypeId) == "Contains not") {
        if (!data.contains(device->paramValue(inputDataParamTypeId).toByteArray()))
            emitEvent(Event(commandReceivedEventTypeId, device->id()));

    } else if (device->paramValue(comparisionParamTypeId) == "Starts with") {
        if (data.startsWith(device->paramValue(inputDataParamTypeId).toByteArray()))
            emitEvent(Event(commandReceivedEventTypeId, device->id()));

    } else if (device->paramValue(comparisionParamTypeId) == "Ends with") {
        if (data.endsWith(device->paramValue(inputDataParamTypeId).toByteArray()))
            emitEvent(Event(commandReceivedEventTypeId, device->id()));
    }
}