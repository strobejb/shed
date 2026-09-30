# Shed

Shed is a Win32 two-player card game for Windows. Its based on a cardgame I can barely
remember playing as a student.

This repository contains the original 2001 C/C++ code updated to build with
Visual Studio 2022.

## Projects

- `Shed.sln` - Visual Studio 2022 solution.
- `Shed.vcxproj` - Win32 desktop application.
- `CardLib/CardLib.vcxproj` - card window/region support library.
- `Cards/Cards.vcxproj` - static replacement for the old Windows `cards.dll`
  API used by CardLib.
- `Catch22Lib/` - small compatibility copy of the helper code used by Shed.

## Building

Open `Shed.sln` in Visual Studio 2022 and build either `Debug|Win32` or
`Release|Win32`.

The command-line equivalent is:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' Shed.sln /m /p:Configuration=Release /p:Platform=Win32
```

The converted projects intentionally preserve some legacy settings, including
Multi-Byte character set and `/Zc:forScope-`, because the original VC6-era
source depends on them.

## Cards.dll Replacement

Modern Windows no longer ships `cards.dll`. The `Cards` project provides a
static implementation of the old API used by this game:

- `cdtInit`
- `cdtDraw`
- `cdtDrawExt`
- `cdtAnimate`
- `cdtTerm`

`cdtAnimate` is currently a compatibility stub because Shed does not call it.

## Repository Notes

The original Visual Studio 6 workspace/project files and obsolete `cards16`
import library files were removed from the cleaned source tree. See
`VS2022_CONVERSION_NOTES.md` for details about the conversion.

Build products, Visual Studio user files, and generated caches are ignored via
`.gitignore`.

## License

MIT. See `LICENSE`.
