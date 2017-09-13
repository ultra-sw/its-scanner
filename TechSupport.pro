#-------------------------------------------------
#
# Project created by QtCreator 2017-02-09T14:38:18
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TechSupport
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += QUAZIP_STATIC SMTP_STATIC

SOURCES += main.cpp\
    MainWindow.cpp \
    FlowLayout.cpp \
    quazip/JlCompress.cpp \
    quazip/qioapi.cpp \
    quazip/quaadler32.cpp \
    quazip/quacrc32.cpp \
    quazip/quagzipfile.cpp \
    quazip/quaziodevice.cpp \
    quazip/quazip.cpp \
    quazip/quazipdir.cpp \
    quazip/quazipfile.cpp \
    quazip/quazipfileinfo.cpp \
    quazip/quazipnewinfo.cpp \
    quazip/unzip.c \
    quazip/zip.c \
    qsmtp/emailaddress.cpp \
    qsmtp/mimeattachment.cpp \
    qsmtp/mimecontentformatter.cpp \
    qsmtp/mimefile.cpp \
    qsmtp/mimehtml.cpp \
    qsmtp/mimeinlinefile.cpp \
    qsmtp/mimemessage.cpp \
    qsmtp/mimemultipart.cpp \
    qsmtp/mimepart.cpp \
    qsmtp/mimetext.cpp \
    qsmtp/quotedprintable.cpp \
    qsmtp/smtpclient.cpp \
    FileDownloader.cpp \
    SupportRequester.cpp \
    ScreenshotLabel.cpp

HEADERS  += MainWindow.h \
    FlowLayout.h \
    quazip/crypt.h \
    quazip/ioapi.h \
    quazip/JlCompress.h \
    quazip/quaadler32.h \
    quazip/quachecksum32.h \
    quazip/quacrc32.h \
    quazip/quagzipfile.h \
    quazip/quaziodevice.h \
    quazip/quazip.h \
    quazip/quazip_global.h \
    quazip/quazipdir.h \
    quazip/quazipfile.h \
    quazip/quazipfileinfo.h \
    quazip/quazipnewinfo.h \
    quazip/unzip.h \
    quazip/zip.h \
    qsmtp/emailaddress.h \
    qsmtp/mimeattachment.h \
    qsmtp/mimecontentformatter.h \
    qsmtp/mimefile.h \
    qsmtp/mimehtml.h \
    qsmtp/mimeinlinefile.h \
    qsmtp/mimemessage.h \
    qsmtp/mimemultipart.h \
    qsmtp/mimepart.h \
    qsmtp/mimetext.h \
    qsmtp/quotedprintable.h \
    qsmtp/smtpclient.h \
    qsmtp/smtpexports.h \
    qsmtp/SmtpMime \
    FileDownloader.h \
    SupportRequester.h \
    ScreenshotLabel.h

FORMS    += MainWindow.ui

RESOURCES += \
    TechSupport.qrc

RC_FILE = TechSupport.rc
