# Local Automation Patches

This fork of Windows Terminal exists to support trusted local automation workflows on the maintainer's own machine. It is not positioned as an upstream-approved security model and it is not intended to be a general remote-control distribution of Windows Terminal.

## Why This Fork Exists

Stock Windows Terminal does not provide a public command-line surface for sending text into an already running terminal window. This fork adds that missing control surface so local helper tools can:

- send text into an existing Windows Terminal window
- optionally press Enter after the text
- delay Enter for TUIs like Codex that need a short settle gap
- target a specific existing WT window deterministically

This fork is currently used by local tools such as:

- `tools/codex-terminal-restart`
- `tools/codex-wakeup`

Those tools rely on this fork for input delivery while using separate state/intelligence layers for session tracking.

## What This Fork Adds

### `wt send-input`

New command-line action:

```powershell
wt.exe -w <target> send-input [--escape] [--enter] [--enter-delay-ms <ms>] [--activate] -- <text>
```

Added behavior:

- `send-input`
- `--enter`
- `--enter-delay-ms <ms>`
- `--activate`

Existing-window `send-input` stays in the background by default unless `--activate` is used.

### Explicit typed existing-window selectors

This fork also supports explicit typed selectors for `-w`:

- `hwnd:0x<HWND>`
- `id:<window-id>`
- `name:<window-name>`

These selectors are intended for automation callers that already know the exact WT target they want to address.

Explicit selectors fail closed if they do not resolve. They do not silently create a new WT window.

## Command Reference

### Primary command shape

```powershell
wt.exe -w <target> send-input [--escape] [--enter] [--enter-delay-ms <ms>] [--activate] -- <text>
```

### Parameters

- `send-input`
  - sends text to the active pane in the targeted command context
- `--escape`
  - treats the provided text as escaped sequences
  - useful for control characters such as `\u0015`
- `--enter`
  - sends Enter after the text
- `--enter-delay-ms <ms>`
  - delays Enter submission
  - primarily intended for TUIs like Codex that need a short gap between text and submit
- `--activate`
  - opts back into normal foreground activation behavior
  - existing-window `send-input` otherwise stays background by default
- `--`
  - ends option parsing so command text such as `/quit` is treated as literal input

### Target selectors

- `-w 0`
  - follows WT's normal "current existing window" behavior
  - convenient for quick local tests, but not the preferred automation selector
- `-w last`
  - WT's most recently used existing-window selector
- `-w <name>`
  - target a named WT window
- `-w <id>`
  - target WT's internal numeric window ID
- `-w hwnd:0x<HWND>`
  - preferred automation-grade selector when a helper already knows the exact WT handle
- `-w id:<window-id>`
  - explicit typed form of the WT numeric window ID
- `-w name:<window-name>`
  - explicit typed form of the WT window name

### Behavior notes

- explicit typed selectors fail closed if the target does not resolve
- explicit typed selectors are the preferred deterministic targeting surface for automation
- helper-side discovery still happens outside WT
- Codex-oriented workflows on this machine currently use `--enter-delay-ms 200`

## Examples

### Normal shell command

```powershell
wt.exe -w hwnd:0x123456 send-input --enter "echo READY"
```

### Codex prompt submission

```powershell
wt.exe -w hwnd:0x123456 send-input --enter --enter-delay-ms 200 "Please reply exactly with TEST_OK."
```

### Clean Codex quit

```powershell
wt.exe -w hwnd:0x123456 send-input --enter --enter-delay-ms 200 -- "/quit"
```

### Window-name targeting

```powershell
wt.exe -w name:codexdev send-input --enter "echo READY"
```

## Intended Use

This fork is intended for:

- trusted local automation
- personal-machine workflows
- helper tools that already identify the intended WT window explicitly

It is not intended as a blanket answer to upstream terminal automation for arbitrary local applications.

## Upstream Status

An upstream PR was opened to demonstrate the feature shape and implementation approach:

- PR: <https://github.com/microsoft/terminal/pull/20106>

That PR was closed without merge. The maintainer concern was the security model of allowing general input injection into running Windows Terminal instances.

This fork remains valuable locally because:

- it proves the workflow
- it provides a concrete command surface
- it supports real automation used by the maintainer's tools

## Building This Fork

This repo does not ship binaries from the fork itself. Build locally.

### Prerequisites

Use the normal upstream Windows Terminal prerequisites described in [README.md](./README.md), especially:

- Visual Studio with the required workloads/components
- the Windows SDK expected by the repo
- any repo bootstrap prerequisites documented upstream

### Build the Windows Terminal package

Example from this fork's current working flow:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' `
  'C:\git\sandbox\projects\windows-terminal\OpenConsole.slnx' `
  /p:Configuration=Release `
  /p:Platform=x64 `
  /p:WindowsTerminalBranding=Release `
  /p:AppxSymbolPackageEnabled=false `
  /t:Terminal\CascadiaPackage `
  /m `
  /nodeReuse:false
```

### Register the locally built package

```powershell
powershell.exe -NoProfile -Command "Add-AppxPackage -ForceApplicationShutdown -ForceUpdateFromAnyVersion -Register 'C:\git\sandbox\projects\windows-terminal\src\cascadia\CascadiaPackage\bin\x64\Release\AppxManifest.xml'"
```

After registration, `wt.exe` on that machine will use the locally registered build.

## Notes For Maintainers Of This Fork

- The automation-grade target identity currently used by local tools is `hwnd:0x...`.
- Helper-side discovery still happens outside WT; this fork focuses on deterministic write targeting.
- Codex-oriented tools on this machine currently use `--enter-delay-ms 200` as the safe default for prompt submission.
- For more detailed local notes, see `C:\git\sandbox\research\windows-terminal.md`.
