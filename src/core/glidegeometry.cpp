// SPDX-License-Identifier: GPL-3.0-or-later

#include "glidegeometry.h"

#include <QLineF>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Tastra::Glide
{

QVector<QPointF> resample(const QVector<QPointF> &path, int n)
{
    if (path.isEmpty() || n <= 0) return {};
    if (n == 1) return {path.first()};
    QVector<double> cumulative(path.size(), 0.0);
    for (int i = 1; i < path.size(); ++i) cumulative[i] = cumulative[i - 1] + QLineF(path.at(i - 1), path.at(i)).length();
    const double total = cumulative.last();
    if (total <= 0.0) return QVector<QPointF>(n, path.first());

    QVector<QPointF> out;
    out.reserve(n);
    int segment = 1;
    for (int k = 0; k < n; ++k) {
        const double target = total * k / (n - 1);
        while (segment < path.size() - 1 && cumulative.at(segment) < target) ++segment;
        const double start = cumulative.at(segment - 1);
        const double length = cumulative.at(segment) - start;
        const double t = length > 0.0 ? qBound(0.0, (target - start) / length, 1.0) : 0.0;
        out.append(path.at(segment - 1) + (path.at(segment) - path.at(segment - 1)) * t);
    }
    out.last() = path.last();
    return out;
}

QChar baseLetter(QChar ch)
{
    switch (ch.unicode()) {
    case 0x0451: return QChar(0x0435);                   // ё -> е
    case 0x0491: return QChar(0x0433);                   // ґ -> г
    case 0x00DF: return QLatin1Char('s');                // ß -> s
    case 0x00E4: case 0x00E0: case 0x00E1: case 0x00E2: return QLatin1Char('a');
    case 0x00F6: case 0x00F2: case 0x00F3: case 0x00F4: return QLatin1Char('o');
    case 0x00FC: case 0x00F9: case 0x00FA: case 0x00FB: return QLatin1Char('u');
    case 0x00E9: case 0x00E8: case 0x00EA: case 0x00EB: return QLatin1Char('e');
    case 0x00EF: case 0x00EE: case 0x00ED: case 0x00EC: return QLatin1Char('i');
    case 0x00E7: return QLatin1Char('c');
    case 0x00F1: return QLatin1Char('n');
    default: return ch;
    }
}

QVector<QPointF> idealPath(const QString &word, const QHash<QChar, QPointF> &centres)
{
    QVector<QPointF> path;
    QChar previous;
    for (const QChar raw : word) {
        const QChar ch = raw.toLower();
        if (ch == QLatin1Char('\'') || ch == QLatin1Char('-') || ch == QChar(0x2019) || ch == QChar(0x02BC)) continue;
        auto it = centres.constFind(ch);
        // A letter without its own key is typed on its base letter's key
        // (long-press): ё on е, ß on s, ü on u, ґ on г.
        if (it == centres.constEnd()) it = centres.constFind(baseLetter(ch));
        if (it == centres.constEnd()) return {};
        if (ch == previous) continue;   // a doubled letter is one key
        previous = ch;
        path.append(it.value());
    }
    return path;
}

double shapeDistance(const QVector<QPointF> &a, const QVector<QPointF> &b)
{
    if (a.isEmpty() || a.size() != b.size()) return std::numeric_limits<double>::infinity();
    auto normalised = [](const QVector<QPointF> &points) {
        QPointF centre;
        double minX = points.first().x(), maxX = minX, minY = points.first().y(), maxY = minY;
        for (const QPointF &p : points) {
            centre += p;
            minX = std::min(minX, p.x()); maxX = std::max(maxX, p.x());
            minY = std::min(minY, p.y()); maxY = std::max(maxY, p.y());
        }
        centre /= points.size();
        const double scale = std::max({maxX - minX, maxY - minY, 1e-9});
        QVector<QPointF> out;
        out.reserve(points.size());
        for (const QPointF &p : points) out.append((p - centre) / scale);
        return out;
    };
    const QVector<QPointF> na = normalised(a);
    const QVector<QPointF> nb = normalised(b);
    double sum = 0.0;
    for (int i = 0; i < na.size(); ++i) sum += QLineF(na.at(i), nb.at(i)).length();
    return sum / na.size();
}

double distanceToPath(const QPointF &point, const QVector<QPointF> &path)
{
    if (path.isEmpty()) return std::numeric_limits<double>::infinity();
    if (path.size() == 1) return QLineF(point, path.first()).length();
    double best = std::numeric_limits<double>::infinity();
    for (int i = 1; i < path.size(); ++i) {
        const QPointF a = path.at(i - 1);
        const QPointF ab = path.at(i) - a;
        const double lengthSquared = QPointF::dotProduct(ab, ab);
        const double t = lengthSquared > 0.0 ? qBound(0.0, QPointF::dotProduct(point - a, ab) / lengthSquared, 1.0) : 0.0;
        best = std::min(best, QLineF(point, a + ab * t).length());
    }
    return best;
}

}
