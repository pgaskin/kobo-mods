#ifndef NS_SERIES_H
#define NS_SERIES_H

#include <QMap>
#include <QPair>

// ns_series reads the series Calibre/EPUB3 metadata from the specified EPUB 2/3
// file and is returned as a map of IDs (the Calibre metadata is !Calibre) to a
// pair of the name/index. If an error occurs, isEmpty() is true for the map and
// the string is !isNull() with the error message. If there isn't any series
// metadata, the isNull() is true for the string and isEmpty() is true for the
// map. If there is series metadata, it is returned in reverse order of
// appearance in the OPF document.
QPair<QMap<QString, QPair<QString, QString>>, QString> ns_series(const char *filename);

#endif
