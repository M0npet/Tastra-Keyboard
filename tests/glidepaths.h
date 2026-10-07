// SPDX-License-Identifier: GPL-3.0-or-later
//
// Synthetic glide paths for tests: key centres as laid out on screen and a
// finger path through a word's keys with seeded noise.

#pragma once

#include <QHash>
#include <QLineF>
#include <QPointF>
#include <QRandomGenerator>
#include <QString>
#include <QVector>

#include <cmath>

#include "core/glidegeometry.h"
#include "core/keyboardmodel.h"

namespace GlidePaths
{
constexpr double KeyWidth = 80.0;

// The middle row is shifted by half a key, the bottom row by one and a half
// (Shift sits on its left).
inline QHash<QChar, QPointF> centresFor(const QString &language)
{
    const QStringList rows = Tastra::KeyboardModel::rowsForLanguage(language);
    const double offsets[] = {0.0, 0.5, 1.5};
    QHash<QChar, QPointF> centres;
    for (int r = 0; r < qMin(3, int(rows.size())); ++r) {
        for (int i = 0; i < rows.at(r).size(); ++i) {
            centres.insert(rows.at(r).at(i), QPointF((offsets[r] + i + 0.5) * KeyWidth, (r + 0.5) * KeyWidth * 1.3));
        }
    }
    return centres;
}

// Each corner lands up to `jitter` key widths off the centre; segments are
// sampled every few pixels with a little tremor. Letters without a key are
// glided over their base letter's key, a doubled letter is one key.
inline QVector<QPointF> pathFor(const QString &word, const QHash<QChar, QPointF> &centres, double jitter, quint32 seed)
{
    QRandomGenerator rng(seed);
    auto noise = [&](double amount) { return (rng.generateDouble() * 2.0 - 1.0) * amount * KeyWidth; };
    QVector<QPointF> corners;
    QChar previous;
    for (const QChar letter : word) {
        const QChar ch = centres.contains(letter) ? letter : Tastra::Glide::baseLetter(letter);
        if (!centres.contains(ch) || ch == previous) continue;
        previous = ch;
        corners.append(centres.value(ch) + QPointF(noise(jitter), noise(jitter)));
    }
    QVector<QPointF> path;
    for (int i = 0; i + 1 < corners.size(); ++i) {
        const QLineF segment(corners.at(i), corners.at(i + 1));
        const int steps = qMax(2, int(segment.length() / 6.0));
        for (int s = 0; s < steps; ++s) path.append(segment.pointAt(double(s) / steps) + QPointF(noise(0.02), noise(0.02)));
    }
    if (!corners.isEmpty()) path.append(corners.last());
    return path;
}

// A more human glide: each corner lands with Gaussian scatter (`sigma` key
// widths), interior corners are cut toward their neighbours' midpoint by
// `cut` (at most 0.6 key widths: the finger turns before reaching the key),
// and the corners are joined by a smooth Catmull-Rom curve instead of
// straight strokes.
inline QVector<QPointF> humanPathFor(const QString &word, const QHash<QChar, QPointF> &centres, double sigma, double cut, quint32 seed)
{
    QRandomGenerator rng(seed);
    auto gaussian = [&]() {
        const double u1 = qMax(1e-12, rng.generateDouble()), u2 = rng.generateDouble();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    };
    QVector<QPointF> corners;
    QChar previous;
    for (const QChar letter : word) {
        const QChar ch = centres.contains(letter) ? letter : Tastra::Glide::baseLetter(letter);
        if (!centres.contains(ch) || ch == previous) continue;
        previous = ch;
        corners.append(centres.value(ch) + QPointF(gaussian() * sigma * KeyWidth, gaussian() * sigma * KeyWidth * 0.8));
    }
    if (corners.size() < 2) return corners;
    QVector<QPointF> cutCorners = corners;
    for (int i = 1; i + 1 < corners.size(); ++i) {
        QPointF shift = ((corners.at(i - 1) + corners.at(i + 1)) / 2.0 - corners.at(i)) * cut;
        const double length = std::hypot(shift.x(), shift.y());
        if (length > 0.6 * KeyWidth) shift *= 0.6 * KeyWidth / length;
        cutCorners[i] = corners.at(i) + shift;
    }
    QVector<QPointF> path;
    const int last = int(cutCorners.size()) - 1;
    for (int i = 0; i < last; ++i) {
        const QPointF p0 = cutCorners.at(qMax(0, i - 1)), p1 = cutCorners.at(i), p2 = cutCorners.at(i + 1),
                      p3 = cutCorners.at(qMin(last, i + 2));
        const int steps = qMax(4, int(QLineF(p1, p2).length() / 5.0));
        for (int s = 0; s < steps; ++s) {
            const double t = double(s) / steps, t2 = t * t, t3 = t2 * t;
            path.append(0.5 * ((2.0 * p1) + (p2 - p0) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2
                               + (3.0 * p1 - p0 - 3.0 * p2 + p3) * t3));
        }
    }
    path.append(cutCorners.last());
    return path;
}
}
