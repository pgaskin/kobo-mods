#include <QApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>

#include <string>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <syslog.h>

#define LOG(format, ...) syslog(LOG_DEBUG, "(dictbug-trace) " format, __VA_ARGS__)
#define SYM(var, sym) reinterpret_cast<void*&>(var) = dlsym(RTLD_NEXT, sym)
#define constructor __attribute__((constructor))

static void(*DictionaryWebview_setHtml_orig)(void*, QString const&, QUrl const&);
static QNetworkReply*(*QNetworkAccessManager_get_orig)(void*, QNetworkRequest const&);
static void*(*DictionaryParser_getFile_orig)(void*, void*, QString const&);

constructor static void init() {
    SYM(DictionaryWebview_setHtml_orig, "_ZN17DictionaryWebview7setHtmlERK7QStringRK4QUrl");
    SYM(QNetworkAccessManager_get_orig, "_ZN21QNetworkAccessManager3getERK15QNetworkRequest");
    SYM(DictionaryParser_getFile_orig, "_ZN16DictionaryParser7getFileEP7ZipFileRK7QString");
}

extern "C" void _ZN17DictionaryWebview7setHtmlERK7QStringRK4QUrl(void* _this, QString const& path, QUrl const& url) {
    LOG("DictionaryWebview::setHtml(`%s`,`%s`)", qPrintable(path), qPrintable(url.toString(QUrl::None)));
    DictionaryWebview_setHtml_orig(_this, path, url);
    return;
}

extern "C" QNetworkReply* _ZN21QNetworkAccessManager3getERK15QNetworkRequest(void* _this, QNetworkRequest const& request) {
    LOG("QNetworkAccessManager::get(QNetworkRequest.url:`%s`)", qPrintable(request.url().toString(QUrl::None)));
    return QNetworkAccessManager_get_orig(_this, request);
}

extern "C" void* _ZN16DictionaryParser7getFileEP7ZipFileRK7QString(void* _this, void* zip, QString const& filename) {
    LOG("DictionaryParser::getFile(zip:*%p, `%s`)", zip, qPrintable(filename));
    return DictionaryParser_getFile_orig(_this, zip, filename);
}
