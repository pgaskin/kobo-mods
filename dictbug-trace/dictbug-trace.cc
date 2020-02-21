#include <QApplication>
#include <QDebug>
#include <QString>
#include <QUrl>

#include <stdexcept>
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

constructor static void init() {
    SYM(DictionaryWebview_setHtml_orig, "_ZN17DictionaryWebview7setHtmlERK7QStringRK4QUrl");
}

extern "C" void _ZN17DictionaryWebview7setHtmlERK7QStringRK4QUrl(void* _this, QString const& path, QUrl const& url) {
    LOG("DictionaryWebview::setHtml(%s, %s)", qPrintable(path), qPrintable(url.toString(QUrl::None)));
    DictionaryWebview_setHtml_orig(_this, path, url);
    return;
}
