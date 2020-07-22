#include <QByteArray>
#include <QMap>
#include <QPair>
#include <QString>
#include <QXmlStreamReader>

#include <miniz.h>

#include "series.h"

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

class MzZipArchiveReader {
public:
    explicit MzZipArchiveReader(const char *filename, mz_uint32 flags = 0) : _z({}),_v(mz_zip_reader_init_file(&_z, filename, flags)) {}
    MzZipArchiveReader(const MzZipArchiveReader&) = delete;
    MzZipArchiveReader& operator=(MzZipArchiveReader const&) = delete;
    ~MzZipArchiveReader() { if (_v) mz_zip_reader_end(&_z); }
    bool valid() const { return _v; };
    const char *error() { mz_zip_error e = mz_zip_get_last_error(&_z); return e == MZ_ZIP_NO_ERROR ? NULL : mz_zip_get_error_string(e); }
    QByteArray extract_file(const char *path, mz_uint32 flags = 0) { char *b; size_t s; if (!(b = static_cast<char*>(mz_zip_reader_extract_file_to_heap(&_z, path, &s, flags)))) return nullptr; QByteArray r = QByteArray(b, s); free(b); return r; }
    operator mz_zip_archive*() { return &_z; }
private:
    mz_zip_archive _z;
    bool _v;
};

QPair<QMap<QString, QPair<QString, QString>>, QString> ns_series(const char *filename) {
    QPair<QMap<QString, QPair<QString, QString>>, QString> result;

    MzZipArchiveReader zip(filename);
    if (!zip.valid()) {
        result.second = QStringLiteral("parse epub '%1': open zip: %2").arg(filename).arg(zip.error());
        return result;
    }

    QByteArray buf = zip.extract_file("META-INF/container.xml");
    if (buf.isNull()) {
        result.second = QStringLiteral("parse epub '%1': read container 'META-INF/container.xml': %2").arg(filename).arg(zip.error());
        return result;
    }

    QXmlStreamReader r;
    r.addData(buf);

    buf = nullptr;
    for (int x[4] = {}; !r.atEnd(); r.readNext()) {
        switch (r.tokenType()) {
        case QXmlStreamReader::StartElement:
            switch (++x[0]) {
            case 1: x[1] = r.name() == "container"; break;
            case 2: x[2] = r.name() == "rootfiles"; break;
            case 3: x[3] = r.name() == "rootfile";  break;
            }
            break;
        case QXmlStreamReader::EndElement:
            switch (x[0]--) {
            case 3: x[3] = false; break;
            case 2: x[2] = false; break;
            case 1: x[1] = false; break;
            }
            break;
        default:
            break;
        }
        if (r.tokenType() != QXmlStreamReader::StartElement || x[0] != 3 || !x[1] || !x[2] || !x[3])
            continue;
        if (r.attributes().value("media-type") != "application/oebps-package+xml")
            continue;
        if ((buf = zip.extract_file(r.attributes().value("full-path").toLatin1().constData())).isNull()) {
            result.second = QStringLiteral("parse epub '%1': read package '%2': %3").arg(filename).arg(r.attributes().value("full-path").toString()).arg(zip.error());
            return result;
        }
    }

    if (r.hasError()) {
        result.second = QStringLiteral("parse epub '%1': parse container: %2").arg(filename).arg(r.errorString());
        return result;
    } else if (buf.isNull()) {
        result.second = QStringLiteral("parse epub '%1': parse container: could not find rootfile").arg(filename);
        return result;
    }

    r.clear();
    r.addData(buf);

    QMap<QString, QPair<QString, QString>> series;
    for (int x[4] = {}; !r.atEnd(); r.readNext()) {
        switch (r.tokenType()) {
        case QXmlStreamReader::StartElement:
            switch (++x[0]) {
            case 1: x[1] = r.name() == "package"; break;
            case 2: x[2] = r.name() == "metadata"; break;
            case 3: x[3] = r.name() == "meta"; break;
            }
            break;
        case QXmlStreamReader::EndElement:
            switch (x[0]--) {
            case 3: x[3] = false; break;
            case 2: x[2] = false; break;
            case 1: x[1] = false; break;
            }
            break;
        default:
            break;
        }
        if (r.tokenType() != QXmlStreamReader::StartElement || x[0] != 3 || !x[1] || !x[2] || !x[3])
            continue;

        QStringRef name = r.attributes().value("name");
        if (name.startsWith("calibre:series")) {
            if (name == "calibre:series")
                series["!calibre"].first = r.attributes().value("content").toString();
            else if (name == "calibre:series_index")
                series["!calibre"].second = r.attributes().value("content").toString();
        }

        name = r.attributes().value("property");
        if (name == "belongs-to-collection" || name == "collection-type" || name == "group-position") {
            QString property = name.toString();
            QString id = r.attributes().value("refines").startsWith("#")
                ? r.attributes().value("refines").mid(1).toString()
                : r.attributes().value("id").toString();

            QString text = r.readElementText(QXmlStreamReader::SkipChildElements);
            if (r.tokenType() == QXmlStreamReader::EndElement)
                if (x[0]-- == 3)
                    x[3] = false;
            if (text.isEmpty())
                continue;

            if (property == "collection-type" && text != "series")
                series.remove(id);
            else if (property == "group-position")
                series[id].second = text;
            else if (!series.contains(id))
                if (property == "belongs-to-collection" && !id.isEmpty())
                    series[id].first = text;
        }
    }

    if (r.hasError()) {
        result.second = QStringLiteral("parse epub '%1': parse package: %2").arg(filename).arg(r.errorString());
        return result;
    }

    for (QString source : series.keys())
        if (series[source].first.isEmpty() || series[source].second.isEmpty())
            series.remove(source);

    result.first = series;
    return result;
}
