#!/usr/bin/python3
import sys
from pathlib import Path

version_file = Path("./version.txt")

if not version_file.exists():
    print("Error: version.txt not found.")
    sys.exit(1)

# Read the current version
current_version = version_file.read_text().strip()
major, minor, patch = map(int, current_version.split("."))

# Increment the patch version
patch += 1
new_version = f"{major}.{minor}.{patch}"

# Write the new version back to the file
version_file.write_text(new_version)
print(new_version)
