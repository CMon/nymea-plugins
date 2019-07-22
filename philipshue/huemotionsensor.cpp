/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "huemotionsensor.h"
#include "extern-plugininfo.h"

HueMotionSensor::HueMotionSensor(QObject *parent) :
    HueDevice(parent)
{
    m_timeout.setInterval(10000);
    connect(&m_timeout, &QTimer::timeout, this, [this](){
        if (m_presence) {
            qCDebug(dcPhilipsHue) << "Motion sensor timeout reached" << m_timeout.interval();
            m_presence = false;
            emit presenceChanged(m_presence);
        }
    });
}

void HueMotionSensor::setTimeout(int timeout)
{
    // The sensor keeps emitting presence = true for 10 secs, let's subtract that time from the timeout to compensate
    m_timeout.setInterval((timeout - 9)* 1000);
}

int HueMotionSensor::temperatureSensorId() const
{
    return m_temperatureSensorId;
}

void HueMotionSensor::setTemperatureSensorId(int sensorId)
{
    m_temperatureSensorId = sensorId;
}

QString HueMotionSensor::temperatureSensorUuid() const
{
    return m_temperatureSensorUuid;
}

void HueMotionSensor::setTemperatureSensorUuid(const QString &temperatureSensorUuid)
{
    m_temperatureSensorUuid = temperatureSensorUuid;
}

int HueMotionSensor::presenceSensorId() const
{
    return m_presenceSensorId;
}

void HueMotionSensor::setPresenceSensorId(int sensorId)
{
    m_presenceSensorId = sensorId;
}

QString HueMotionSensor::presenceSensorUuid() const
{
    return m_presenceSensorUuid;
}

void HueMotionSensor::setPresenceSensorUuid(const QString &presenceSensorUuid)
{
    m_presenceSensorUuid = presenceSensorUuid;
}

int HueMotionSensor::lightSensorId() const
{
    return m_lightSensorId;
}

void HueMotionSensor::setLightSensorId(int sensorId)
{
    m_lightSensorId = sensorId;
}

QString HueMotionSensor::lightSensorUuid() const
{
    return m_lightSensorUuid;
}

void HueMotionSensor::setLightSensorUuid(const QString &lightSensorUuid)
{
    m_lightSensorUuid = lightSensorUuid;
}

double HueMotionSensor::temperature() const
{
    return m_temperature;
}

double HueMotionSensor::lightIntensity() const
{
    return m_lightIntensity;
}

bool HueMotionSensor::present() const
{
    return m_presence;
}

int HueMotionSensor::batteryLevel() const
{
    return m_batteryLevel;
}

void HueMotionSensor::updateStates(const QVariantMap &sensorMap)
{
    //qCDebug(dcPhilipsHue()) << "Outdoor sensor: Process sensor map" << qUtf8Printable(QJsonDocument::fromVariant(sensorMap).toJson(QJsonDocument::Indented));

    // Config
    QVariantMap configMap = sensorMap.value("config").toMap();
    if (configMap.contains("reachable")) {
        setReachable(configMap.value("reachable", false).toBool());
    }

    if (configMap.contains("battery")) {
        int batteryLevel = configMap.value("battery", 0).toInt();
        if (m_batteryLevel != batteryLevel) {
            m_batteryLevel = batteryLevel;
            emit batteryLevelChanged(m_batteryLevel);
        }
    }

    // If temperature sensor
    QVariantMap stateMap = sensorMap.value("state").toMap();
    if (sensorMap.value("uniqueid").toString() == m_temperatureSensorUuid) {
        double temperature = stateMap.value("temperature", 0).toInt() / 100.0;
        if (m_temperature != temperature) {
            m_temperature = temperature;
            emit temperatureChanged(m_temperature);
            qCDebug(dcPhilipsHue) << "Motion sensor temperature changed" << m_temperature;
        }
    }

    // If presence sensor
    if (sensorMap.value("uniqueid").toString() == m_presenceSensorUuid) {
        bool presence = stateMap.value("presence", false).toBool();
        if (presence) {
            if (!m_presence) {
                m_presence = true;
                emit presenceChanged(m_presence);
                qCDebug(dcPhilipsHue) << "Motion sensor presence changed" << presence;
            }
            qCDebug(dcPhilipsHue) << "Motion sensor restarting timeout" << m_timeout.interval();
            m_timeout.start();
        }
    }

    // If light sensor
    if (sensorMap.value("uniqueid").toString() == m_lightSensorUuid) {
        int lightIntensity = stateMap.value("lightlevel", 0).toInt();
        if (m_lightIntensity != lightIntensity) {
            m_lightIntensity = lightIntensity;
            emit lightIntensityChanged(m_lightIntensity);
            qCDebug(dcPhilipsHue) << "Outdoor sensor light intensity changed" << m_lightIntensity;
        }
    }
}

bool HueMotionSensor::isValid()
{
    return !m_temperatureSensorUuid.isEmpty() && !m_presenceSensorUuid.isEmpty() && !m_lightSensorUuid.isEmpty();
}

bool HueMotionSensor::hasSensor(int sensorId)
{
    return m_temperatureSensorId == sensorId || m_presenceSensorId == sensorId || m_lightSensorId == sensorId;
}

bool HueMotionSensor::hasSensor(const QString &sensorUuid)
{
    return m_temperatureSensorUuid == sensorUuid || m_presenceSensorUuid == sensorUuid || m_lightSensorUuid == sensorUuid;
}