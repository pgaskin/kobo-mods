#include <QMap>
#include <QPair>
#include <QString>
#include <QVariant>

#include <cstdlib>

#include <dlfcn.h>
#include <syslog.h>
#include <unistd.h>

#include <NickelHook.h>

#include "series.h"

typedef void EPubParser;
typedef void Content;
typedef Content Volume;

typedef void (*Volume__setter)(Volume *_this, QVariant const& value); // the value is almost always a QString

static Volume__setter
    Volume__setSeriesName = nullptr,
    Volume__setSeriesNumber = nullptr,
    Volume__setSeriesNumberFloat = nullptr,
    Volume__setSeriesId = nullptr;

static void (*EPubParser__parse)(EPubParser *_this, QString const& filename, Volume /*const&*/* volume);
static void (*Content__setId)(Content *_this, QVariant const& value);

static struct nh_info NickelSeries = (struct nh_info){
    .name           = "NickelSeries",
    .desc           = "Adds built-in EPUB/KEPUB series metadata support.",
    .uninstall_flag = "/mnt/onboard/ns_uninstall",
};

static struct nh_hook NickelSeriesHook[] = {
    {.sym = "_ZN10EPubParser5parseERK7QStringRK6Volume", .sym_new = "_ns_kepub_parse_hook", .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(EPubParser__parse), .desc = "series metadata for KEPUBs", .optional = true},
    {.sym = "_ZN7Content5setIdERK8QVariant",             .sym_new = "_ns_epub_cid_hook",    .lib = "libadobe.so",        .out = nh_symoutptr(Content__setId),    .desc = "series metadata for EPUBs",  .optional = true},
    {0},
};

static struct nh_dlsym NickelSeriesDlsym[] = {
    {.name = "_ZN6Volume13setSeriesNameERK8QVariant",        .out = nh_symoutptr(Volume__setSeriesName)},
    {.name = "_ZN6Volume15setSeriesNumberERK8QVariant",      .out = nh_symoutptr(Volume__setSeriesNumber)},
    {.name = "_ZN6Volume20setSeriesNumberFloatERK8QVariant", .out = nh_symoutptr(Volume__setSeriesNumberFloat), .desc = "series number on newer firmware versions", .optional = true},
    {.name = "_ZN6Volume11setSeriesIdERK8QVariant",          .out = nh_symoutptr(Volume__setSeriesId),          .desc = "series tab on 4.20.14601+",                .optional = true},
    {0},
};

NickelHook(
    .init  = nullptr,
    .info  = &NickelSeries,
    .hook  = NickelSeriesHook,
    .dlsym = NickelSeriesDlsym,
)

static void ns_update_series(Volume *v, QString const& filename) {
    nh_log("hook: updating series metadata for '%s'", qPrintable(filename));

    nh_log("... getting series metadata for '%s'", qPrintable(filename));
    auto r = ns_series(filename.toLatin1().constData());
    if (!r.second.isNull()) {
        nh_log("... error: '%s', ignoring", qPrintable(r.second));
    } else {
        QString series;
        QString index;

        for (QString id : r.first.keys()) {
            series = r.first[id].first;
            index  = r.first[id].second;
            nh_log("... found metadata: id='%s' series='%s' index='%s'", qPrintable(id), qPrintable(series), qPrintable(index));
        }

        if (r.first.contains("!calibre")) {
            series = r.first["!calibre"].first;
            index = r.first["!calibre"].second;
        }

        nh_log("... using ('%s', %s)", qPrintable(series), qPrintable(index));

        bool ok;
        double d = QVariant(index).toDouble(&ok);
        if (ok) {
            nh_log("... simplified series index '%s' to '%s'", qPrintable(index), qPrintable(QString::number(d)));
            index = QString::number(d);
        }

        nh_log("... Volume::setSeriesName('%s')", qPrintable(series));
        Volume__setSeriesName(v, series);

        nh_log("... Volume::setSeriesNumber('%s')", qPrintable(index));
        Volume__setSeriesNumber(v, index);

        if (Volume__setSeriesNumberFloat) {
            nh_log("... Volume::setSeriesNumberFloat('%s')", qPrintable(index));
            Volume__setSeriesNumberFloat(v, index);
        }

        if (Volume__setSeriesId) {
            nh_log("... Volume::setSeriesId('%s')", qPrintable(series));
            Volume__setSeriesId(v, series); // matches the Calibre Kobo plugin's behaviour for compatibility
        }
    }
}

extern "C" __attribute__((visibility("default"))) void _ns_kepub_parse_hook(EPubParser *_this, QString const& filename, Volume /*const&*/* volume) {
    nh_log("hook: intercepting KEPUB EPubParser::parse for ('%s', %p)", qPrintable(filename), volume);
    ns_update_series(volume, filename);
    nh_log("... calling original parser");
    EPubParser__parse(_this, filename, volume);
}

extern "C" __attribute__((visibility("default"))) void _ns_epub_cid_hook(Content *_this, QVariant const& cid) {
    QString s = cid.toString();
    if (s.startsWith("file://") && !s.contains("!") && !s.contains("#") && s.toLower().endsWith(".epub")) {
        ns_update_series(_this, s.remove("file://"));
        nh_log("hook: intercepting EPUB Content::setId from libadobe for ('%s', %p)", qPrintable(s), _this);
        nh_log("... calling original function");
    }
    Content__setId(_this, cid);
}
