/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWebSockets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwebsockethandshakeresponse_p.h"
#include "qwebsockethandshakerequest_p.h"
#include "qwebsocketprotocol.h"

#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>
#include <QtCore/QCryptographicHash>
#include <QtCore/QSet>
#include <QtCore/QList>
#include <QtCore/QStringBuilder>   //for more efficient string concatenation

#include <functional>   //for std::greater
#include <algorithm>    //for std::sort

#if QT_VERSION <= QT_VERSION_CHECK(5, 0, 2)
    #include "qwebsocket4.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \internal
 */
QWebSocketHandshakeResponse::QWebSocketHandshakeResponse(
        const QWebSocketHandshakeRequest &request,
        const QString &serverName,
        bool isOriginAllowed,
        const QList<QWebSocketProtocol::Version> &supportedVersions,
        const QList<QString> &supportedProtocols,
        const QList<QString> &supportedExtensions) :
    m_isValid(false),
    m_canUpgrade(false),
    m_response(),
    m_acceptedProtocol(),
    m_acceptedExtension(),
    m_acceptedVersion(QWebSocketProtocol::V_Unknow),
    m_error(QWebSocketProtocol::CC_NORMAL),
    m_errorString()
{
    m_response = getHandshakeResponse(request, serverName,
                                      isOriginAllowed, supportedVersions,
                                      supportedProtocols, supportedExtensions);
    m_isValid = true;
}

/*!
    \internal
 */
QWebSocketHandshakeResponse::~QWebSocketHandshakeResponse()
{
}

/*!
    \internal
 */
bool QWebSocketHandshakeResponse::isValid() const
{
    return m_isValid;
}

/*!
    \internal
 */
bool QWebSocketHandshakeResponse::canUpgrade() const
{
    return m_isValid && m_canUpgrade;
}

/*!
    \internal
 */
QString QWebSocketHandshakeResponse::acceptedProtocol() const
{
    return m_acceptedProtocol;
}

/*!
    \internal
 */
QString QWebSocketHandshakeResponse::calculateAcceptKey(const QString &key) const
{
    //the UID comes from RFC6455
    const QString tmpKey = key % QStringLiteral("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    const QByteArray hash = QCryptographicHash::hash(tmpKey.toLatin1(), QCryptographicHash::Sha1);
    return QString::fromLatin1(hash.toBase64());
}

/*!
    \internal
 */
QString QWebSocketHandshakeResponse::getHandshakeResponse(
        const QWebSocketHandshakeRequest &request,
        const QString &serverName,
        bool isOriginAllowed,
        const QList<QWebSocketProtocol::Version> &supportedVersions,
        const QList<QString> &supportedProtocols,
        const QList<QString> &supportedExtensions)
{
    QStringList response;
    m_canUpgrade = false;

    if (!isOriginAllowed) {
        if (!m_canUpgrade) {
            m_error = QWebSocketProtocol::CC_POLICY_VIOLATED;
            m_errorString = tr("Access forbidden.");
            response << QStringLiteral("HTTP/1.1 403 Access Forbidden");
        }
    } else {
        if (request.isValid()) {
            const QString acceptKey = calculateAcceptKey(request.key());
            const QList<QString> matchingProtocols =
                    supportedProtocols.toSet().intersect(request.protocols().toSet()).toList();
            const QList<QString> matchingExtensions =
                    supportedExtensions.toSet().intersect(request.extensions().toSet()).toList();
            QList<QWebSocketProtocol::Version> matchingVersions =
                    request.versions().toSet().intersect(supportedVersions.toSet()).toList();
            std::sort(matchingVersions.begin(), matchingVersions.end(),
                      std::greater<QWebSocketProtocol::Version>());    //sort in descending order

            if (Q_UNLIKELY(matchingVersions.isEmpty())) {
                m_error = QWebSocketProtocol::CC_PROTOCOL_ERROR;
                m_errorString = tr("Unsupported version requested.");
                m_canUpgrade = false;
            } else {
                response << QStringLiteral("HTTP/1.1 101 Switching Protocols") <<
                            QStringLiteral("Upgrade: websocket") <<
                            QStringLiteral("Connection: Upgrade") <<
                            QStringLiteral("Sec-WebSocket-Accept: ") % acceptKey;
                if (!matchingProtocols.isEmpty()) {
                    m_acceptedProtocol = matchingProtocols.first();
                    response << QStringLiteral("Sec-WebSocket-Protocol: ") % m_acceptedProtocol;
                }
                if (!matchingExtensions.isEmpty()) {
                    m_acceptedExtension = matchingExtensions.first();
                    response << QStringLiteral("Sec-WebSocket-Extensions: ") % m_acceptedExtension;
                }
                QString origin = request.origin().trimmed();
                if (origin.isEmpty())
                    origin = QStringLiteral("*");
                response << QStringLiteral("Server: ") % serverName                      <<
                            QStringLiteral("Access-Control-Allow-Credentials: false")    <<
                            QStringLiteral("Access-Control-Allow-Methods: GET")          <<
                            QStringLiteral("Access-Control-Allow-Headers: content-type") <<
                            QStringLiteral("Access-Control-Allow-Origin: ") % origin     <<
                            QStringLiteral("Date: ") %
                                QDateTime::currentDateTimeUtc()
                                    .toString(QStringLiteral("ddd, dd MMM yyyy hh:mm:ss 'GMT'"));

                m_acceptedVersion = QWebSocketProtocol::currentVersion();
                m_canUpgrade = true;
            }
        } else {
            m_error = QWebSocketProtocol::CC_PROTOCOL_ERROR;
            m_errorString = tr("Bad handshake request received.");
            m_canUpgrade = false;
        }
        if (Q_UNLIKELY(!m_canUpgrade)) {
            response << QStringLiteral("HTTP/1.1 400 Bad Request");
            QStringList versions;
            Q_FOREACH (QWebSocketProtocol::Version version, supportedVersions)
                versions << QString::number(static_cast<int>(version));
            response << QStringLiteral("Sec-WebSocket-Version: ")
                                % versions.join(QStringLiteral(", "));
        }
    }
    response << QStringLiteral("\r\n");    //append empty line at end of header
    return response.join(QStringLiteral("\r\n"));
}

/*!
    \internal
 */
QTextStream &QWebSocketHandshakeResponse::writeToStream(QTextStream &textStream) const
{
    if (Q_LIKELY(!m_response.isEmpty()))
        textStream << m_response.toLatin1().constData();
    else
        textStream.setStatus(QTextStream::ReadCorruptData);
    return textStream;
}

/*!
    \internal
 */
QTextStream &operator <<(QTextStream &stream, const QWebSocketHandshakeResponse &response)
{
    return response.writeToStream(stream);
}

/*!
    \internal
 */
QWebSocketProtocol::Version QWebSocketHandshakeResponse::acceptedVersion() const
{
    return m_acceptedVersion;
}

/*!
    \internal
 */
QWebSocketProtocol::CloseCode QWebSocketHandshakeResponse::error() const
{
    return m_error;
}

/*!
    \internal
 */
QString QWebSocketHandshakeResponse::errorString() const
{
    return m_errorString;
}

/*!
    \internal
 */
QString QWebSocketHandshakeResponse::acceptedExtension() const
{
    return m_acceptedExtension;
}

QT_END_NAMESPACE