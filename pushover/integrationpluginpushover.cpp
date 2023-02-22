/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginpushover.h"

#include <QJsonDocument>

#include "network/networkaccessmanager.h"

#include "plugininfo.h"
#include "qjsonarray.h"

IntegrationPluginPushover::IntegrationPluginPushover(QObject* parent)
    : IntegrationPlugin (parent)
{
}

void IntegrationPluginPushover::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcPushover()) << "Setting up Pushover" << thing->name() << thing->id().toString();
    pluginStorage()->beginGroup(info->thing()->id().toString());
    QString applicationToken = pluginStorage()->value("applicationToken").toString();
    QString userToken = pluginStorage()->value("userToken").toString();
    pluginStorage()->endGroup();

    QNetworkRequest checkUserExistRequest(QUrl("https://api.pushover.net/1/users/validate.json"));
    checkUserExistRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject reqObject;
    reqObject["token"] = applicationToken;
    reqObject["user"]  = userToken;
    QNetworkReply *reply = hardwareManager()->networkManager()->post(checkUserExistRequest, QJsonDocument(reqObject).toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, this, &IntegrationPluginPushover::handlePushoverReply);
}

void IntegrationPluginPushover::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcPushover()) << "Executing action" << action.actionTypeId() << "for" << thing->name() << thing->id().toString();
    pluginStorage()->beginGroup(info->thing()->id().toString());
    QString applicationToken = pluginStorage()->value("applicationToken").toString();
    QString userToken = pluginStorage()->value("userToken").toString();
    pluginStorage()->endGroup();


    if(userToken.isEmpty()) {
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The provided user token is not valid."));
        return;
    }
    if(applicationToken.isEmpty()) {
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The provided application token is not valid."));
        return;
    }

    QNetworkRequest checkUserExistRequest(QUrl("https://api.pushover.net/1/messages.json"));
    checkUserExistRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject reqObject;
    reqObject["token"]   = applicationToken;
    reqObject["user"]    = userToken;
    reqObject["title"]   = action.param(pushNotificationNotifyActionTitleParamTypeId).value().toString();
    reqObject["message"] = action.param(pushNotificationNotifyActionBodyParamTypeId).value().toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->post(checkUserExistRequest, QJsonDocument(reqObject).toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, [&, info]{
        handlePushoverReply(reply, info);
    });

}

void IntegrationPluginPushover::handlePushoverReply(QNetworkReply *reply, ThingSetupInfo *info)
{
    QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();

    if(root.contains("status") && root["status"].toInt() == 0) {
        if(root.contains("user") && root["user"].toString() == "invalid") {
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The provided user token is not valid."));
            return;
        }
        if(root.contains("token") && root["token"].toString() == "invalid") {
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The provided application token is not valid."));
            return;
        }
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP(QString("Unknown error occured: %1").arg(root["errors"].toArray()[0].toString())));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dcPushover()) << "Error fetching user/app profile:" << reply->errorString() << reply->error();
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to Pushover."));
        return;
    }
    qCDebug(dcPushover()) << "Pushover" << info->thing()->name() << info->thing()->id().toString() << "setup complete";
    info->finish(Thing::ThingErrorNoError);
}
