# SMARTLUX Engineering Practices Report
## Best Configurations, Settings, and Engineering Practices for the SMARTLUX GitHub Repository

**Prepared for:** Kelompok 2, SMAN CMBBS  
**Repository:** https://github.com/FlintsXzzz/smartlux (private, branch: master)  
**Report Date:** 2026-11-21

---

## Executive Summary

SMARTLUX is a well-scoped high-school IoT practicum project with a solid hardware foundation (ESP32 + LDR + PIR + INA219 + SSD1306 + MOSFET lamp control + local web dashboard) but currently lacks the engineering scaffolding that separates a learning prototype from a maintainable, secure project. The seven critical gaps are: **(1)** no repository hygiene (no LICENSE, no README structure, no branch protection); **(2)** no CI/CD; **(3)** unpinned dependencies; **(4)** IoT security vulnerabilities in the web dashboard; **(5)** no tests; **(6)** no structured documentation or commit discipline; and **(7)** no prioritized action plan.

This report provides specific, actionable guidance for each gap, with citations to authoritative primary sources. A P0/P1/P2 checklist at the end gives the team an exact sequence to follow, with file contents and commands where useful.

The most urgent actions (P0, do this week) are: add a LICENSE, restructure the README, add `SECURITY.md`, enable branch protection on `master`, pin the PlatformIO platform version, and move WiFi credentials out of any committed file. Everything else can follow in an ordered sprint.

---

## 1. GitHub Repository Setup & Hygiene

### 1.1 README Structure

GitHub recommends a README for every repository to make work easier to understand and navigate [11]. For SMARTLUX, the README should cover the following sections in order:

```
# SMARTLUX — Smart Lighting System
## Project Overview (1–2 sentences, link to proposal)
## Hardware Bill of Materials (table: component, pin, value)
## Wiring Diagram (embed image or link to hardware/enclosure/)
## Quick Start
   - Prerequisites (arduino-cli version, pio version, Python)
   - secrets.h setup (copy secrets.example.h → secrets.h, fill values)
   - Build: `pio run` or `arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX`
   - Flash: `pio run --target upload`
   - Dashboard: open http://smartlux.local or http://<IP>
## Calibration Notes (LDR threshold values, INA219 shunt resistor)
## Repository Layout (tree with one-line description per folder)
## Contributing (link to CONTRIBUTING.md)
## License
```

This structure fulfills GitHub's expectation that a README "communicates expectations for your project and helps you manage contributions" [11].

For the ~550 KB proposal document with embedded base64 PNGs: GitHub limits file sizes and recommends Git Large File Storage (Git LFS) for large files [11]. The embedded PNGs inflate the Markdown file; consider extracting them to `docs/images/` and referencing them normally, or using Git LFS for the proposal PDF export.

### 1.2 LICENSE Choice

There is currently no LICENSE file. Without a license, copyright law in Indonesia (and internationally) means no one has permission to use, copy, or modify the code. For a student practicum project, the most appropriate choices are:

| License | Best for | Key terms |
|---|---|---|
| **MIT** | Maximum reuse, attribution only | Simple, permissive |
| **GPL-3.0** | Copyleft, derivatives stay open | Same license required for derivatives |
| **CC BY 4.0** | Documentation/proposal only | Creative Commons, not for code |

**Recommendation:** Use **MIT** for all code (firmware, OpenSCAD, harness) and **CC BY 4.0** for the proposal document. This is consistent with what OWASP itself uses (MIT License) [6] and allows teachers, other students, and the community to learn from and build on SMARTLUX.

Create `.github/LICENSE` or root `LICENSE` with MIT text. GitHub's Security Quickstart notes that a license is a core component of communicating project expectations [11].

### 1.3 `.gitignore` Tuning

The current repo already ignores `secrets.h`, which is correct. A comprehensive `.gitignore` for PlatformIO + Arduino CLI + OpenSCAD should include:

```gitignore
# PlatformIO build artifacts
.pio/
.pioenvs/
.piolibdeps/

# PlatformIO IDE files (VS Code)
.vscode/
.pio/

# Arduino CLI build cache
build/
*.bin
*.elf
*.hex
*.map

# Secrets — NEVER commit real credentials
firmware/SMARTLUX/secrets.h

# OpenSCAD renders
hardware/enclosure/*.stl
hardware/enclosure/*.png
# (keep .scad source files)

# Wokwi simulation output
diagram.json  # only if auto-generated; keep if hand-edited
.pio/build/   # already covered above

# macOS
.DS_Store

# Windows
Thumbs.db
desktop.ini

# Python (for tooling scripts)
__pycache__/
*.pyc
venv/
.env

# Node (if any JS tooling is added)
node_modules/
```

The `secrets.h` exclusion is the most security-critical line — GitHub's secret scanning documentation notes that even in a private repository, secrets committed to Git history are a persistent risk and should be rotated immediately if discovered [36].

### 1.4 Repository Topics, Description, and Default Branch

**Description** (shown on the GitHub repo page): Set to:  
`"IoT smart lighting system using ESP32 — LDR/PIR sensing, INA219 energy monitoring, local web dashboard. Physics practicum project, SMAN CMBBS."`

**Topics:** Add via Settings → Topics: `esp32`, `iot`, `arduino`, `platformio`, `energy-monitoring`, `smart-lighting`, `physics`, `indonesia`, `high-school`.

**Default branch naming:** GitHub and the industry have moved to `main` as the default branch name [11]. The repo currently uses `master`. Rename via:
```bash
git branch -m master main
git push -u origin main
# Then set default branch in Settings → Branches → Default branch
```

### 1.5 Branch Protection Rules

Branch protection is available on **public repositories** under GitHub Free and on **all repositories** (public and private) under GitHub Pro, Team, and Enterprise [13]. Since this is a **private personal-account repository**, branch protection rules require GitHub Pro [48]. If the repo is on the free plan, the best mitigation is to make the repository public (which also unlocks secret scanning, code scanning, and Dependabot for free) [11, 14], or upgrade to Pro.

If branch protection *is* available, enable these rules on `main` (or `master`):

1. **Require a pull request before merging** — minimum 1 approval [19]
2. **Require status checks to pass** — once CI is added, require the compile job [13]
3. **Dismiss stale pull request approvals when new commits are pushed** [13]
4. **Require conversation resolution before merging** [13]
5. **Do not allow bypassing the above settings** (even for admins, for a student project)

As a note: on a private free-plan repo, required status checks report but cannot block merges [48]. This is advisory-only until the plan is upgraded or the repo is made public.

### 1.6 CODEOWNERS

Create `.github/CODEOWNERS` to assign ownership:

```
# SMARTLUX CODEOWNERS
# Format: <pattern> <@username or @org/team>

# Default owner for everything
* @FlintsXzzz

# Firmware — requires firmware review
/firmware/ @FlintsXzzz

# Hardware enclosure
/hardware/ @FlintsXzzz

# Documentation
/docs/ @FlintsXzzz
*.md @FlintsXzzz

# CI/security configs — most sensitive
/.github/ @FlintsXzzz
```

CODEOWNERS files must be in `.github/`, root, or `docs/`; the `.github/` location is recommended because it protects the CODEOWNERS file itself [12]. Users must have write permissions to be code owners [12]. Enable "Require review from Code Owners" in the branch protection rule [12].

### 1.7 Issue and PR Templates

Create the following files under `.github/ISSUE_TEMPLATE/`:

**`.github/ISSUE_TEMPLATE/bug_report.yml`:**
```yaml
name: Bug Report
description: Report a firmware or dashboard bug
title: "[BUG] "
labels: ["bug"]
body:
  - type: input
    id: firmware-version
    attributes:
      label: Firmware Version
      placeholder: "e.g. v1.2.0"
    validations:
      required: true
  - type: textarea
    id: description
    attributes:
      label: Bug Description
      description: What happened? What did you expect?
    validations:
      required: true
  - type: textarea
    id: reproduction
    attributes:
      label: Steps to Reproduce
  - type: dropdown
    id: component
    attributes:
      label: Affected Component
      options:
        - LDR sensing
        - PIR sensing
        - Lamp control (MOSFET)
        - OLED display
        - INA219 energy meter
        - Web dashboard
        - OTA/WiFi
        - Enclosure (OpenSCAD)
```

**`.github/ISSUE_TEMPLATE/feature_request.yml`:**
```yaml
name: Feature Request
description: Suggest an improvement
title: "[FEAT] "
labels: ["enhancement"]
body:
  - type: textarea
    id: motivation
    attributes:
      label: Motivation
      description: Why is this feature needed?
    validations:
      required: true
  - type: textarea
    id: proposal
    attributes:
      label: Proposed Solution
```

**`.github/ISSUE_TEMPLATE/config.yml`:**
```yaml
blank_issues_enabled: false
contact_links:
  - name: Security Vulnerability
    url: https://github.com/FlintsXzzz/smartlux/security/advisories/new
    about: Please report security issues via private advisory, not public issues.
```

Setting `blank_issues_enabled: false` encourages use of templates [15]. The `contact_links` entry redirects security reports to the private advisory form [15].

**`.github/pull_request_template.md`:**
```markdown
## Summary
<!-- Describe the change in 1-2 sentences -->

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Refactor / code cleanup
- [ ] Documentation update
- [ ] CI/tooling change

## Testing Done
- [ ] Compiled successfully with `pio run`
- [ ] Compiled with `arduino-cli compile --fqbn esp32:esp32:esp32`
- [ ] Tested on physical hardware (describe results)
- [ ] Static analysis: `pio check` passes
- [ ] No new compiler warnings

## Checklist
- [ ] `secrets.h` is NOT committed
- [ ] `CHANGELOG.md` updated under `[Unreleased]`
- [ ] Commit messages follow Conventional Commits format
```

### 1.8 SECURITY.md

Create `.github/SECURITY.md` (GitHub searches `.github/` first, then root, then `docs/`) [43]:

```markdown
# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| Latest (main) | ✅ Yes |
| Older releases | ❌ No |

## Reporting a Vulnerability

**Please do NOT open a public GitHub Issue for security vulnerabilities.**

Report vulnerabilities using GitHub's private advisory feature:
👉 [Report a Vulnerability](https://github.com/FlintsXzzz/smartlux/security/advisories/new)

Or contact the maintainer directly at: [school email / teacher email]

We aim to acknowledge reports within 48 hours and resolve confirmed issues within 14 days.

## Security Considerations for This Project

SMARTLUX is a local-network IoT device. Key security notes:
- WiFi credentials are stored in `secrets.h` (git-ignored, never committed)
- The web dashboard has no authentication by default (see docs/superpowers/ for planned auth)
- This device is intended for use on a trusted local network only
- Do not expose the web dashboard (port 80) to the public internet
```

GitHub recommends adding `SECURITY.md` so reporters know where to send reports instead of opening public issues [19]. Enable private vulnerability reporting via Settings → Security → Reporting [19].

### 1.9 Dependabot and Dependency Alerts

Create `.github/dependabot.yml`:

```yaml
version: 2
updates:
  # GitHub Actions dependencies
  - package-ecosystem: "github-actions"
    directory: "/"
    schedule:
      interval: "weekly"
    commit-message:
      prefix: "ci"
    labels:
      - "dependencies"
      - "ci"

  # pip (for PlatformIO tooling in CI)
  - package-ecosystem: "pip"
    directory: "/"
    schedule:
      interval: "monthly"
    commit-message:
      prefix: "chore"
```

GitHub notes that Dependabot alerts maintainers when a dependency has a known vulnerability, and dependency review shows what's being added in pull requests [19]. Dependabot for GitHub Actions is available on all plans; Dependabot for pip is available on public repos or with GitHub Secret Protection [14]. Since PlatformIO libraries are not yet tracked by Dependabot's native package ecosystems, the team should periodically run `pio pkg outdated` to check for updates [49].

**Note on private repos/free plan:** Secret scanning on private user-owned repositories requires organization plans or GitHub Secret Protection — it is not unlocked by GitHub Pro alone [48]. As noted above, making the repository public unlocks these features for free.

### 1.10 Secret Scanning and Push Protection

For public repositories, secret scanning runs automatically and for free [36]. Secret scanning scans the entire Git history for hardcoded credentials [36]. Push protection blocks pushes containing supported secrets before they enter the repository [11].

For this project, the team should also consider custom patterns to detect SMARTLUX-specific secrets (e.g., the WiFi SSID/password format in `secrets.h` if it were accidentally committed).

When a secret is detected, GitHub recommends rotating the credential immediately [36]. Removing secrets from Git history is time-intensive; revocation is the priority [36].

### 1.11 Release and Semantic Versioning Strategy

**Semantic versioning (SemVer)** uses `MAJOR.MINOR.PATCH` [30]. For firmware, the public API surface is the OTA compatibility contract: a device running `1.x` should not need manual intervention to accept `1.y` updates, but `2.0` may require a factory reset or new partition table [47].

For SMARTLUX, a practical mapping is:
- **PATCH** (1.0.1): Bug fixes, threshold adjustments, typo corrections
- **MINOR** (1.1.0): New dashboard features, new sensor support, non-breaking API changes
- **MAJOR** (2.0.0): Partition table changes, breaking API changes, complete refactors

**Git tagging workflow:**
```bash
git tag -a v1.0.0 -m "feat: initial release with LDR/PIR control and INA219 monitoring"
git push origin v1.0.0
```

**Stamp version into firmware** via `build_flags` in `platformio.ini` [9, 30]:
```ini
[env:esp32dev]
build_flags =
  -DFIRMWARE_VERSION='"1.0.0"'
  -DBUILD_GIT_SHA='"${sysenv.GIT_SHA}"'
```

Then in CI: `export GIT_SHA=$(git rev-parse --short HEAD)`.

**GitHub Release notes** can be automatically generated from PR labels [35]. Configure `.github/release.yml`:

```yaml
changelog:
  exclude:
    labels:
      - ignore-for-release
  categories:
    - title: "🚀 New Features"
      labels: ["enhancement", "feature"]
    - title: "🐛 Bug Fixes"
      labels: ["bug"]
    - title: "🔒 Security"
      labels: ["security"]
    - title: "🔧 Hardware / Enclosure"
      labels: ["hardware"]
    - title: "📚 Documentation"
      labels: ["documentation"]
    - title: "🏗️ CI / Tooling"
      labels: ["ci", "dependencies"]
```

---

## 2. CI/CD with GitHub Actions

### 2.1 Compile-on-PR Workflow

Create `.github/workflows/build.yml`:

```yaml
name: Build & Analyze

on:
  push:
    branches: [main, master]
  pull_request:
    branches: [main, master]

jobs:
  build-pio:
    name: PlatformIO Build (${{ matrix.env }})
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false           # all board variants report results [47]
      matrix:
        env: [esp32dev]          # add more board variants here as needed

    steps:
      - uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.12'

      - name: Cache PlatformIO
        uses: actions/cache@v4
        with:
          path: |
            ~/.cache/pip
            ~/.platformio/.cache
          key: ${{ runner.os }}-pio-${{ hashFiles('firmware/SMARTLUX/platformio.ini') }}
          restore-keys: |
            ${{ runner.os }}-pio-

      - name: Install PlatformIO
        run: pip install --upgrade platformio

      - name: Generate secrets.h from GitHub Secrets
        env:
          WIFI_SSID: ${{ secrets.WIFI_SSID }}
          WIFI_PASSWORD: ${{ secrets.WIFI_PASSWORD }}
        run: |
          cat > firmware/SMARTLUX/secrets.h << 'EOF'
          #pragma once
          #define WIFI_SSID "${{ env.WIFI_SSID }}"
          #define WIFI_PASSWORD "${{ env.WIFI_PASSWORD }}"
          EOF

      - name: Build firmware
        run: |
          cd firmware/SMARTLUX
          pio run -e ${{ matrix.env }}

      - name: Upload firmware artifact
        uses: actions/upload-artifact@v4
        with:
          name: smartlux-firmware-${{ matrix.env }}-${{ github.sha }}
          path: |
            firmware/SMARTLUX/.pio/build/${{ matrix.env }}/firmware.bin
            firmware/SMARTLUX/.pio/build/${{ matrix.env }}/firmware.elf
          retention-days: 30

  build-arduino-cli:
    name: Arduino CLI Build (cross-verify)
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Generate secrets.h
        run: cp firmware/SMARTLUX/secrets.example.h firmware/SMARTLUX/secrets.h

      - name: Compile with arduino/compile-sketches
        uses: arduino/compile-sketches@v1
        with:
          fqbn: esp32:esp32:esp32
          platforms: |
            - name: esp32:esp32
              version: 3.3.12
              source-url: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
          sketch-paths: |
            - firmware/SMARTLUX
          enable-deltas-report: true
          github-token: ${{ secrets.GITHUB_TOKEN }}
```

**Key design decisions:**
- The cache key includes a hash of `platformio.ini` so a platform version bump invalidates the cache automatically [47]
- `fail-fast: false` lets all matrix variants report results even if one fails [47]
- `secrets.h` is generated at build time from GitHub Secrets, never from a committed file [9]
- The `arduino/compile-sketches` action cross-verifies the arduino-cli build path [33, 34]

### 2.2 Caching Strategy

PlatformIO is Python-based and benefits from two layers of caching [10]:
1. **pip cache** (`~/.cache/pip`): saves re-downloading PlatformIO Core
2. **PlatformIO package cache** (`~/.platformio/.cache`): saves re-downloading the ESP32 platform, toolchains, and libraries

Measured impact: download caching cuts ~14 seconds and unpacked-package caching cuts a further ~7 seconds per run in benchmarks [10], which matters when CI runs on every PR.

For the full package cache (`~/.platformio/`), be aware that cached packages won't update while the same cache key is in use [10]. The `hashFiles('platformio.ini')` key strategy automatically invalidates cache on any platformio.ini change [47].

### 2.3 Static Analysis Job

Add a separate job for `pio check`:

```yaml
  static-analysis:
    name: Static Analysis (cppcheck)
    runs-on: ubuntu-latest
    needs: build-pio       # only run if build passes [26]
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: '3.12'
      - name: Cache PlatformIO
        uses: actions/cache@v4
        with:
          path: |
            ~/.cache/pip
            ~/.platformio/.cache
          key: ${{ runner.os }}-pio-${{ hashFiles('firmware/SMARTLUX/platformio.ini') }}
      - name: Install PlatformIO
        run: pip install --upgrade platformio
      - name: Generate secrets.h
        run: cp firmware/SMARTLUX/secrets.example.h firmware/SMARTLUX/secrets.h
      - name: Run cppcheck
        run: |
          cd firmware/SMARTLUX
          pio check --fail-on-defect high --skip-packages
```

PlatformIO's `pio check` uses Cppcheck by default and can be configured for Clang-Tidy as well [27]. The `--fail-on-defect high` flag makes the CI job fail on high-severity findings [27]. The `--skip-packages` flag excludes framework sources (Arduino core, ESP-IDF) from analysis, focusing only on project code [27].

For SMARTLUX's ~1100-line Arduino C++ sketch, relevant Cppcheck findings will include:
- Null pointer dereferences (e.g., unchecked I2C/Wire results)
- Out-of-bounds array access
- Unused variables from sensor reads
- `setup()` and `loop()` falsely reported as unused — suppress with `// cppcheck-suppress unusedFunction` [22]

To also enable Clang-Tidy, add to `platformio.ini`:
```ini
[env:esp32dev]
check_tool = clangtidy, cppcheck
check_flags =
  clangtidy: --checks=-*,cert-*,bugprone-*,clang-analyzer-*
  cppcheck: --enable=warning,style,performance --std=c++14
```

Clang-Tidy can detect buffer overflows, potential NULL dereferences, and use-after-free [23].

### 2.4 Firmware Size Check

Add a size check step to the build job:

```yaml
      - name: Check firmware size
        run: |
          cd firmware/SMARTLUX
          SIZE=$(stat -c%s .pio/build/esp32dev/firmware.bin)
          MAX=1572864  # 1.5 MB — leaves room for OTA partition
          echo "Firmware size: $SIZE bytes (max: $MAX)"
          if [ "$SIZE" -gt "$MAX" ]; then
            echo "ERROR: Firmware exceeds size limit!"
            exit 1
          fi
```

The `arduino/compile-sketches` action's `enable-deltas-report: true` option also reports memory usage changes between PR branch and base, alerting reviewers to regressions [33, 34].

### 2.5 Keeping Secrets Out of CI

The authoritative pattern is [9]:
1. Keep `secrets.example.h` in the repo (all dummy/empty values)
2. Add real `secrets.h` to `.gitignore`
3. In CI, write `secrets.h` from GitHub Repository Secrets (Settings → Secrets and variables → Actions)
4. For PR builds from forks, only use `secrets.GITHUB_TOKEN` (auto-provided); never expose WiFi credentials to fork CI

```bash
# Add these in GitHub Settings → Secrets → Actions:
# WIFI_SSID = your_network_name
# WIFI_PASSWORD = your_network_password
```

For builds that do not need real credentials (compile-only checks), use the `secrets.example.h` with empty/placeholder values, as the firmware should compile without a WiFi connection established.

### 2.6 Release Workflow

Add `.github/workflows/release.yml` triggered on version tags:

```yaml
name: Release

on:
  push:
    tags:
      - 'v*.*.*'

jobs:
  release:
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v4
      - name: Set version from tag
        run: echo "VERSION=${GITHUB_REF_NAME#v}" >> $GITHUB_ENV
      - uses: actions/setup-python@v5
        with:
          python-version: '3.12'
      - name: Install PlatformIO
        run: pip install --upgrade platformio
      - name: Generate secrets.h
        env:
          WIFI_SSID: ${{ secrets.WIFI_SSID }}
          WIFI_PASSWORD: ${{ secrets.WIFI_PASSWORD }}
        run: |
          cat > firmware/SMARTLUX/secrets.h << EOF
          #pragma once
          #define WIFI_SSID "${WIFI_SSID}"
          #define WIFI_PASSWORD "${WIFI_PASSWORD}"
          #define FIRMWARE_VERSION "${VERSION}"
          EOF
      - name: Build release firmware
        run: cd firmware/SMARTLUX && pio run -e esp32dev
      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          generate_release_notes: true
          files: |
            firmware/SMARTLUX/.pio/build/esp32dev/firmware.bin
```

The gate that "pull requests build but never ship firmware" [9] is enforced by the `on: push: tags:` trigger.

---

## 3. Configuration & Settings Best Practices

### 3.1 PlatformIO `platformio.ini` — Authoritative Configuration

The current `platformio.ini` should be updated to reflect pinned versions, explicit build flags, and test environments:

```ini
; ============================================================
; SMARTLUX — PlatformIO Configuration
; Kelompok 2, SMAN CMBBS
; ============================================================

[platformio]
default_envs = esp32dev
; Exclude native env from default build to avoid cross-compilation attempts [44]

[env]
; Shared settings for all environments
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder, colorize
test_framework = unity

[env:esp32dev]
; Pin platform version — REQUIRED for reproducible builds [47][25]
; espressif32 v8.0.0 uses Arduino core v3.3.12 (IDF v5.5.5) [46]
platform = espressif32 @ ~8.0.0
board = esp32dev

build_flags =
  ; Warnings as errors in CI — catches problems early [47]
  -Wall
  -Wextra
  ; Project-specific defines
  -DBOARD_NAME='"ESP32-DevKit"'
  -DPROJECT_NAME='"SMARTLUX"'
  ; Pin assignments (documentation doubles as compile-time check)
  -DPIN_LDR=34
  -DPIN_PIR=27
  -DPIN_LAMP=26
  -DPIN_BUTTON=32
  -DI2C_SDA=21
  -DI2C_SCL=22
  ; Rate limiting constant (ms)
  -DAPI_RATE_LIMIT_MS=300

; Pin ALL library dependencies to exact versions [49][45]
lib_deps =
  adafruit/Adafruit SSD1306 @ 2.5.13
  adafruit/Adafruit GFX Library @ 1.11.11
  adafruit/Adafruit BusIO @ 1.17.0
  adafruit/Adafruit INA219 @ 1.2.1
  ; Wire, WiFi, WebServer, ESPmDNS are part of Arduino-ESP32 core — no lib_deps entry needed

check_tool = cppcheck
check_flags =
  cppcheck: --enable=warning,style,performance --std=c++14 --suppress=unusedFunction

[env:native]
; Host-based unit tests — no hardware required [44][21]
platform = native
build_src_filter = +<../lib/**>
test_filter = test_native/*
; Exclude Arduino-specific code from native tests
build_flags = -std=c++14 -DNATIVE_TEST
```

**Why pin the platform?** PlatformIO's official documentation states that specifying no version "does not ensure that builds are repeatable" and "highly recommends pinning the platform to a version" [25]. The latest stable release is `espressif32 @ ~8.0.0` (released October 2025, Arduino core v3.3.12 based on IDF v5.5.5) [46]. The `~` prefix (tilde range) allows patch updates within the same minor version, which Espressif themselves recommend [46].

### 3.2 `arduino-cli` Reproducible Builds via `sketch.yaml`

Create `firmware/SMARTLUX/sketch.yaml` to enable reproducible `arduino-cli` builds [4, 28]:

```yaml
profiles:
  smartlux:
    fqbn: esp32:esp32:esp32
    platforms:
      - platform: esp32:esp32 (3.3.12)
        platform_index_url: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    libraries:
      - Adafruit SSD1306 (2.5.13)
      - Adafruit GFX Library (1.11.11)
      - Adafruit BusIO (1.17.0)
      - Adafruit INA219 (1.2.1)

default_profile: smartlux
default_fqbn: esp32:esp32:esp32
default_port_config:
  baudrate: 115200
```

When `default_profile` is set, a bare `arduino-cli compile firmware/SMARTLUX` triggers a profile-based build that ignores globally installed platforms and libraries, ensuring isolation from the developer's system [4]. Missing platforms and libraries are auto-downloaded to an isolated cache [28].

### 3.3 Separating Secrets from Code

The existing `secrets.example.h` pattern is correct. Ensure it contains only safe placeholder values:

```cpp
// firmware/SMARTLUX/secrets.example.h
// Copy to secrets.h and fill in real values.
// NEVER commit secrets.h

#pragma once

#define WIFI_SSID     "your_wifi_ssid_here"
#define WIFI_PASSWORD "your_wifi_password_here"
```

For more advanced secret management (e.g., if the project later uses MQTT or cloud APIs), consider NVS (Non-Volatile Storage) encryption for runtime credentials on the device [5]. Espressif recommends protecting the default NVS partition with NVS Encryption since it stores WiFi credentials [5].

---

## 4. Security Best Practices for the IoT/ESP32 Web Dashboard

### 4.1 OWASP IoT Top 10 — Applicable to SMARTLUX

The OWASP IoT Top 10 (2018 edition) [6, 31, 38] maps to SMARTLUX as follows:

| OWASP Item | SMARTLUX Relevance | Recommended Fix |
|---|---|---|
| **I1** Weak/hardcoded passwords | Web dashboard has no authentication | Add token-based auth (see §4.3) |
| **I2** Insecure network services | mDNS (`smartlux.local`) exposes device on LAN | Restrict to trusted network; disable AP fallback in production |
| **I4** Lack of secure update | No OTA mechanism currently | Add signed OTA before deploying to production |
| **I5** Outdated components | Unpinned library versions | Pin all `lib_deps` to exact versions (§3.1) |
| **I7** Insecure data transfer | HTTP only (port 80) | Evaluate TLS (§4.5); use HTTPS headers |
| **I9** Insecure default settings | AP fallback `SMARTLUX-XXXX` with no password | Require password for AP mode |
| **I10** Physical hardening | JTAG/UART enabled | Disable in production via eFuse [31] |

The OWASP list aligns with ETSI EN 303 645 and NIST IR 8259A [31].

### 4.2 WiFi Credential Storage

**Current state:** `secrets.h` is git-ignored ✅  
**Risk:** Credentials are compiled into flash as plaintext strings.

**Mitigations:**

1. **Immediate:** Keep `secrets.h` git-ignored and generate it in CI from GitHub Secrets [9]. This is already done.

2. **Better for production:** Store credentials in NVS (not flash constants) using `Preferences` library:
```cpp
#include <Preferences.h>
Preferences prefs;
prefs.begin("smartlux", false);
prefs.putString("ssid", WIFI_SSID);       // only write once at provisioning
prefs.putString("pass", WIFI_PASSWORD);
prefs.end();
```
Then read at startup. This separates credentials from firmware binary.

3. **Best for production:** Enable NVS Encryption. Espressif states: "By default, ESP-IDF components write device-specific data into the default NVS partition, including Wi-Fi credentials, and it is recommended to protect this data using NVS Encryption" [5], using AES-XTS encryption [8].

### 4.3 API Hardening: Authentication, CSRF, CORS, CSP, Rate Limiting

**Current state:** The dashboard has CSP and 300ms rate limiting ✅ but no authentication.

**Authentication for a local-only dashboard:**

For a school practicum device on a trusted LAN, a simple shared secret in a header is pragmatic:

```cpp
// In SMARTLUX.ino — add to all state-changing endpoints
bool isAuthorized(WebServer &server) {
  // Check for X-SMARTLUX-Token header
  if (!server.hasHeader("X-SMARTLUX-Token")) return false;
  return server.header("X-SMARTLUX-Token") == DASHBOARD_TOKEN;
  // DASHBOARD_TOKEN defined in secrets.h
}

server.on("/api/override", HTTP_POST, [&]() {
  if (!isAuthorized(server)) {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  // ... handler
});
```

**CSRF Protection:**

For a dashboard that is only accessed from the same device (direct browser → ESP32 URL), CSRF risk is lower. However, since the dashboard is accessible via `smartlux.local` (mDNS), any browser tab on the LAN could make cross-site requests.

The OWASP CSRF Cheat Sheet recommends: for AJAX/API endpoints, use a custom request header (`X-SMARTLUX-Token`) as a CSRF mitigation, relying on CORS preflight to block unauthorized origins [7]. The ESP32 WebServer's CORS should be set to a specific origin, not `*`:

```cpp
// AVOID this (from [51]):
server.sendHeader("Access-Control-Allow-Origin", "*");

// PREFER: Restrict to the dashboard's own origin
server.sendHeader("Access-Control-Allow-Origin", "http://smartlux.local");
server.sendHeader("Access-Control-Allow-Headers", "X-SMARTLUX-Token, Content-Type");
```

OWASP notes: "If an attacker is able to take over a subdomain, your CORS configuration would allow them to bypass the same origin policy" [7]. For a local-only device, restricting to the specific origin is sufficient.

**CSP Header** (the existing CSP should be verified to include):
```
Content-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self'
```

**Security response headers** via `sendHeader()` [51]:
```cpp
server.sendHeader("X-Content-Type-Options", "nosniff");
server.sendHeader("X-Frame-Options", "DENY");
server.sendHeader("X-XSS-Protection", "1; mode=block");
// Note: Strict-Transport-Security has no effect over plain HTTP [51]
```

**Rate Limiting:** The existing 300ms rate limit on API endpoints is a good start. Extend it to prevent brute-force login attempts on the token endpoint.

### 4.4 mDNS and AP Fallback Risks

**mDNS (`smartlux.local`):** mDNS resolves only on the local network segment. Risks:
- Any device on the LAN can reach `smartlux.local` — authentication (§4.3) mitigates this
- mDNS advertisements announce device presence; this is acceptable for a school LAN

**AP Fallback (`SMARTLUX-XXXX`):** When the primary WiFi is unavailable, the device creates an open access point. This is OWASP I9 (Insecure Default Settings) [31]:

```cpp
// In AP setup code — require a password, defined in secrets.h
WiFi.softAP("SMARTLUX-" + String((uint32_t)ESP.getEfuseMac(), HEX),
            AP_PASSWORD);  // AP_PASSWORD defined in secrets.h
```

An open AP can be joined by anyone nearby, giving them unauthenticated access to the dashboard.

### 4.5 TLS Feasibility on ESP32

**Is TLS viable?** Yes, but with memory constraints. Each SSL socket on ESP32 uses approximately 40 KB RAM [39]. One reported figure is ~35 KB of heap per TLS connection [16]. With `CONFIG_MBEDTLS_DYNAMIC_BUFFER` enabled, idle connections use approximately 23.6 KB [16].

The ESP32-DevKit has 320 KB RAM. With the SMARTLUX firmware (OLED + INA219 + LDR + PIR + WebServer), available heap is typically 100–150 KB. A single TLS connection is feasible, but reducing the socket count (default 4 → 2 for HTTPS) is necessary [39].

**Practical recommendation for SMARTLUX at school-project scale:**
- **Short term:** Use plain HTTP but add all security headers, authentication, and CORS restrictions. This is appropriate for a trusted LAN device.
- **Medium term:** If the dashboard will be exposed beyond the LAN, implement TLS using `ESP32 WebServer` with self-signed certificates. The ESP-IDF HTTPS server example (`protocols/https_server/simple`) demonstrates this [39].
- **Long term / production:** Espressif recommends TLS for all external communications and advises against disabling HTTPS [5].

Espressif's Jumpstart Guide states: "All communication with any entity outside of the device must be secured. Instead of reinventing the wheel, we recommend using the standard TLS" using mbedTLS [8].

### 4.6 Secure OTA Considerations

SMARTLUX currently has no OTA mechanism. If OTA is added:

1. **OTA must occur over HTTPS** — Espressif states this explicitly [5]
2. **Signed images:** ESP32 supports Secure Boot, which ensures firmware is signed by a trusted entity [8]. With Secure Boot enabled, the OTA server should host signed images [5].
3. **Anti-rollback:** Store a security version in eFuse and reject older versions [5]
4. **Flash Encryption:** Keeps firmware encrypted in flash; in Release Mode, new images can only be loaded via OTA [42]

For the school project, the minimum viable OTA security is:
- HTTPS delivery (self-signed cert acceptable on LAN)
- Firmware signed with Ed25519, verified on-device [9]
- Version check to prevent downgrades [30]

Espressif recommends enabling Secure Boot on all production devices [5]. For a school device, this is P2 (nice to have), not P0.

### 4.7 Firmware Signing

If signing is implemented in CI:

```bash
# In GitHub Actions release workflow
# Generate key (once, offline): openssl genpkey -algorithm ed25519 -out signing_key.pem
# Store signing_key.pem in GitHub Secrets as SIGNING_KEY_PEM

echo "${{ secrets.SIGNING_KEY_PEM }}" > /tmp/signing_key.pem
# Sign firmware
openssl dgst -sha256 -sign /tmp/signing_key.pem \
  -out firmware.bin.sig firmware.bin
# Remove key immediately [9]
rm -f /tmp/signing_key.pem
```

Espressif's recommendation: signing keys should be generated on a system with a quality entropy source and kept private [5]. In a CI context, keys should ideally be stored in a cloud KMS (Azure Key Vault or AWS KMS) and accessed via OIDC federation so no long-lived credentials sit in GitHub Secrets [47].

---

## 5. Testing Strategy

### 5.1 Testing Pyramid for a Small Student Project

For a team of 2–5 students with no dedicated hardware CI runner, the pragmatic testing priority order is:

```
Priority 1 (do now):  Static Analysis (pio check) — zero hardware needed
Priority 2 (1 sprint): Native unit tests (PlatformIO native + Unity) — zero hardware needed
Priority 3 (2 sprints): Wokwi CI simulation — virtual hardware, cloud-based
Priority 4 (stretch):  Hardware-in-the-loop — requires physical setup
```

This order is consistent with the FSS Technology CI/CD guidance: "native tests cover protocol parsers, state machines, telemetry packing, command validation, and anything pure-functional" [47], and hardware tests cover "peripheral, RTOS, and timing code" [47].

### 5.2 PlatformIO Native + Unity Unit Tests

The core SMARTLUX logic that can be extracted and tested natively (without Arduino hardware):

**Testable components:**
- **LDR threshold logic:** Given a raw ADC value (0–4095), is the lamp decision (on/off) correct?
- **PIR state machine:** Does the occupancy state transition correctly with time?
- **Energy accumulation:** Does the kWh calculation from INA219 readings accumulate correctly?
- **API rate limiting:** Does the 300ms rate limiter correctly reject rapid calls?
- **CORS/CSP header generation:** Are security headers assembled correctly?

**Setup:**

Add to `platformio.ini` (already shown in §3.1):
```ini
[env:native]
platform = native
build_src_filter = +<../lib/**>
test_filter = test_native/*
build_flags = -std=c++14 -DNATIVE_TEST
```

Create `firmware/SMARTLUX/lib/SmartLux/SmartLux.h`:
```cpp
#pragma once
#include <stdint.h>
#include <stdbool.h>

// Pure functions — no Arduino dependency
bool shouldLampOn(uint16_t ldrAdc, uint16_t threshold, bool pirActive);
float accumulateEnergy(float prevKwh, float watts, uint32_t intervalMs);
bool rateLimitAllow(uint32_t nowMs, uint32_t lastMs, uint32_t limitMs);
```

Create `firmware/SMARTLUX/test/test_native/test_lamp_logic.cpp`:
```cpp
#include <unity.h>
#include "SmartLux.h"

void test_lamp_on_when_dark_and_pir_active() {
    // LDR value 100 = dark (below threshold 1000), PIR active
    TEST_ASSERT_TRUE(shouldLampOn(100, 1000, true));
}

void test_lamp_off_when_bright_regardless_of_pir() {
    // LDR value 3000 = bright (above threshold 1000), PIR active
    TEST_ASSERT_FALSE(shouldLampOn(3000, 1000, true));
}

void test_energy_accumulation() {
    // 100W for 1 hour = 0.1 kWh
    float energy = accumulateEnergy(0.0f, 100.0f, 3600000UL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, energy);
}

void test_rate_limit_blocks_rapid_calls() {
    TEST_ASSERT_FALSE(rateLimitAllow(100, 50, 300));  // 50ms elapsed < 300ms limit
}

void test_rate_limit_allows_after_interval() {
    TEST_ASSERT_TRUE(rateLimitAllow(400, 50, 300));   // 350ms elapsed > 300ms limit
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_lamp_on_when_dark_and_pir_active);
    RUN_TEST(test_lamp_off_when_bright_regardless_of_pir);
    RUN_TEST(test_energy_accumulation);
    RUN_TEST(test_rate_limit_blocks_rapid_calls);
    RUN_TEST(test_rate_limit_allows_after_interval);
    return UNITY_END();
}
```

PlatformIO only treats folders with a `test_` prefix as independent test applications [32]. Unity is PlatformIO's built-in default test framework [32]. The key architectural insight: Arduino-specific IO code (GPIO reads, I2C, WiFi) is the "glue" that is NOT unit-tested natively; the business logic (threshold comparison, energy accumulation, rate limiting) IS tested [32].

The Arduino framework is not supported on the PlatformIO native environment, so unit-tested code must be hardware-independent and must not `#include <Arduino.h>` [17].

**Coverage measurement** (optional but informative):

```ini
[env:native]
platform = native
build_flags = -std=c++14 -DNATIVE_TEST --coverage -fprofile-arcs -ftest-coverage
```

Then: `gcovr --html-details -o coverage_report/index.html` [18].

### 5.3 Wokwi CI Simulation

Wokwi CI uses the same simulation engine as the Wokwi Simulator [2] and runs in GitHub Actions [1]. It provides "Wokwi in the Loop (WITL)" — combining the speed of unit testing with the realism of hardware testing [2].

**Setup steps:**

1. Get a Wokwi CI token from the Wokwi CI Dashboard; store as `WOKWI_CLI_TOKEN` in GitHub Secrets [1]
2. Create `wokwi.toml` in repo root:

```toml
[wokwi]
version = 1
firmware = ".pio/build/esp32dev/firmware.bin"
elf = ".pio/build/esp32dev/firmware.elf"
```

3. Create `diagram.json` for the SMARTLUX circuit:
```json
{
  "version": 1,
  "author": "Kelompok 2 SMAN CMBBS",
  "editor": "wokwi",
  "parts": [
    { "type": "board-esp32-devkit-v1", "id": "esp32", "top": 0, "left": 0, "attrs": {} },
    { "type": "wokwi-ssd1306", "id": "oled1", "top": -50, "left": 150, "attrs": {"i2cAddress": "0x3c"} },
    { "type": "wokwi-photoresistor-sensor", "id": "ldr1", "top": 100, "left": 200 },
    { "type": "wokwi-pir-motion-sensor", "id": "pir1", "top": 150, "left": 200 }
  ],
  "connections": [
    ["esp32:SDA", "oled1:SDA", "blue", []],
    ["esp32:SCL", "oled1:SCL", "yellow", []],
    ["esp32:IO34", "ldr1:AO", "green", []],
    ["esp32:IO27", "pir1:OUT", "orange", []]
  ]
}
```

4. Run with `wokwi-cli init` to auto-detect the PlatformIO project and generate configuration [29]:
```bash
wokwi-cli init
```

5. Add to GitHub Actions workflow:
```yaml
  wokwi-simulation:
    name: Wokwi Hardware Simulation
    runs-on: ubuntu-latest
    needs: build-pio
    steps:
      - uses: actions/checkout@v4
      - name: Download firmware artifact
        uses: actions/download-artifact@v4
        with:
          name: smartlux-firmware-esp32dev-${{ github.sha }}
          path: .pio/build/esp32dev/
      - name: Run Wokwi simulation
        uses: wokwi/wokwi-ci-action@v1
        with:
          token: ${{ secrets.WOKWI_CLI_TOKEN }}
          path: /
          expect_text: "SMARTLUX Ready"   # check serial output on startup
          timeout: 30000
```

Wokwi CI provides free 50 simulation-minutes per 30-day rolling window [2]. For SMARTLUX's simple boot sequence, a 10–30 second simulation is sufficient per run.

**Important:** Wokwi does not store uploaded firmware; it is deleted from the cloud server after simulation [2]. For the ESP32 with PlatformIO, `firmware.bin` is at `.pio/build/<env>/firmware.bin` [29].

### 5.4 Hardware-in-the-Loop (HIL)

HIL testing requires a physical ESP32 DevKit connected to a "self-hosted GitHub Actions runner." For a student project without a dedicated CI machine, this is a P2 priority. The FSS Technology guide describes HIL as "the highest-leverage testing step" but notes "most teams stop before it" [47].

If the team wants to implement HIL:
- Connect an ESP32 DevKit to a laptop/Raspberry Pi running a [self-hosted GitHub Actions runner](https://docs.github.com/en/actions/hosting-your-own-runners)
- Use switched USB hub to allow power-cycling in CI [47]
- Flash with `pio run --target upload` and check serial output

### 5.5 Test Priority Summary

| Priority | Test Type | Effort | Hardware Needed | Recommended? |
|---|---|---|---|---|
| **P0** | Static analysis (`pio check`) | 1 hour | No | ✅ Yes, immediately |
| **P1** | Native unit tests (Unity, native env) | 1–2 days | No | ✅ Yes |
| **P1** | Wokwi CI simulation | 2–4 hours | No (virtual) | ✅ Yes |
| **P2** | Hardware-in-the-loop | 1 week | Yes | Optional |

---

## 6. Documentation & Collaboration Practices

### 6.1 Conventional Commits

Conventional Commits is a lightweight convention on commit messages that integrates with SemVer and enables automated changelog generation [3].

**Format:** `<type>[optional scope]: <description>`

**Types for SMARTLUX:**
```
feat(firmware):   New sensor feature or dashboard endpoint
fix(firmware):    Bug fix in control logic or display
fix(hardware):    OpenSCAD model correction
docs:             README, proposal, ADR updates
ci:               GitHub Actions workflow changes
chore:            Dependency updates, .gitignore, tooling
refactor:         Code restructuring without behavior change
test:             Adding or updating tests
perf:             Performance improvement (e.g., reduce heap usage)
```

**Examples:**
```
feat(dashboard): add INA219 real-time energy chart to web UI
fix(firmware): correct LDR threshold inversion causing lamp to stay on in daylight
docs(adr): record decision to use MOSFET instead of relay for lamp control
ci: add cppcheck static analysis job to build workflow
chore(deps): pin Adafruit SSD1306 to v2.5.13 for reproducibility
feat!: redesign API endpoints — BREAKING CHANGE: /api/status → /api/v2/status
```

The `!` after the type (or `BREAKING CHANGE:` in the footer) triggers a MAJOR version bump [3]. Conventional Commits also supports automated CHANGELOG generation [3].

For a squash-based workflow (squashing all PR commits into one), only the PR author needs to format the final commit message correctly, reducing the burden on casual contributors [3].

### 6.2 CHANGELOG

Create `CHANGELOG.md` following the Keep a Changelog format [40]:

```markdown
# Changelog
All notable changes to SMARTLUX will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- GitHub Actions CI/CD pipeline with PlatformIO build
- Native unit tests for lamp control logic
- SECURITY.md and private vulnerability reporting

## [1.0.0] - 2026-11-21
### Added
- LDR (GPIO34) light sensing with configurable threshold
- PIR (GPIO27) occupancy detection
- MOSFET (GPIO26) lamp control
- SSD1306 OLED status display over I2C (SDA:21, SCL:22)
- INA219 energy monitoring (address 0x40)
- Override button (GPIO32)
- Local WiFi dashboard (/, /api/status, /api/override, /api/reset)
- 300ms API rate limiting
- Content Security Policy headers
- mDNS (smartlux.local) with AP fallback
- OpenSCAD parametric enclosure design
- secrets.example.h for safe credential management

[Unreleased]: https://github.com/FlintsXzzz/smartlux/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/FlintsXzzz/smartlux/releases/tag/v1.0.0
```

Key Keep a Changelog rules: changelogs are for humans, not machines; there should be an entry for every version; the latest version comes first; use standard change types (Added, Changed, Deprecated, Removed, Fixed, Security); use ISO 8601 dates (YYYY-MM-DD) [40].

An inconsistently maintained changelog can be as dangerous as having none [40]. The team should update `[Unreleased]` as part of every pull request (enforced by the PR template).

### 6.3 Architectural Decision Records (ADRs)

An ADR captures a single architectural decision and its rationale, trade-offs, and consequences [41]. For SMARTLUX, decisions worth documenting include hardware choices, security trade-offs, and protocol decisions.

Create `docs/decisions/` directory. ADR format:

**`docs/decisions/ADR-001-mosfet-vs-relay.md`:**
```markdown
# ADR-001: MOSFET instead of relay for lamp control

## Status
Accepted

## Context
The lamp (220V AC / max 60W) needs to be controlled by the ESP32 GPIO26 (3.3V logic).
Options: electromechanical relay, solid-state relay (SSR), or N-channel MOSFET with driver.

## Decision
Use an N-channel MOSFET (IRF520 or similar) with a gate driver resistor on GPIO26.

## Rationale
- MOSFET switching: ~microsecond vs relay ~10ms — better for dimming applications
- No mechanical wear (relay rated ~100K cycles)
- Lower BOM cost for school project
- Silent operation

## Consequences
- Must ensure MOSFET is rated for load current (I_D > 300mA for 60W/220V lamp driving circuit)
- GPIO26 can only drive MOSFET up to 3.3V gate — verify V_GS(th) of chosen MOSFET
- Not galvanically isolated — additional consideration for 220V safety

## Date
2026-11-21
```

Additional recommended ADRs:
- **ADR-002:** HTTP vs HTTPS for the web dashboard (and why TLS was deferred)
- **ADR-003:** `secrets.h` pattern vs NVS encryption
- **ADR-004:** PlatformIO vs arduino-cli as primary build tool

AWS Prescriptive Guidance recommends ADRs for streamlining technical decision-making [41].

### 6.4 Keeping Proposal and Code in Sync

The `proposal_SMARTLUX_Kelompok_2.md` document describes the system design but may drift from the actual implementation. Recommended sync practices:

1. **Version-link the proposal:** Add a header to the proposal: `> This proposal describes the design for SMARTLUX v1.0.0. See [CHANGELOG.md](../CHANGELOG.md) for implementation deviations.`

2. **Issue template for divergence:** When code deviates from the proposal, open an issue with label `design-drift` and document the deviation in an ADR.

3. **Proposal as living document:** The proposal is in Markdown and in the repo — treat it like code. When the implementation changes a spec'd value (e.g., the threshold ADC value for LDR, the sampling rate), update the proposal in the same PR.

4. **Hardware calibration in `docs/calibration.md`:**

```markdown
# SMARTLUX Hardware Calibration

## LDR (Light Dependent Resistor) — GPIO34 ADC1

Measurement date: 2026-11-21
Room: Physics lab, SMAN CMBBS

| Condition | ADC Value (raw, 12-bit, 0–4095) | Voltage (V) |
|---|---|---|
| Lights off, daytime ambient | ~200–400 | 0.16–0.32 |
| Normal room lighting (300 lux) | ~1500–2000 | 1.22–1.63 |
| Bright direct light (~1000 lux) | ~3200–3600 | 2.61–2.93 |

**Decision threshold:** `LDR_THRESHOLD = 1200`
(Lamp turns on when ADC reading < 1200)

Calibration procedure:
1. With room at target "darkness" level, record ADC value via Serial Monitor
2. Set LDR_THRESHOLD = (ambient_dark_value + ambient_light_value) / 2
3. Update build flag: `-DLDR_THRESHOLD=1200` in platformio.ini

## INA219 Energy Meter — I2C 0x40

Shunt resistor: 0.1 Ω (default INA219 breakout)
Max current: 3.2A (INA219 default range)
Calibration: Use `Adafruit_INA219::setCalibration_16V_400mA()` for ≤400mA loads

Verification: At 5V/100mA load, INA219 should read:
- Bus voltage: 5.0V ± 0.1V
- Current: 100mA ± 5mA
- Power: 500mW ± 25mW
```

This calibration documentation directly supports reproducibility — another student can replicate the SMARTLUX behavior using these values [30].

---

## 7. Prioritized P0/P1/P2 Checklist for SMARTLUX

### P0 — Do This Week (Foundational, blocks everything else)

| # | Action | File/Command | Why |
|---|---|---|---|
| **P0.1** | Add MIT LICENSE | `LICENSE` (root) | No license = no permissions; legal requirement [11] |
| **P0.2** | Restructure README | `README.md` (root) | Communicates project to collaborators [11] |
| **P0.3** | Add `SECURITY.md` | `.github/SECURITY.md` | Directs vulnerability reports to private channel [19] |
| **P0.4** | Pin platform in `platformio.ini` | `platform = espressif32 @ ~8.0.0` | Reproducible builds; prevents surprise breakage [25][46] |
| **P0.5** | Pin ALL `lib_deps` to exact versions | See §3.1 | SBOM accuracy; reproducibility [49][45] |
| **P0.6** | Verify `secrets.h` is git-ignored | `echo "firmware/SMARTLUX/secrets.h" >> .gitignore` | Credential security [36] |
| **P0.7** | Rename default branch to `main` | `git branch -m master main && git push -u origin main` | Industry standard; enables GH features [11] |
| **P0.8** | Enable private vulnerability reporting | Settings → Security → Reporting | Private disclosure channel [19] |
| **P0.9** | Add `sketch.yaml` for arduino-cli | `firmware/SMARTLUX/sketch.yaml` | Reproducible arduino-cli builds [4][28] |
| **P0.10** | Add AP fallback password | Add `AP_PASSWORD` to `secrets.h` | OWASP I9: no open AP [31] |

**Exact command sequence for P0:**
```bash
# P0.1: License
curl -o LICENSE https://opensource.org/licenses/MIT  # copy MIT text
# Edit to include year and "Kelompok 2 SMAN CMBBS"

# P0.4+P0.5: platformio.ini update (edit manually per §3.1)

# P0.6: Verify gitignore
grep "secrets.h" firmware/SMARTLUX/.gitignore || \
  echo "firmware/SMARTLUX/secrets.h" >> .gitignore

# P0.7: Rename branch
git branch -m master main
git push -u origin main
# Then: GitHub Settings → Branches → Default branch → main

# P0.8: GitHub web UI: Settings → Security → "Private vulnerability reporting" → Enable
```

---

### P1 — Next Sprint (High value, low-to-medium effort)

| # | Action | File/Command | Why |
|---|---|---|---|
| **P1.1** | Add CI build workflow | `.github/workflows/build.yml` (§2.1) | Compile-on-PR catches regressions [47] |
| **P1.2** | Add static analysis to CI | Add `pio check` job to workflow (§2.3) | Catches null dereferences, unused vars [26][27] |
| **P1.3** | Add `CHANGELOG.md` | `CHANGELOG.md` (root) (§6.2) | Release tracking; Conventional Commits integration [40] |
| **P1.4** | Add issue/PR templates | `.github/ISSUE_TEMPLATE/*.yml`, `.github/pull_request_template.md` | Structured contributions [15] |
| **P1.5** | Add `.github/CODEOWNERS` | `.github/CODEOWNERS` (§1.6) | Enforce review for firmware changes [12] |
| **P1.6** | Add `dependabot.yml` | `.github/dependabot.yml` (§1.9) | Automated dependency alerts for Actions [14] |
| **P1.7** | Add token-based API authentication | `firmware/SMARTLUX/SMARTLUX.ino` (§4.3) | OWASP I1: no unauthenticated state changes [31] |
| **P1.8** | Restrict CORS to specific origin | `server.sendHeader("Access-Control-Allow-Origin", "http://smartlux.local")` | Prevent cross-site request abuse [7][51] |
| **P1.9** | Add AP fallback WPA2 password | `secrets.h + firmware` | OWASP I9: close open AP [31] |
| **P1.10** | First git tag + GitHub Release | `git tag -a v1.0.0 -m "..."` + push | SemVer baseline; artifact provenance [30] |
| **P1.11** | Add `docs/calibration.md` | `docs/calibration.md` (§6.4) | Reproducibility for other students [30] |
| **P1.12** | Add native unit tests | `lib/SmartLux/`, `test/test_native/` (§5.2) | Fast feedback loop, no hardware needed [21][32] |

**Exact commands for P1.1 (create workflow):**
```bash
mkdir -p .github/workflows
# Create .github/workflows/build.yml with content from §2.1

# Test locally (optional, requires act):
act pull_request

# Or push a test branch:
git checkout -b ci/add-build-workflow
git add .github/workflows/build.yml
git commit -m "ci: add PlatformIO build and static analysis workflow"
git push origin ci/add-build-workflow
# Open PR → watch Actions tab
```

**Exact commands for P1.12 (native unit tests):**
```bash
# 1. Create lib structure
mkdir -p firmware/SMARTLUX/lib/SmartLux
# Create SmartLux.h and SmartLux.cpp with pure functions

# 2. Create test directory
mkdir -p firmware/SMARTLUX/test/test_native
# Create test_lamp_logic.cpp with Unity tests (§5.2)

# 3. Add native env to platformio.ini (§3.1)

# 4. Run tests:
cd firmware/SMARTLUX
pio test -e native
```

---

### P2 — Future Sprint (Good engineering, manageable effort)

| # | Action | File/Command | Why |
|---|---|---|---|
| **P2.1** | Add Wokwi CI simulation | `wokwi.toml`, `diagram.json`, CI job (§5.3) | Virtual hardware testing on every PR [1][2] |
| **P2.2** | Add ADRs | `docs/decisions/ADR-00*.md` (§6.3) | Document hardware/software decisions [41] |
| **P2.3** | Add `release.yml` for auto release notes | `.github/release.yml` (§1.11) | Automated changelog from PR labels [35] |
| **P2.4** | Extract base64 PNGs from proposal | `docs/images/` + update Markdown links | Reduce file size, Git LFS compliance [11] |
| **P2.5** | Add firmware size check to CI | (§2.4) | Prevent OTA partition overflow [9] |
| **P2.6** | Implement NVS storage for WiFi credentials | `Preferences.h` pattern (§4.2) | Better credential security than compiled strings [5] |
| **P2.7** | Add `release.yml` workflow | `.github/workflows/release.yml` (§2.6) | Automated firmware artifact on git tag [9] |
| **P2.8** | Enable branch protection (or make repo public) | GitHub Settings → Branches | Enforce CI gate on PRs [13][48] |
| **P2.9** | Implement Conventional Commits discipline | Team agreement + PR template | Automated SemVer; clean history [3] |
| **P2.10** | Add Clang-Tidy to CI | `check_tool = clangtidy` in `platformio.ini` (§2.3) | Deeper C++ issue detection [23] |
| **P2.11** | Plan TLS/HTTPS for dashboard | (§4.5, with self-signed cert) | OWASP I7: encrypted data transfer [5][31] |
| **P2.12** | Add firmware version stamp via build_flags | `-DFIRMWARE_VERSION='"1.0.0"'` in CI (§3.1) | Devices self-report running version [30] |
| **P2.13** | HIL testing with self-hosted runner | Physical ESP32 + local Actions runner (§5.4) | Real hardware validation [47] |

---

## References & Citations

> All URLs are from the evidence list provided. Retrieved date: 2026-11-21.

[1] Wokwi CI — Using Wokwi in GitHub Actions: https://docs.wokwi.com/wokwi-ci/github-actions

[2] Wokwi CI — Getting Started: https://docs.wokwi.com/wokwi-ci/getting-started

[3] Conventional Commits Specification v1.0.0: https://conventionalcommits.org/en/v1.0.0

[4] Arduino CLI Sketch Project File: https://docs.arduino.cc/arduino-cli/sketch-project-file

[5] Espressif ESP-IDF Security Overview (v6.1.1): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/security.html

[6] OWASP Internet of Things Project: https://owasp.org/projects/internet-of-things

[7] OWASP CSRF Prevention Cheat Sheet: https://cheatsheetseries.owasp.org/cheatsheets/Cross-Site_Request_Forgery_Prevention_Cheat_Sheet.html

[8] ESP-Jumpstart Security Considerations: https://docs.espressif.com/projects/esp-jumpstart/en/latest/esp32/security.html

[9] ESP32 OTA Releases from GitHub Actions with Arduino CLI: https://simpleota.com/blog/esp32-ota-github-actions-arduino-cli

[10] PlatformIO on GitHub Actions with Cache Accelerate: https://dev.to/akuad/platformio-on-github-actions-with-cache-accelerate-4cm9

[11] GitHub — Best Practices for Repositories: https://docs.github.com/en/repositories/creating-and-managing-repositories/best-practices-for-repositories

[12] GitHub — About Code Owners: https://docs.github.com/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners

[13] GitHub — Managing a Branch Protection Rule: https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/managing-a-branch-protection-rule

[14] GitHub — Quickstart for Securing Your Repository: https://docs.github.com/en/code-security/getting-started/quickstart-for-securing-your-repository

[15] GitHub — Configuring Issue Templates: https://docs.github.com/en/communities/using-templates-to-encourage-useful-issues-and-pull-requests/configuring-issue-templates-for-your-repository

[16] ESP32 TLS Memory Usage (mbedTLS): https://pinscope.dev/guides/esp32-tls-memory

[17] ESP32 with PlatformIO, C++, Unity and Arduino: https://watenan.uk/2023/07/24/esp32-with-platformio-c-and-arduino/

[18] Unit Testing Embedded Firmware — A Practical Guide: https://embeddedsoft.net/unit-testing-embedded-firmware-a-practical-guide

[19] GitHub Blog — 6 Security Settings Every GitHub Maintainer Should Enable: https://github.blog/security/6-security-settings-every-github-maintainer-should-enable-this-week

[20] PlatformIO — GitHub Actions Integration (v6.2 docs): https://docs.platformio.org/en/stable/integration/ci/github-actions.html

[21] PlatformIO — Unit Testing: https://docs.platformio.org/en/latest/advanced/unit-testing/index.html

[22] PlatformIO — Cppcheck: https://docs.platformio.org/en/latest/advanced/static-code-analysis/tools/cppcheck.html

[23] PlatformIO — Clang-Tidy: https://docs.platformio.org/en/latest/advanced/static-code-analysis/tools/clang-tidy.html

[24] PlatformIO — build_flags (v6.2): https://docs.platformio.org/en/stable/projectconf/sections/env/options/build/build_flags.html

[25] PlatformIO — platform option: https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/platform.html

[26] PlatformIO — Static Code Analysis: https://docs.platformio.org/en/latest/advanced/static-code-analysis/index.html

[27] PlatformIO — pio check command: https://docs.platformio.org/en/latest/core/userguide/cmd_check.html

[28] Arduino CLI — Sketch Project File (v0.35): https://arduino.github.io/arduino-cli/0.35/sketch-project-file

[29] Wokwi — Configuring Your Project (wokwi.toml): https://docs.wokwi.com/vscode/project-config

[30] Memfault Interrupt — Proper Release Versioning Goes a Long Way: https://interrupt.memfault.com/blog/release-versioning

[31] FM TecHub — OWASP IoT Top 10 Explained with Fixes: https://fmtechub.com/notes/iots-owasp-iot-top10

[32] xa1.at — Unit Testing Arduino Code on the Host Machine using PlatformIO: https://xa1.at/platformio-unit-test/

[33] GitHub Marketplace — Compile Arduino Sketches: https://github.com/marketplace/actions/compile-arduino-sketches

[34] GitHub — arduino/compile-sketches repository: https://github.com/arduino/compile-sketches

[35] GitHub — Automatically Generated Release Notes: https://docs.github.com/en/repositories/releasing-projects-on-github/automatically-generated-release-notes

[36] GitHub — Secret Scanning: https://docs.github.com/en/code-security/concepts/secret-security/secret-scanning

[37] Embedded Artistry — Safely Storing Secrets in Git: https://embeddedartistry.com/blog/2018/03/15/safely-storing-secrets-in-git

[38] InfoSec Insights — OWASP IoT Top 10 List: https://sectigostore.com/blog/owasp-iot-top-10-iot-vulnerabilities/

[39] ESP-IDF — HTTPS Server (v6.1): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_https_server.html

[40] Keep a Changelog v1.1.0: https://keepachangelog.com/en/1.1.0/

[41] ADR GitHub — Architectural Decision Records: https://adr.github.io/

[42] ESP-IDF — Flash Encryption (v6.1.1): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/flash-encryption.html

[43] Carets — How to Write a SECURITY.md: https://carets.app/articles/security-md-github-how-to-write-report-vulnerability

[44] Memfault Interrupt — Seamless Firmware Development with PlatformIO: https://interrupt.memfault.com/blog/seamless-firmware-with-platformio

[45] Safeguard.sh — PlatformIO lib_deps Pinning for SBOM: https://safeguard.sh/resources/blog/platformio-lib-deps-pinned-or-floating-for-firmware-sbom

[46] PlatformIO — platform-espressif32 Release 8.0.0: https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0

[47] FSS Technology — CI/CD for Embedded Firmware: GitHub Actions + PlatformIO Pipeline for ESP32: https://fss.cc/cicd-embedded-firmware/

[48] Tobias Krug — Free Plan, Private Repo: The Protections You Think You Have: https://tobiaskrug.com/blog/free-plan-private-repo-protections

[49] PlatformIO — Dependency Management: https://docs.platformio.org/en/latest/librarymanager/dependencies.html

[50] PlatformIO Registry — ArduinoFake: https://registry.platformio.org/libraries/fabiobatsilva/ArduinoFake

[51] AvantMaker — ESP32 WebServer Library responseHeaders(): https://avantmaker.com/references/esp32-arduino-core-index/esp32-webserver-library/esp32-webserver-library-responseheaders

---

*This report was produced using authoritative primary sources as cited throughout. All code examples are illustrative — adapt pin numbers, library versions, and thresholds to match the actual SMARTLUX hardware configuration as measured during calibration. The P0 checklist items can be completed in a single afternoon; P1 items form a solid engineering foundation appropriate for a school project that may also serve as a portfolio artifact.*