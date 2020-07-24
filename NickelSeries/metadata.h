#ifndef NS_SERIES_H
#define NS_SERIES_H

#include <QMap>
#include <QPair>
#include <QXmlStreamReader>

// NSMetadata represents the metadata for an EPUB 2/3 file.
struct NSMetadata {
    // calibre is a special ID returned for Calibre metadata.
    static const QString calibre;

    // series is a map of the element ID to its series name and index. The
    // series index is returned as-is and may not be a number, but both the name
    // and index will not be empty.
    //
    // Sources:
    // - Calibre: meta[name="calibre:series"][content]          source =  ns_metadata_t::calibre
    //                                                          series =  [content]
    //            meta[name="calibre:series_index"][content]    index  =  [content]
    // - EPUB3:   meta[property="belongs-to-collection"][id]    source =  [id]
    //                                                          series =  text
    //            meta[property="group-position"][refines=#id]  index  =  text
    //            meta[property="collection-type"][refines=#id] text   == "series"  || none
    QMap<QString, QPair<QString, QString>> series;

    // NSMetadata reads metadata from the specified EPUB 2/3 file.
    NSMetadata(QString const &filename);
    NSMetadata(const char *filename);

    // NSMetadata reads metadata from the provided package document.
    NSMetadata(QXmlStreamReader &opf);

    // error returns the first error, if any, which ocurred while reading the
    // metadata.
    const QString &error() const;

private:
    QString _error;
    void init(QXmlStreamReader &opf);
};

#endif
