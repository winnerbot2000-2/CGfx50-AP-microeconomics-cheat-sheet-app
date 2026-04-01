# CG Micro

AP Microeconomics study app for the Casio fx-CG50.

## Quick Summary

This is a quick native study app for the Casio fx-CG50.

It includes:

- AP Micro unit notes
- vocabulary pages
- graph reference pages
- formula cards
- quick review sheets
- exam cram pages

If you want a calculator study app with fast lookups, graph reminders, and compact review pages, this is the AP Micro one.

## At A Glance

- Platform: Casio fx-CG50
- Format: native `fxsdk/gint` add-in
- Output file: `cgmicro.g3a`
- Focus: AP Microeconomics review and reference

## Full README

### What It Does

This project builds a native Casio `fxsdk/gint` add-in for the fx-CG50 focused on AP Microeconomics review.

Main sections in the app:

- Units
- Concepts
- Vocabulary
- Graphs
- Formulas
- Structures
- Quick Review
- Exam Cram
- Reference
- Recent

### Main Features

- 6 AP Micro units with linked topic pages
- 20 topic summaries
- 48 vocabulary entries with short and full explanations
- graph-linked vocabulary pages with mini graph support
- 12 concept pages with mini graphs and related links
- 18 graph reference pages with labels, shifts, reading guides, and common mistakes
- 15 formula cards with built-in helper tools where useful
- market structure comparison pages
- quick review, exam cram, and reference sheets

### Project Layout

```text
casio-fx-cg50-apmicro/
  assets-cg/                  fx-CG50 icon assets
  generated/
    apmicro_content.json      intermediate content data
  src/
    app.h
    main.c
    ui.c
    graphs.c
    generated/
      apmicro_content.c
      apmicro_content.h
  tools/
    manual_content.py
    extract_apmicro_content.py
    generate_icons.py
    validate_content.py
    run_smoke_tests.py
    start_preview.py
  CMakeLists.txt
  build.ps1
  USER_GUIDE.md
```

### Build Requirements

You need a working fxSDK/gint toolchain to build the add-in:

- `fxsdk`
- `cmake`
- `sh-elf-gcc`
- `make` or Ninja
- `gint`

### Build

Regenerate content and icons:

```powershell
$env:APMICRO_SOURCE_ROOT = 'D:\path\to\desktop-source-repo'
python tools\extract_apmicro_content.py
python tools\generate_icons.py
```

Build the add-in:

```powershell
fxsdk build-cg
```

Or run the helper script:

```powershell
.\build.ps1
```

Expected output:

```text
cgmicro.g3a
```

### Install On Calculator

1. Build `cgmicro.g3a`.
2. Connect the fx-CG50 in USB storage mode.
3. Copy `cgmicro.g3a` to calculator storage.
4. Safely eject the calculator.
5. Launch the add-in from the main menu.

### Content Source Notes

The content pipeline can reuse structured AP Micro source material from:

- `apps/api/.data/apmicro-lemon.db`
- `lemon-microeconomics/**/*.pptx`
- `lemon-microeconomics/**/*.docx`
- `apps/web/components/graphs/*-module.tsx`

To regenerate content after source changes:

```powershell
$env:APMICRO_SOURCE_ROOT = 'D:\path\to\desktop-source-repo'
python tools\extract_apmicro_content.py
```

That refreshes:

- `generated/apmicro_content.json`
- `src/generated/apmicro_content.h`
- `src/generated/apmicro_content.c`
