#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Turn the QML runtime warnings in a verbose ctest log into GitHub
annotations, so warnings of the CI's Qt version (the tablet's) are visible
without reading the whole log. Never fails the build.

    ctest --test-dir build -V 2>&1 | tee ctest.log
    python3 scripts/ci-qml-warnings.py ctest.log
"""
import re
import sys
from collections import Counter
from pathlib import Path

PATTERN = re.compile(r'file://\S*?/(src/ui/[A-Za-z]+\.qml):(\d+):(?:\d+:)?\s*(.+)$')


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 0
    seen = Counter()
    places = {}
    for line in Path(sys.argv[1]).read_text(encoding='utf-8', errors='replace').splitlines():
        match = PATTERN.search(line)
        if not match:
            continue
        path, row, message = match.group(1), match.group(2), match.group(3).strip()
        key = (path, row, message)
        seen[key] += 1
        places.setdefault(key, (path, row))
    print(f'{len(seen)} distinct QML warnings ({sum(seen.values())} in total)')
    for (path, row, message), count in seen.most_common():
        text = message.replace('%', '%25').replace('\r', '').replace('\n', ' ')
        print(f'::warning file={path},line={row}::{text} (x{count})')
    return 0


if __name__ == '__main__':
    sys.exit(main())
