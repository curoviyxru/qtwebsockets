QT += core network

DEFINES += QTWEBSOCKETS_LIBRARY

PUBLIC_HEADERS += \
    $$PWD/qwebsocket.h \
    $$PWD/qwebsocketserver.h \
    $$PWD/qwebsocket4.h \

PRIVATE_HEADERS += \
    $$PWD/qwebsockets_global.h \
    $$PWD/qwebsocketprotocol.h \
    $$PWD/qwebsocketcorsauthenticator.h \
    $$PWD/qwebsocket_p.h \
    $$PWD/qwebsocketserver_p.h \
    $$PWD/qwebsocketprotocol_p.h \
    $$PWD/qwebsockethandshakerequest_p.h \
    $$PWD/qwebsockethandshakeresponse_p.h \
    $$PWD/qwebsocketdataprocessor_p.h \
    $$PWD/qwebsocketcorsauthenticator_p.h \
    $$PWD/qwebsocketframe_p.h

HEADERS +=  $$PUBLIC_HEADERS \
            $$PRIVATE_HEADERS

SOURCES += \
    $$PWD/qwebsocket.cpp \
    $$PWD/qwebsocket_p.cpp \
    $$PWD/qwebsocketserver.cpp \
    $$PWD/qwebsocketserver_p.cpp \
    $$PWD/qwebsocketprotocol.cpp \
    $$PWD/qwebsockethandshakerequest.cpp \
    $$PWD/qwebsockethandshakeresponse.cpp \
    $$PWD/qwebsocketdataprocessor.cpp \
    $$PWD/qwebsocketcorsauthenticator.cpp \
    $$PWD/qwebsocketframe.cpp

SOURCES += $$PWD/qsslserver.cpp
HEADERS += $$PWD/qsslserver_p.h
