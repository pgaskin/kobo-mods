#include <cstdlib>

#include <QMap>
#include <QPair>
#include <QString>
#include <QVariant>

#include <dlfcn.h>
#include <syslog.h>

#include <nm/dlhook.h>
#include <nm/failsafe.h>

#include "series.h"

#define NS_LOG(fmt, ...) syslog(LOG_DEBUG, "(NickelSeries) " fmt, ##__VA_ARGS__)

#ifndef NS_VERSION
#define NS_VERSION "dev"
#endif

void ns_init2();

__attribute__((constructor)) void ns_init() {
    // for if it's been loaded with LD_PRELOAD rather than as a Qt plugin
    if (strcmp(program_invocation_short_name, "nickel"))
        if (!(getenv("LIBNS_FORCE") && !strcmp(getenv("LIBNS_FORCE"), "true")))
            return;

    char *err;

    NS_LOG("version: " NS_VERSION);
    
    NS_LOG("init: creating failsafe");

    nm_failsafe_t *fs;
    if (!(fs = nm_failsafe_create(&err)) && err) {
        NS_LOG("fatal: could not create failsafe: %s", err);
        free(err);
        goto stop;
    }

    ns_init2();

    NS_LOG("init: destroying failsafe");
    nm_failsafe_destroy(fs, 1);

stop:
    NS_LOG("init: done");
    return;
}

typedef void Volume;
typedef void EPubParser;

// EPubParser::parse parses an EPUB file into a Volume.
static void (*EPubParser__parse)(EPubParser *_this, QString const& filename, Volume /*const&*/* volume);

// Volume::setAttribute sets an attribute on a Volume. For the best
// compatibility, the attribute should be retrieved from the ATTRIBUTE_*
// symbols.
static void (*Volume__setAttribute)(Volume *_this, QString /*const&*/* attribute, QVariant const& value);

// ATTRIBUTE_* specifies the database column name. Note that these attributes
// are created upon initialization of libnickel by essentially doing `QString
// *ATTRIBUTE_WHATEVER = QString::fromLatin1_helper("whatever", 8)`.
static QString *ATTRIBUTE_SERIES,              // QString
               *ATTRIBUTE_SERIES_NUMBER,       // QString
               *ATTRIBUTE_SERIES_NUMBER_FLOAT, // QString
               *ATTRIBUTE_SERIES_ID;           // QString

void ns_init2() {
    NS_LOG("init: resolving symbols");

    void *libnickel = dlopen("libnickel.so.1.0.0", RTLD_LAZY|RTLD_NODELETE);
    if (!libnickel) {
        NS_LOG("fatal: could not dlopen libnickel");
        return;
    }
    
    //libnickel 3.19.5761 * _ZN6Volume12setAttributeERK7QStringRK8QVariant
    reinterpret_cast<void*&>(Volume__setAttribute) = dlsym(RTLD_DEFAULT, "_ZN6Volume12setAttributeERK7QStringRK8QVariant");
    if (!Volume__setAttribute) {
        NS_LOG("fatal: could not dlsym Volume::setAttribute");
        return;
    }

    //libnickel 3.19.5761 * ATTRIBUTE_SERIES
    reinterpret_cast<void*&>(ATTRIBUTE_SERIES) = dlsym(RTLD_DEFAULT, "ATTRIBUTE_SERIES");
    if (!ATTRIBUTE_SERIES) {
        NS_LOG("fatal: could not dlsym ATTRIBUTE_SERIES");
        return;
    }

    //libnickel 3.19.5761 * ATTRIBUTE_SERIES_NUMBER
    reinterpret_cast<void*&>(ATTRIBUTE_SERIES_NUMBER) = dlsym(RTLD_DEFAULT, "ATTRIBUTE_SERIES_NUMBER");
    if (!ATTRIBUTE_SERIES_NUMBER) {
        NS_LOG("fatal: could not dlsym ATTRIBUTE_SERIES_NUMBER");
        return;
    }

    // idk what version this first appeared in, but I know it wasn't in 3.x
    //libnickel 4.6 * ATTRIBUTE_SERIES_NUMBER_FLOAT
    reinterpret_cast<void*&>(ATTRIBUTE_SERIES_NUMBER_FLOAT) = dlsym(RTLD_DEFAULT, "ATTRIBUTE_SERIES_NUMBER_FLOAT");
    if (!ATTRIBUTE_SERIES_NUMBER_FLOAT)
        NS_LOG("note: could not dlsym ATTRIBUTE_SERIES_NUMBER_FLOAT, not updating it");

    //libnickel 4.20.14601 * ATTRIBUTE_SERIES_ID
    reinterpret_cast<void*&>(ATTRIBUTE_SERIES_ID) = dlsym(RTLD_DEFAULT, "ATTRIBUTE_SERIES_ID");
    if (!ATTRIBUTE_SERIES_ID)
        NS_LOG("note: could not dlsym ATTRIBUTE_SERIES_ID, not updating series id for Series tab of My Books");

    void *nph = dlsym(RTLD_DEFAULT, "_ns_parse_hook");
    if (!nph) {
        NS_LOG("fatal: could not dlsym _ns_parse_hook");
        return;
    }

    // we'd rather fail at startup and trigger the failsafe than fail during import
    NS_LOG("init: testing attribute strings");

    if (ATTRIBUTE_SERIES)
        NS_LOG("ATTRIBUTE_SERIES = %s", qPrintable(*ATTRIBUTE_SERIES));

    if (ATTRIBUTE_SERIES_ID)
        NS_LOG("ATTRIBUTE_SERIES_ID = %s", qPrintable(*ATTRIBUTE_SERIES_ID));

    if (ATTRIBUTE_SERIES_NUMBER)
        NS_LOG("ATTRIBUTE_SERIES_NUMBER = %s", qPrintable(*ATTRIBUTE_SERIES_NUMBER));

    if (ATTRIBUTE_SERIES_NUMBER_FLOAT)
        NS_LOG("ATTRIBUTE_SERIES_NUMBER_FLOAT = %s", qPrintable(*ATTRIBUTE_SERIES_NUMBER));

    NS_LOG("init: hooking EPubParser::parse");

    //libnickel 3.19.5761 _ZN10EPubParser5parseERK7QStringRK6Volume
    char *err;
    reinterpret_cast<void*&>(EPubParser__parse) = nm_dlhook(libnickel, "_ZN10EPubParser5parseERK7QStringRK6Volume", nph, &err);
    if (err) {
        NS_LOG("fatal: could not hook EPubParser::parse");
        free(err);
        return;
    }

    NS_LOG("init: note: EPubParser::parse is only used for KEPUBs (SyncFileSystemCommand::parseKepub), EPUBs are handled by the Adobe plugin");
}

extern "C" void _ns_parse_hook(EPubParser *_this, QString const& filename, Volume /*const&*/* volume) {
    NS_LOG("hook: intercepting parser for ('%s', %p)", qPrintable(filename), volume);

    NS_LOG("... getting series metadata for '%s'", qPrintable(filename));
    auto r = ns_series(filename.toLatin1().constData());
    if (!r.second.isNull()) {
        NS_LOG("... error: '%s', ignoring", qPrintable(r.second));
    } else {
        QString series;
        QString index;

        for (QString id : r.first.keys()) {
            series = r.first[id].first;
            index  = r.first[id].second;
            NS_LOG("... found metadata: id='%s' series='%s' index='%s'", qPrintable(id), qPrintable(series), qPrintable(index));
        }

        if (r.first.contains("!calibre")) {
            series = r.first["!calibre"].first;
            index = r.first["!calibre"].second;
        }

        NS_LOG("... using ('%s', %s) (note that the original parser will take precedence if Kobo ever implements it)", qPrintable(series), qPrintable(index));

        if (ATTRIBUTE_SERIES) {
            NS_LOG("... setAttribute(ATTRIBUTE_SERIES, '%s')", qPrintable(series));
            Volume__setAttribute(volume, ATTRIBUTE_SERIES, series);
        }

        if (ATTRIBUTE_SERIES_NUMBER) {
            NS_LOG("... setAttribute(ATTRIBUTE_SERIES_NUMBER, '%s')", qPrintable(index));
            Volume__setAttribute(volume, ATTRIBUTE_SERIES_NUMBER, index);
        }

        if (ATTRIBUTE_SERIES_NUMBER_FLOAT) {
            NS_LOG("... setAttribute(ATTRIBUTE_SERIES_NUMBER_FLOAT, '%s')", qPrintable(index));
            Volume__setAttribute(volume, ATTRIBUTE_SERIES_NUMBER_FLOAT, index);
        }

        if (ATTRIBUTE_SERIES_ID) {
            NS_LOG("... setAttribute(ATTRIBUTE_SERIES_ID, '%s')", qPrintable(series));
            Volume__setAttribute(volume, ATTRIBUTE_SERIES_ID, series); // matches the Calibre Kobo plugin's behaviour for compatibility
        }
    }

    NS_LOG("... calling original parser");
    EPubParser__parse(_this, filename, volume);
}
