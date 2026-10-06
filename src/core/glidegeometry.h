// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QPointF>
#include <QString>
#include <QVector>

namespace Tastra::Glide
{

// `n` points spaced evenly along the path (first and last kept).
QVector<QPointF> resample(const QVector<QPointF> &path, int n);

// The key a letter without its own key is typed on (ё -> е, ß -> s, ü -> u).
QChar baseLetter(QChar ch);

// The path a perfect glide of `word` takes: through the centres of its keys,
// a repeated letter once ("hello" -> h e l o). Apostrophes and hyphens are
// skipped ("don't" glides d-o-n-t). Empty if any other letter has no key.
QVector<QPointF> idealPath(const QString &word, const QHash<QChar, QPointF> &centres);

// Mean distance between two equally sampled paths after moving each to its
// centroid and scaling its larger side to 1 (SHARK2's shape channel).
double shapeDistance(const QVector<QPointF> &a, const QVector<QPointF> &b);

// Shortest distance from `point` to the polyline `path`.
double distanceToPath(const QPointF &point, const QVector<QPointF> &path);

}
