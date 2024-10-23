CONFIG += qt 
CONFIG += sql

INCLUDEPATH += \
    $$PWD/Headers

HEADERS += \
    $$PWD/Headers/Common/common.h \
    $$PWD/Headers/Common/tdbloger.h \
    $$PWD/Headers/Common/thttpquery.h \
    $$PWD/Headers/Common/httpsslquery.h \
    $$PWD/Headers/Common/regcheck.h

SOURCES += \
    $$PWD/Src/common.cpp \
    $$PWD/Src/tdbloger.cpp \
    $$PWD/Src/thttpquery.cpp \
    $$PWD/Src/httpsslquery.cpp \
    $$PWD/Src/regcheck.cpp

