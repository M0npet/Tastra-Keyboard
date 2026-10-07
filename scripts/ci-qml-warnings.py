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
    report_failed_tests(Path(sys.argv[1]).read_text(encoding='utf-8', errors='replace').splitlines())
    return 0


def report_failed_tests(lines: list[str]) -> None:
    """The CI logs themselves cannot be fetched from where the keyboard is
    developed, but annotations can: one error per failed test with the
    lines QtTest prints for a failure (FAIL!, Actual, Expected, Loc) and the
    last lines of its output."""
    failed = {}
    for line in lines:
        # "Test #17: name .....***Failed", "...Subprocess aborted***Exception: ..."
        m = re.search(r'Test\s+#(\d+): (\S+) .*?\*\*\*\s*(\w[\w ]*)', line)
        if m:
            failed[m.group(1)] = (m.group(2), m.group(3).strip())
        # ctest's summary: "	 17 - name (Failed)"
        m = re.match(r'\s+(\d+) - (\S+) \((.+)\)\s*$', line)
        if m and m.group(1) not in failed:
            failed[m.group(1)] = (m.group(2), m.group(3))
    for number, (name, kind) in failed.items():
        prefix = f'{number}: '
        output = [l[len(prefix):] for l in lines if l.startswith(prefix)]
        important = [l for l in output if re.search(r'FAIL!|Actual|Expected|Loc:|QFATAL|error|Error|failed|Segmentation|Aborted', l)]
        text = '\n'.join(important[:20] + ['--- last lines ---'] + output[-15:])
        text = text.replace('%', '%25').replace('\r', '').replace('\n', '%0A')
        print(f'::error title=Test {name} {kind}::{text}')


if __name__ == '__main__':
    sys.exit(main())
