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

// SOME NOTES ABOUT PARSING AND HOOKING
//
// Content is the superclass of Volume and Shortcover. All three are DB model
// classes, which contain a setAttribute function which gets and calls the
// specific setter from a QMap<QString, void (Class::*)(QVariant const&)> in the
// class using getAttributeSetter, which will call getAttributeSetter from the
// superclass if none is found. These functions are not virtual.
//
// Content IDs:
// - Volume:
//   - A path with a file:// prefix, for local stuff.
//   - A UUID, for other stuff.
// - Shortcover: The above, but with:
//   - A suffix starting with #, for RMSDK locations.
//   - A suffix starting with !, for Kobo locations. ^ Note: Those characters
//     cannot legally occur in the book path itself.
//
// KEPUB:
// - EPubParser::parse is called to parse the provided filename.
// - EPubParser::parseMetadata (called by EPubParser::parse) sets the content ID
//   to file://... using Volume::setAttribute.
// - It's pretty easy to add metadata here, since we have easy access to the
//   Volume object.
//
// EPUB:
// - adobehost provides a DBus interface from a separate process to
//   ParserService, ReaderService, and SearchService (which happen to be
//   implemented in libnickel, although nickel calls them over DBus, so this
//   isn't relevant to us).
// - adobehost isn't really relevant right now either, but it *might* be in the
//   future.
// - libadobe is loaded by nickel, and provides the PluginInterface for parsing
//   adobe content, which is called by nickel directly.
// - libadobe's AdobeParser does the same sort of thing as the KEPUB EPubParser.
// - libadobe's AdobeParser has a slightly more convoluted implementation, where
//   it's AdobeParser::parse(QString const&) just sets an internal variable
//   (AdobeParser + 0x8 as of 15015) and calls AdobeParser::parse(void), which
//   actually does the work. Nevertheless, since the QString one is what the
//   interface requires, it will be called.
// - This is where the convoluted design starts. Even though most of the
//   required stuff is in the libraries nickel loads, AdobeParser::parse(void)
//   wants to call adobehost over DBus to get a QMap<QString, QVariant> of
//   properties which will eventually be set on the Volume. All adobehost does
//   is get the data from rmsdk.
// - Since libadobe is loaded as a nickel plugin, it is opened using dlopen. If
//   a library is already opened, the existing pointer will be returned. This
//   means we can load libadobe manually with dlopen from the plugin like we do
//   to hook libnickel and hook the base AdobeParser::parse(QString const&) to
//   be able to set some data before or after (but not during, since the
//   in-between stuff is handled by adobehost in a separate process, and I'd
//   rather not have a plugin for multiple ones) the attributes on the Volume
//   are set.
// - This gets slightly more complicated since AdobeParser only creates the
//   Volume itself a bit into AdobeParser::parse, just before doing the DBus
//   call.
// - The easiest way is probably to hook a setter used in AdobeParser::parse
//   when applying the metadata from the returned QMap.
//
// FB2:
// - Out of the scope for NickelSeries, at least for now. It's slightly harder
//   than KEPUB, but easier than EPUB.
//
// One way to avoid most of these intricacies is to hook some setter on Content
// for libnickel and all parser plugins. Ideally, that setter should be called
// after the ID for the Volume is set so we can retrieve it immediately and add
// the series metadata at that point. It should also be only called during
// parsing, so we don't have to deal with saving it and doing unnecessary work
// parsing an already-parsed EPUB.
//
// After some more thought, the best way is probably a combination, as the
// priorities are: to have it called during parsing only if possible (for
// performance, and to reduce the severity and scope if there happens to be a
// serious bug in NickelSeries), to have the hook's scope as limited as possible
// (also for stability), to have the hook unlikely to change between versions
// (for the best backwards/forwards compatibility), to be able to set the
// metadata as early as possible (to allow it to be overridden if it ever is
// implemented officially) and to have access to the filename and Volume
// directly from or as close to the hook if possible (for stability, simplicity,
// and compatiblity).
//
// Therefore, for KEPUB, we can just hook EPubParser::parse on libnickel in
// nickel and take the filename and Volume from the arguments to set the
// metadata at the very beginning. For EPUB, we can hook Content::setId on the
// libadobe loaded by libnickel in nickel, check the extension to ensure it's a
// Volume and not a Shortcover, then update the metadata from there (remember
// that the ContentId is the file path as a URI).
//
// Now, we need to deal with setting the metadata itself. For this, there are
// two options. The first way is to use Volume::setAttribute, which takes a
// QString with the column name (usually used with ATTRIBUTE_* global variables,
// which are pointers to a global QString initialized when loading libnickel).
// This way has the advantage of a single function being needed, and better
// compatibility if the names ever change (however highly unlikely that is). It
// is also less susceptible to compiler optimizations messing up any
// assumptions. The disadvantage is the ATTRIBUTE_* variables don't have any
// type information in the symbol name, but that's unlikely to change and we
// don't necessarily care about the contents, as we only need to pass them on.
// Another thing is that it will gracefully ignore invalid attributes, which is
// both an advantage and a disadvantage. The KEPUB parser uses this method for
// setting metadata. The second way is to use Volume::set* directly, which is
// used by the adobe parser. This has the advantage of not needing to make
// assumptions about types, but it also comes with the disadvantage of being
// more dependent on how the compiler chooses to handle types with regards to
// vtables and the like (in contrast to setAttribute, which would generally deal
// with any changes for us). Although, this is not likely to cause issues in
// practice, since it appears to currently consist of setting the property on an
// internal QMap<QString, QVariant> to be saved later (the function appears so
// large due to inlining of the QMap code). Note that these attribute-related
// things apply to all DB models in libnickel.
//
// In the end, both these options are basically equivalent, as what would break
// one is likely to break the other in a similar way too. I was originally going
// to go with Volume::setAttribute, but I ended up going with Volume::set*
// instead, as it was easier to work with.

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

void ns_init2() {
    NS_LOG("init: resolving symbols");

    void *nph = dlsym(RTLD_DEFAULT, "_ns_kepub_parse_hook");
    if (!nph) {
        NS_LOG("fatal: could not dlsym _ns_kepub_parse_hook");
        return;
    }

    void *nch = dlsym(RTLD_DEFAULT, "_ns_epub_cid_hook");
    if (!nph) {
        NS_LOG("fatal: could not dlsym _ns_epub_cid_hook");
        return;
    }

    void *libnickel = dlopen("libnickel.so.1.0.0", RTLD_LAZY|RTLD_NODELETE);
    if (!libnickel) {
        NS_LOG("fatal: could not dlopen libnickel");
        return;
    }
    
    //libnickel 3.19.5761 * _ZN6Volume13setSeriesNameERK8QVariant
    reinterpret_cast<void*&>(Volume__setSeriesName) = dlsym(libnickel, "_ZN6Volume13setSeriesNameERK8QVariant");
    if (!Volume__setSeriesName) {
        NS_LOG("fatal: could not dlsym Volume::setSeriesName");
        return;
    }

    //libnickel 3.19.5761 * _ZN6Volume15setSeriesNumberERK8QVariant
    reinterpret_cast<void*&>(Volume__setSeriesNumber) = dlsym(libnickel, "_ZN6Volume15setSeriesNumberERK8QVariant");
    if (!Volume__setSeriesNumber) {
        NS_LOG("fatal: could not dlsym Volume::setSeriesNumber");
        return;
    }

    // idk what version this first appeared in, but I know it wasn't in 3.x
    //libnickel 4.6 * _ZN6Volume20setSeriesNumberFloatERK8QVariant
    reinterpret_cast<void*&>(Volume__setSeriesNumberFloat) = dlsym(libnickel, "_ZN6Volume20setSeriesNumberFloatERK8QVariant");
    if (!Volume__setSeriesNumberFloat)
        NS_LOG("note: could not dlsym Volume::setSeriesNumberFloat, not updating it");

    //libnickel 4.20.14601 * _ZN6Volume11setSeriesIdERK8QVariant
    reinterpret_cast<void*&>(Volume__setSeriesId) = dlsym(libnickel, "_ZN6Volume11setSeriesIdERK8QVariant");
    if (!Volume__setSeriesId)
        NS_LOG("note: could not dlsym Volume::setSeriesId, not updating series id for Series tab of My Books");

    NS_LOG("init: hooking EPubParser::parse on libnickel for KEPUBs");

    //libnickel 3.19.5761 * _ZN10EPubParser5parseERK7QStringRK6Volume
    char *err;
    reinterpret_cast<void*&>(EPubParser__parse) = nm_dlhook(libnickel, "_ZN10EPubParser5parseERK7QStringRK6Volume", nph, &err);
    if (err) {
        NS_LOG("note: could not hook EPubParser::parse, will not parse series metadata for KEPUBs");
        free(err);
    }

    NS_LOG("init: hooking Content::setId on libadobe for KEPUBs");

    void *libadobe = dlopen("libadobe.so", RTLD_LAZY|RTLD_NODELETE);
    if (!libadobe) {
        NS_LOG("note: could not dlopen libadobe, will not parse series metadata for EPUBs");
        return;
    }

    //libnickel 3.19.5761 * _ZN7Content5setIdERK8QVariant
    //libadobe 3.19.5761 * _ZN7Content5setIdERK8QVariant
    reinterpret_cast<void*&>(Content__setId) = nm_dlhook(libadobe, "_ZN7Content5setIdERK8QVariant", nch, &err);
    if (err) {
        NS_LOG("note: could not hook Content::setId on libadobe, will not parse series metadata for EPUBs");
        free(err);
        return;
    }
}

void ns_update_series(Volume *v, QString const& filename) {
    NS_LOG("hook: updating series metadata for '%s'", qPrintable(filename));

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

        NS_LOG("... using ('%s', %s)", qPrintable(series), qPrintable(index));

        if (Volume__setSeriesName) {
            NS_LOG("... Volume::setSeriesName('%s')", qPrintable(series));
            Volume__setSeriesName(v, series);
        }

        if (Volume__setSeriesNumber) {
            NS_LOG("... Volume::setSeriesNumber('%s')", qPrintable(index));
            Volume__setSeriesNumber(v, index);
        }

        if (Volume__setSeriesNumberFloat) {
            NS_LOG("... Volume::setSeriesNumberFloat('%s')", qPrintable(index));
            Volume__setSeriesNumberFloat(v, index);
        }

        if (Volume__setSeriesId) {
            NS_LOG("... Volume::setSeriesId('%s')", qPrintable(series));
            Volume__setSeriesId(v, series); // matches the Calibre Kobo plugin's behaviour for compatibility
        }
    }
}

extern "C" void _ns_epub_cid_hook(Content *_this, QVariant const& cid) {
    QString s = cid.toString();
    NS_LOG("hook: intercepting EPUB Content::setId from libadobe for ('%s', %p)", qPrintable(s), _this);
    if (s.startsWith("file://") && !s.contains("!") && !s.contains("#") && s.toLower().endsWith(".epub"))
        ns_update_series(_this, s.remove("file://"));
    NS_LOG("... calling original function");
    Content__setId(_this, cid);
}

extern "C" void _ns_kepub_parse_hook(EPubParser *_this, QString const& filename, Volume /*const&*/* volume) {
    NS_LOG("hook: intercepting KEPUB EPubParser::parse for ('%s', %p)", qPrintable(filename), volume);
    ns_update_series(volume, filename);
    NS_LOG("... calling original parser");
    EPubParser__parse(_this, filename, volume);
}
