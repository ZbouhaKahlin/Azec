# Willow Voice 2.1.28 — reverse engineering report

Static analysis of a locally installed Windows build, performed on October 17, 2026 with the
`reverse-engineer-anything` (REA) skill. Scope: the shipped application artifacts only — the
ASAR archive, its unpacked native addons, the bundled helper executables, Electron fuses, and
the local data directory layout. No runtime execution, no debugger, no network capture, and no
attempt to bypass any licensing, authentication, or access control was performed.

This report separates **observations** (read directly from artifacts), **inferences**
(reasoning over observations), and **unknowns** (not determinable from the collected evidence).

---

## 1. Subject

| Item | Value |
| --- | --- |
| Product | Willow Voice 2.1.28 (Electron, Windows x64) |
| Package name | `willow-voice` |
| Publisher | Willow Care Inc. (Authenticode signer `CN=Willow Care Inc.`) |
| Homepage | `https://willowvoice.com/` |
| Install root | `%USERPROFILE%\AppData\Local\Programs\Willow Voice` |
| `Willow Voice.exe` | 235,318,992 bytes, SHA-256 `F2805958D27EEC4C7801C4067060ECABCCE4D66AEE95B638320E3DF062C8BFF5` |
| `resources\app.asar` | 42.8 MB, SHA-256 `800637CBBECFED3543880A4D92062BA1713A8DDB455A3B19FF007B1271870055` |

Bundled helper binaries (all signed by `CN=Willow Care Inc.`):

- `whisper-server.exe` — local Whisper (ggml/whisper.cpp) transcription server.
- `willow-audio-quality-vad.exe` — voice-activity detection helper.
- `elevate.exe` — UAC elevation helper.

Update channel (`resources\app-update.yml`): electron-updater S3 provider, bucket
`willow-electron-builds` (us-east-1), path `/windows-x64`.

Runtime dependencies observed in `package.json`: `@sentry/electron`, `@supabase/supabase-js`,
`@msgpack/msgpack`, `arktype`, `compromise`, `electron-log`, `electron-updater`,
`posthog-node`, `ws`. Sources are built with `oxfmt`/`oxlint` tooling.

---

## 2. Artifact integrity

### 2.1 ASAR hash mismatch (explained)

The ASAR header declares per-file SHA-256 digests. All six unpacked `.node` addons
(`accessibility`, `appinfo`, `audiocapture`, `keyboardmonitor`, `screenshotcapture`,
`taskbarinfo` under `out/native/build/Release/`) mismatched their declared hash — each by
exactly **+10,448 bytes**. Forensics showed the trailing bytes are a single PKCS#7
`WIN_CERTIFICATE` (dwLength=10448, wRevision=0x0200, wCertificateType=2) appended by
Authenticode signing after packaging. After truncating to the declared size and zeroing the
PE checksum and certificate-table data-directory entry, the reconstructed bytes hash
**exactly** to the declared ASAR digests. All six files validate as `Valid` signatures by
`CN=Willow Care Inc.`.

**Conclusion (inference):** the mismatch is fully explained by post-packaging code signing; it
is a benign pipeline artifact, not tampering of the installed copy.

### 2.2 Electron fuses (from `Willow Voice.exe`)

| Fuse | State |
| --- | --- |
| `RunAsNode` | Enabled |
| `EnableCookieEncryption` | Disabled |
| `EnableNodeOptionsEnvironmentVariable` | Enabled |
| `EnableNodeCliInspectArguments` | Enabled |
| `EnableEmbeddedAsarIntegrityValidation` | **Disabled** |
| `OnlyLoadAppFromAsar` | **Disabled** |
| `GrantFileProtocolExtraPrivileges` | Enabled |
| `WasmTrapHandlers` | Enabled |

**Observation:** the app does not enforce ASAR integrity validation and does not restrict app
code loading to the ASAR file. Modifications to application resources would not be rejected by
the fused runtime checks, and Node debugging options remain available. This is reported as a
supply-chain hardening observation only; no exploitation was attempted and no malware was
found.

---

## 3. Application architecture

### 3.1 Processes

- **Main process** — webpack-bundled modules at `out/main/`:
  `index.js` (76,689 B), `index-D0wgffWq.js` (63,638 B), `runtime-startup-BwwOnqbx.js`
  (426,313 B), plus `app-data-manager`, `auto-updater`, `current-user-state`,
  `proper-noun-extractor`, `session-transfer-api`, `transcript-cleanup`.
  Named services recovered from bundles include `RecordingManager`, `KeyboardMonitor`,
  `SessionManager`, `PermissionsService`, `TranscriptCleanup`, `AppDataManager`.
- **Preload** — `out/preload/preload.js` exposes `window.api` through `contextBridge`
  (`contextIsolation` semantics). Roughly **300 IPC channels** were enumerated from this file
  (dictation, auth, teams, transcripts, dictionary, knowledge, Scribe, notifications,
  tutorials, offline model, updates, and more).
- **Renderer** — React application at `out/renderer/assets/*.js` (feature module names
  recovered from chunk filenames: `Dictionary`, `Knowledge`, `ScribeOnboarding*`,
  `HotkeySettingsModal`, `SettingsModal`, `TeamMembers`, `MicSelectorModal`, etc.).
- **Native addons** (N-API `.node`): accessibility, app info, audio capture, keyboard monitor,
  screenshot capture, taskbar info.

### 3.2 Local helper processes (observed spawn configuration)

- `whisper-server.exe` with arguments including `-t <threads> -nt -sns` — local transcription
  service.
- `willow-audio-quality-vad.exe` — audio gating.
- Offline model management downloads the Silero VAD model
  `ggml-silero-v5.1.2.bin` from
  `https://huggingface.co/ggml-org/whisper-vad/resolve/main/` with a pinned SHA-256 constant
  (`29940d98d42b91fbd05ce489f3ecf7c72f0a42f027e4875…`).

### 3.3 Global hotkey mechanism

Global shortcuts are **not** implemented via Electron `globalShortcut`; a native low-level
hook in `keyboardmonitor.node` is used. The default binding constant observed in the bundle is
a paste-transcript chord; the reviewed local profile stores a user-modified
paste-transcript chord and a hands-free chord. Virtual-key codes observed: 162/160 (Ctrl/Shift),
80 (`P`), 76 (`L`).

---

## 4. Feature map (observations)

- **Dictation** — hold-to-talk and hands-free modes; audio capture → local Whisper →
  transcript cleanup (`transcript-cleanup`, `proper-noun-extractor`) → text pasted into the
  focused application. Settings observed: privacy mode, auto-duck audio, press-enter after
  dictation, preferred languages, style matching (per-category formal/casual tone).
- **Willow Scribe** — meeting transcription (WebSocket transport; `ws` dependency;
  Meet/Teams onboarding flows).
- **Screen context** — accessibility tree and screenshot capture; app/taskbar info used to
  identify the focused app. Target-app fingerprints observed for claude.ai, chat.openai.com,
  cursor.sh, mail.google.com, outlook.com, notion.so, docs.new, superhuman, icloud, onenote.
- **Dictionary / Glossary / Knowledge** — user-managed vocab and context stores.
- **Teams** — workspace membership, invites, team-level settings such as enforced privacy mode
  and transcript auto-deletion.
- **Account** — Supabase auth; session transfer via
  `https://app.willowvoice.com/auth/session-transfer`; PKCE verifiers stored locally.
- **Updates** — electron-updater from the S3 update bucket; forced-update modal exists.
- **Telemetry** — Sentry (`@sentry/electron`, ingest key `o4509174948626432…`) and PostHog
  (`us.i.posthog.com`).

### 4.1 Network endpoints (observed in bundles)

| Endpoint | Purpose (inferred) |
| --- | --- |
| `https://api.willowvoice.com/api` (+ `/api/v1`, `/api/v2`) | Primary product API |
| `https://db.willowvoice.com` | Supabase (database/auth) |
| `https://middleware.willowvoice.com` (+ `dev-middleware.`) | Middleware service |
| `https://app.willowvoice.com/auth/session-transfer` | Web-to-app session handoff |
| `https://us.i.posthog.com` | Product analytics |
| Sentry ingest (`o4509174948626432.ingest.us.sentry.io`) | Crash/error reporting |
| `willow-electron-builds.s3.us-east-1.amazonaws.com/windows-x64` | App updates |
| `huggingface.co/ggml-org/whisper-vad` | VAD model download |

### 4.2 Data at rest (structure only)

`%APPDATA%\willow-voice\` contains: `dictionary.json`, `electron-app-data.json`
(settings/hotkey bindings), `supabase-session.json` (tokens and PKCE verifiers),
`Recordings\*.aac`, `Transcripts\*.json`, `VADModel\ggml-silero-v5.1.2.bin`,
`RecordingRecovery\`, `sentry\`, and `logs\main.log` (rotating, verbose). No personal content
from these files is reproduced here.

---

## 5. REA analysis runs and coverage

Tool: `npx -y rea-agents@6.3.0 analyze-javascript-application` (the sanctioned CLI fallback;
the REA MCP server is not registered for this client). `--integrity-policy record-and-continue`
was used for the signed `.node` artifacts (Section 2.1).

| Run | Scope | Outcome |
| --- | --- | --- |
| 1 | Full `app.asar` (4,354 entries) | **Failed** — Node heap OOM at default 4 GB during parse (4,116/4,163 files) |
| 2 | Full `app.asar`, 10 GB heap | **Failed** — parse completed, projection aborted with V8 `RangeError: Set maximum size exceeded` |
| 3 | `package.json` + `out/` app code | **Failed** — heap OOM on the large minified renderer bundles |
| 4 | `out/main` + `out/preload` + `package.json` | **Succeeded** — evidence `ev_3b18fd0c7dd6570147427838d6cde14b0fccfc78fb1634a2507e9f7115ae5c59` |

Successful run statistics: 141,710 AST nodes visited, **0 parse failures**, 452 findings,
392 JavaScript module nodes recovered (including nested webpack modules inside the
`runtime-startup` bundle), 118,360 source-range nodes, 22,244 expression nodes,
9,266 call-site nodes, 5 IPC-channel observations, 3 endpoint observations
(`email`/`code`/`token` — consistent with auth/session flows), 6 BrowserWindow creations,
27 configuration-source reads, 19 process boundaries.

The analyzer's own limitations apply and were preserved in the evidence: dynamic IPC channel
expressions remain unresolved; Electron relationships are derived from inert syntax and do not
prove runtime registration, reachability, or enforcement; bundle bootstrap code is never
executed. Runs 1–3 constitute a **bounded negative result** for the current analyzer version
on this payload size, not a statement about the application's quality.

### Working artifacts (kept locally, excluded from Git)

- Extracted ASAR tree: `%TEMP%\opencode\willow-rea\extracted\`
- Main+preload inventory used for run 4: `%TEMP%\opencode\willow-rea\main-only\`
- Full evidence JSON (170 MB): shell output file `sh_14a3042fd0015rhMcHQTRzuBJp.out`
- Forensics scripts: `asar-integrity.mjs`, `asar-verify-unpacked.mjs`, `tail-check.mjs`,
  `pe-reconstruct.mjs` in `%TEMP%\opencode\willow-rea\`

---

## 6. Observations vs inferences vs unknowns

**Observed** — artifact hashes and layouts; fuse states; signing certificates; bundled helper
binaries and their spawn arguments; endpoint strings; IPC channel names; local data-directory
structure; settings keys.

**Inferred** — the dictation pipeline order (hook → capture → local Whisper → cleanup →
paste); the role of each endpoint; the meaning of team-level settings; that the ASAR hash
mismatch arose from post-packaging signing.

**Unknown / not verified** — runtime behavior of any feature; server-side handling of data;
the contents of user recordings/transcripts (deliberately not examined beyond structure);
whether the renderer code contains additional endpoints beyond filenames (renderer bundles
were not projectable by the analyzer); actual enforcement of privacy mode at runtime; behavior
under tampered resources (no tampering was performed).
