#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPair>
#include <QString>
#include <QXmlStreamReader>

#include <miniz.h>

#include "metadata.h"
#include "qxmlstream.h"

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

class MzZipArchiveReader {
public:
    explicit MzZipArchiveReader(const char *filename, mz_uint32 flags = 0) : _z({}), _v(mz_zip_reader_init_file(&_z, filename, flags)) {}
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

const QString NSMetadata::calibre = QStringLiteral("!calibre");

NSMetadata::NSMetadata(const char *filename) {
    MzZipArchiveReader zip(filename);
    if (!zip.valid()) {
        this->_error = QStringLiteral("parse epub '%1': open zip: %2").arg(filename).arg(zip.error());
        return;
    }

    QByteArray buf = zip.extract_file("META-INF/container.xml");
    if (buf.isNull()) {
        this->_error = QStringLiteral("parse epub '%1': read container 'META-INF/container.xml': %2").arg(filename).arg(zip.error());
        return;
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
            this->_error = QStringLiteral("parse epub '%1': read package '%2': %3").arg(filename).arg(r.attributes().value("full-path").toString()).arg(zip.error());
            return;
        }
    }

    if (r.hasError()) {
        this->_error = QStringLiteral("parse epub '%1': parse container: %2").arg(filename).arg(r.errorString());
        return;
    } else if (buf.isNull()) {
        this->_error = QStringLiteral("parse epub '%1': parse container: could not find rootfile").arg(filename);
        return;
    }

    r.clear();
    r.addData(buf);

    this->init(r);
    if (!this->_error.isNull())
        this->_error = QStringLiteral("parse epub '%1': %2").arg(filename).arg(this->_error);
}

NSMetadata::NSMetadata(QXmlStreamReader &r) {
    this->init(r);
}

QString const &NSMetadata::error() const {
    return this->_error;
}

void NSMetadata::init(QXmlStreamReader &r) {
    QMap<QString, bool> titles;
    for (int x[4] = {}; !r.atEnd(); r.readNext()) {
        switch (r.tokenType()) {
        case QXmlStreamReader::StartElement:
            switch (++x[0]) {
            case 1: x[1] = r.name() == "package"; break;
            case 2: x[2] = r.name() == "metadata"; break;
            case 3: x[3] = r.name() == "meta" ? 1 : r.qualifiedName() == "dc:title" ? 2 : 0; break;
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

        if (x[3] == 2) {
            QString id = r.attributes().value("id").toString();
            if (id.isNull())
                continue;
            QString text = r.readElementText(QXmlStreamReader::SkipChildElements);
            if (r.tokenType() == QXmlStreamReader::EndElement)
                if (x[0]-- == 3)
                    x[3] = false;
            if (text.isEmpty())
                continue;
            this->subtitle[id] = text;
            continue;
        }

        QStringRef name = r.attributes().value("name");
        if (name.startsWith("calibre:series")) {
            if (name == "calibre:series")
                this->series[NSMetadata::calibre].first = r.attributes().value("content").toString();
            else if (name == "calibre:series_index")
                this->series[NSMetadata::calibre].second = r.attributes().value("content").toString();
            continue;
        } else if (name == "calibre:user_metadata:#subtitle") {
            QJsonDocument doc = QJsonDocument::fromJson(r.attributes().value("content").toString().toLatin1());
            if (doc.isNull())
                continue;

            QJsonValue val = doc.object().value("#value#");
            if (!val.isString())
                continue;

            this->subtitle[NSMetadata::calibre] = val.toString();
            titles[NSMetadata::calibre] = true;
            continue;
        }

        name = r.attributes().value("property");
        if (name == "belongs-to-collection" || name == "collection-type" || name == "group-position") {
            QString id = r.attributes().value("refines").startsWith("#")
                ? r.attributes().value("refines").mid(1).toString()
                : r.attributes().value("id").toString();

            QString text = r.readElementText(QXmlStreamReader::SkipChildElements);
            if (r.tokenType() == QXmlStreamReader::EndElement)
                if (x[0]-- == 3)
                    x[3] = false;
            if (text.isEmpty())
                continue;

            if (name == "collection-type" && text != "series")
                this->series.remove(id);
            else if (name == "group-position")
                this->series[id].second = text;
            else if (!this->series.contains(id) && name == "belongs-to-collection" && !id.isEmpty())
                this->series[id].first = text;
            continue;
        } else if (name == "title-type") {
            if (!r.attributes().value("refines").startsWith("#"))
                continue;
            QString id = r.attributes().value("refines").mid(1).toString();

            QString text = r.readElementText(QXmlStreamReader::SkipChildElements);
            if (r.tokenType() == QXmlStreamReader::EndElement)
                if (x[0]-- == 3)
                    x[3] = false;
            if (text.isEmpty())
                continue;

            if (text == "subtitle")
                titles[id] = true;
            continue;
        }
    }

    for (QString source : this->series.keys())
        if (this->series[source].first.isEmpty() || this->series[source].second.isEmpty())
            this->series.remove(source);

    for (QString source : this->subtitle.keys())
        if (!titles[source] || this->subtitle[source].isEmpty())
            this->subtitle.remove(source);

    if (r.hasError())
        this->_error = QStringLiteral("parse package: %1").arg(r.errorString());
}
