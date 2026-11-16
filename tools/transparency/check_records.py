# PROJECT: VerOS NT
# LICENSE: GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE: tools/transparency/check_records.py
# PURPOSE: Reject a scoped commit that changes Windows behavior and omits its source note
# COPYRIGHT: Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

"""Check transparency notes for scoped commits in a revision range."""

import json
import os
import re
import subprocess
import sys
from datetime import datetime

SCOPED_PREFIXES = (
    "boot/",
    "ntoskrnl/",
    "hal/",
    "drivers/",
    "dll/",
    "win32ss/",
    "sdk/lib/fslib/ntfslib/",
)

DATE_FILE = re.compile(
    r"^veros/transparency/(\d{4}-\d{2}-\d{2})-[A-Za-z0-9][A-Za-z0-9-]*\.json$"
)
SHA_FILE = re.compile(r"^veros/transparency/([0-9a-f]{40})\.json$")
LOG_SHA = re.compile(r"^[0-9a-f]{64}$")
OUTPUT_FILE = re.compile(
    r"output/[A-Za-z0-9_./+-]+\.(?:log|json|txt|probe|err|ok)"
)
BANNED_FIELDS = ("forbiddenInputsNotUsed", "humanReview", "cursor")
NOTES_DIR = "veros/transparency"


def git(*args):
    result = subprocess.run(
        ["git", *args],
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout


def git_exists(*args):
    result = subprocess.run(
        ["git", *args],
        capture_output=True,
        text=True,
    )
    return result.returncode == 0


def repo_root():
    return git("rev-parse", "--show-toplevel").strip()


def is_scoped(path):
    return path.startswith(SCOPED_PREFIXES)


def require_text(record, field, errors, path):
    value = record.get(field)
    if not isinstance(value, str) or not value.strip():
        errors.append("%s: %s must be a non-empty string" % (path, field))
        return None
    return value


def cites_source(item):
    lowered = item.lower()
    if "https://" in lowered or "http://" in lowered or "/" in item:
        return True
    return lowered.startswith("reported:")


def note_name_error(sha, path):
    date_match = DATE_FILE.match(path)
    sha_match = SHA_FILE.match(path)
    if date_match:
        try:
            datetime.strptime(date_match.group(1), "%Y-%m-%d")
        except ValueError:
            return "%s: %s has a bad date" % (sha, path)
        return None
    if sha_match:
        named = sha_match.group(1)
        if named == sha:
            return "%s: %s cannot be named with this commit" % (sha, path)
        if not git_exists("cat-file", "-e", "%s^{commit}" % named):
            return "%s: %s does not name an earlier commit" % (sha, path)
        return None
    return "%s: %s is not a date slug or an earlier commit SHA" % (sha, path)


def validate_record(record, path):
    errors = []
    if not isinstance(record, dict):
        return ["%s: the note must be a JSON object" % path]
    for field in BANNED_FIELDS:
        if field in record:
            errors.append("%s: leave %s out of the note" % (path, field))
    spdx = require_text(record, "spdx", errors, path)
    if spdx is not None and spdx != "GPL-2.0-or-later":
        errors.append("%s: spdx must be GPL-2.0-or-later" % path)
    require_text(record, "question", errors, path)
    inputs = record.get("allowedInputs")
    if (
        not isinstance(inputs, list)
        or not inputs
        or not all(isinstance(item, str) and item.strip() for item in inputs)
    ):
        errors.append("%s: allowedInputs must name the sources that were used" % path)
    elif not all(cites_source(item) for item in inputs):
        errors.append(
            "%s: each allowedInputs entry needs a URL, a path, or a Reported: citation"
            % path
        )
    if "harness" in record:
        harness = record["harness"]
        if not isinstance(harness, dict):
            errors.append("%s: harness must name the guest that ran" % path)
        else:
            for field in ("guest", "name", "traceOrReplayId"):
                value = harness.get(field)
                if not isinstance(value, str) or not value.strip() or value == "none":
                    errors.append("%s: harness.%s must name the guest that ran" % (path, field))
            if "logSha256" in harness:
                digest = harness.get("logSha256")
                if not isinstance(digest, str) or LOG_SHA.match(digest) is None:
                    errors.append("%s: harness.logSha256 must be 64 hex characters" % path)
    if "measuredEffect" in record:
        effect = record["measuredEffect"]
        if not isinstance(effect, dict):
            errors.append("%s: measuredEffect must come from a committed log" % path)
        else:
            metric = effect.get("metric")
            if not isinstance(metric, str) or not metric.strip():
                errors.append("%s: measuredEffect.metric must name the log or snapshot" % path)
            for field in ("before", "after"):
                if effect.get(field) is None or isinstance(effect.get(field), bool):
                    errors.append(
                        "%s: measuredEffect.%s must come from a committed log" % (path, field)
                    )
    errors.extend(cited_log_errors(record, path))
    return errors


def record_strings(value):
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for item in value.values():
            for text in record_strings(item):
                yield text
    elif isinstance(value, list):
        for item in value:
            for text in record_strings(item):
                yield text


def cited_log_errors(record, path):
    errors = []
    root = repo_root()
    seen = set()
    for text in record_strings(record):
        for cited in OUTPUT_FILE.findall(text):
            if cited in seen:
                continue
            seen.add(cited)
            full = os.path.join(root, cited.replace("/", os.sep))
            if not os.path.isfile(full):
                errors.append("%s: %s is not on disk" % (path, cited))
    return errors


def changed_entries(sha):
    text = git("diff-tree", "--no-commit-id", "--name-status", "-r", sha)
    entries = []
    for line in text.splitlines():
        if not line:
            continue
        parts = line.split("\t")
        entries.append((parts[0], parts[-1]))
    return entries


def load_json(text, path, errors):
    try:
        return json.loads(text)
    except json.JSONDecodeError as exc:
        errors.append("%s: %s is not JSON (%s)" % (path, path, exc))
        return None


def validate_checkout():
    errors = []
    root = repo_root()
    directory = os.path.join(root, NOTES_DIR.replace("/", os.sep))
    if not os.path.isdir(directory):
        return ["%s is missing" % NOTES_DIR]
    for name in sorted(os.listdir(directory)):
        if not name.endswith(".json"):
            continue
        path = "%s/%s" % (NOTES_DIR, name)
        name_error = note_name_error("checkout", path)
        if name_error:
            errors.append(name_error)
            continue
        full = os.path.join(directory, name)
        with open(full, "r", encoding="utf-8") as handle:
            record = load_json(handle.read(), path, errors)
        if record is not None:
            errors.extend(validate_record(record, path))
    return errors


def note_exists(rel, end):
    full = os.path.join(repo_root(), rel.replace("/", os.sep))
    if os.path.isfile(full):
        return True
    return git_exists("cat-file", "-e", "%s:%s" % (end, rel))


def backfill_present(sha, end):
    return note_exists("%s/%s.json" % (NOTES_DIR, sha), end)


def range_end(spec):
    if "..." in spec:
        right = spec.split("...")[-1]
    elif ".." in spec:
        right = spec.split("..")[-1]
    else:
        right = spec
    if not right:
        right = "HEAD"
    return git("rev-parse", "--verify", right).strip()


def check_commit(sha, end):
    entries = changed_entries(sha)
    errors = []
    for status, path in entries:
        if not path.startswith(NOTES_DIR + "/"):
            continue
        if not status or status[0] not in ("A", "M", "R", "C"):
            continue
        name_error = note_name_error(sha, path)
        if name_error:
            errors.append(name_error)

    if not any(is_scoped(path) for _status, path in entries):
        return errors

    date_files = []
    for status, path in entries:
        if status.startswith("A") and DATE_FILE.match(path):
            date_files.append(path)
    if any(note_exists(path, end) for path in date_files):
        return errors
    if backfill_present(sha, end):
        return errors
    errors.append(
        "%s: a commit that changes boot, the kernel, the HAL, a driver, NTFS, "
        "or a Windows-facing DLL adds one veros/transparency/<yyyy-mm-dd>-<slug>.json"
        % sha
    )
    return errors


def main(argv):
    if len(argv) == 2 and argv[1] == "--checkout":
        errors = validate_checkout()
    elif len(argv) == 2:
        end = range_end(argv[1])
        errors = validate_checkout()
        listed = git("rev-list", "--reverse", "--no-merges", argv[1])
        for sha in listed.splitlines():
            if sha:
                errors.extend(check_commit(sha, end))
    else:
        print("usage: check_records.py <revision-range>|--checkout", file=sys.stderr)
        return 2
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
