# NeoNyx — C++ Edition 6.0 (Lunexxy)

Native Windows Dynamic Island-style launcher. The app starts as a compact black pill centered at the top of the primary display. Click the pill to expand the full NeoNyx panel; use “− Свернуть” to return to the compact state. The expand/collapse transition is animated.

## Included

- Borderless, always-on-top compact island window.
- Animated expansion and collapse, centered at the top of the screen.
- Live clock in the compact island and date/time + RAM usage in the expanded panel.
- Existing native launcher: add, search, launch, edit, remove and reorder EXE shortcuts.
- Application categories and saved settings.
- Password Vault encrypted with Windows DPAPI (`CryptProtectData`).
- Dark and light themes for the expanded panel.
- Native Win32 application; no Python runtime is required.
- Installer script for Inno Setup 6.

## Build on Windows

Requirements: Windows 10/11 x64, Visual Studio 2022 Build Tools with “Desktop development with C++”, and Inno Setup 6 for the installer.

1. Open “x64 Native Tools Command Prompt for VS 2022”.
2. Run `build_cpp.bat`.
3. Run `build_installer.bat` to create `dist\NeoNyx_Setup.exe`.

## Build in GitHub Actions (no local Visual Studio required)

The workflow at `.github/workflows/build-windows.yml` builds the EXE and installer on a GitHub-hosted Windows runner. Push this project to a GitHub repository and run the “Build NeoNyx for Windows” workflow from the Actions tab. The generated ZIP is published as a workflow artifact.

## Data and password vault

Launcher settings are stored in `%APPDATA%\NeoNyx`. The password vault is encrypted with Windows DPAPI and tied to the current Windows user profile on this PC. It is not a cloud-sync or cross-device password manager. Clipboard contents can be read by other apps in the same Windows session.

## Branding

Created by Lunexxy.
