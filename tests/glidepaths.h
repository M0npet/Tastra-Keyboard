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
}
