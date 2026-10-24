# Azec product overview

Narration comes first: `NARRATION.txt` is the plain text to paste into ElevenLabs. The revised scenes follow its paragraphs in order. Provisional duration: 189 seconds at 1920 × 1080, 60 fps. Retiming will follow the actual voice recording; this replaces the previous 14-second promo.

Keep the approved Charcoal & blush UI, Geist, Fredoka, pixel Azec wordmark, flat materials, ink outlines, yellow number chips and hard shadows. Remove the invented problem, slogans, operator counts, corner labels, bottom step rail, decorative footer branding and floating emoji. Show one useful chapter title and a large, continuous app view. The only other text belongs to the demonstrated controls.

Use the real sidebar/header/right-panel structure. A drawn pointer demonstrates each action without moving the user's cursor. No access to the installed app or personal profiles. Values, filenames, pairing address and code are fictional examples; the pairing graphic is an illustration, not a real session credential. The overview is a vector reconstruction of the actual controls, not a recording of native behavior.

| Time | Scene and exact title | Demonstration |
| --- | --- | --- |
| 0–8 | Open Azec | Azec.exe opens into the main app; no problem statement or sales copy. |
| 8–27 | Mouse setup | Open Settings, select Mouse setup, read example game settings, set DPI, save starting values, show a rescaling preview and cancel that preview. |
| 27–36 | Choose an operator | Return to operators, type Ash into search, select Ash; show its identity changing. |
| 36–51 | Primary and secondary | Enable Separate secondary profile, show the copied values, change the secondary values, switch back to the unchanged primary. Keys 1 and 2 are shown beside their actual tabs. |
| 51–66 | Recoil control | Move Vertical force, move Horizontal adjust, demonstrate a 0.1 adjustment and enable the header Recoil switch. |
| 66–80 | Rapid fire | Enable rapid fire for primary, adjust Shot delay, switch to secondary with rapid fire off, then back to primary with rapid fire on. |
| 80–90 | Save your changes | Press the header Save icon. Unsaved accent clears; the success tooltip appears. Select another operator and return while Save stays neutral. |
| 90–109 | Shortcuts | Show the defaults; open the compact binding popup, capture F8, then demonstrate the already-assigned rejection. |
| 109–126 | General | Show Only in Rainbow Six on, a brief illustrative inactive-output state while another window is active, then Language and Feedback sound controls. |
| 126–141 | Connect your phone | Open Phone control; show the hidden QR becoming revealed, the example LAN address, pairing code and copy actions; bring the phone into the scene. |
| 141–156 | Edit from your phone | Phone has Output first, profile selector, recoil controls and Save changes. Change a value; the desktop mirrors it; save on phone. |
| 156–167 | Appearance and favorites | Preview all ten palette cards, apply Lilac, return to operators, right-click Ash and choose Favorite. Ash moves to Favorites and appears only once. |
| 167–175 | Profile backup | Demonstrate Export profiles, Import profiles, the imported-unsaved state, then the shared Save control. |
| 175–186 | Updates | In General, highlight Check for updates. No fake claim that a real update check completed. |
| 186–189 | Azec | Plain logo end card. Hold the final frame. |

Type and layout: chapter title at (120, 125), 60 px Fredoka. Main app uses a 960 × 688 local drawing space, enlarged to approximately 1.15× and centered in the remaining frame. Settings have the actual six categories at left. Desktop/phone pairing chapters shrink the desktop to 1.05× at left and bring in a 390 × 770 phone at right. No explanatory captions competing with narration.

Motion: pointer travel uses quintic ease-in-out over 0.65–1.10 seconds, with 0.25-second press holds; it disappears after its last action. Values and toggles ease over 0.4–0.8 seconds. The view eases into place over 0.8 seconds and camera framing changes over 1.2 seconds with no bounce. Chapter titles fade out over the final 0.25 seconds, then fade in over 0.4 seconds; never overlap. Settings contents transition in place over 0.4 seconds. Keep long reading holds between actions. Every state and motion is an absolute-time function so MP3 syncing and frame export remain repeatable.

Deliver a self-contained autoplay canvas HTML with replay, chapter navigation, scrubber and optional local MP3 loading outside the video frame. Expose duration, fps, ready, scenes and render(t). A local voice file stays on the machine. The provisional chapter timings remain editable in one array for syncing after recording.

`motion.setTimings(boundaries)` retimes all action animation with its chapter, preserving the demonstrated states. The editable source is `overview.js`; rebuild the single HTML with `python docs/motion/build-overview.py` (fonttools, brotli and qrcode). Fonts and artwork are embedded; the original Illustrator-only source blocks are omitted from embedded operator SVGs.

Review completed: all 15 chapters, 15 evenly spaced frames, and four detail frames covering rescaling, app focus, the phone transition and favorites. Eight repeated renders matched exactly, and changing chapter timing preserved the corresponding frame. Final-time clamping passed and no external resource requests were made. No native Azec instance was launched. Voice timing and an MP4 with narration await the user's recording.
