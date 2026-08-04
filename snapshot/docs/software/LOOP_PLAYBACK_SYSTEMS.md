# Loop Playback Systems — Detailed Specification

Two related architectural changes to the loop/drone signal flow. System 1 is the
foundation; System 2 builds on top of it.

---

## System 1: Delayed Dry Playback

### Problem
The drone maker takes 3-4 seconds to build up output from incoming audio. With
loops, the dry audio is never heard — it only exists as input to the drone. This
means there's no audible connection between the source material and the drone
output. With loops (unlike live audio), we know the future, so we can delay the
dry playback to synchronise it with the drone output.

### Architecture

**Signal flow per loop slot:**
```
Raw loop buffer
  → Read sample at current position
  → Apply per-loop HP/LP/volume
  → Processed sample goes to:
      (a) Drone maker input (wet send, controlled by wet level)
      (b) Processed history ring buffer (write)

History ring buffer
  → Read from N seconds behind write position
  → Dry output (controlled by dry level)

Drone output + all dry outputs → effects chain → master output
```

**Key properties:**
- One processed audio stream per slot, two consumers
- The drone and dry output always correspond — both reflect filter/volume
  changes at approximately the same time (~N second delay from parameter change)
- Changing HP/LP/volume is not heard on either output for ~N seconds, maintaining
  continuity between dry and wet

### New parameters
- **Drone offset** (global, on Drone page): N seconds, the delay between drone
  input and dry output. Set once, rarely changed. Default TBD (~3-5 seconds).
- **Wet send level** (per loop): how much processed audio goes to the drone input.
  0 = loop is silent in the drone.
- **Dry send level** (per loop): how much delayed processed audio goes to the
  output. 0 = no dry audio heard.

### Recording behaviour
- Audio feeds into the drone maker from the moment recording starts (not when
  playback begins). This means the drone is already built up by the time the loop
  starts cycling.
- When recording stops and playback begins, there must be no gap in the audio
  stream feeding the drone — seamless handoff from recording to looped playback.
- Dry output begins N seconds after the loop first starts feeding the drone.

### History buffer
- Per-slot mono ring buffer, ~5-6 seconds at sample rate (~250KB per slot)
- Write position advances every sample during recording and playback
- Read position = write position - (offset * sampleRate)
- Cleared when slot is cleared or new recording starts

### Per-loop filters and the delay tradeoff
- HP/LP/volume are applied before the audio enters the drone AND before it's
  written to the history buffer. So filter changes affect both paths identically,
  but with ~N second latency on both.
- This means twisting a filter knob doesn't produce an audible result for several
  seconds. This is a necessary tradeoff to maintain dry/wet continuity.

---

## System 2: Bounce-in-Place

### Problem
Running all loops through the drone maker consumes significant CPU (two FFT
processor instances for stereo). Bouncing a loop pre-renders the drone output for
that loop, replacing live FFT processing with simple stereo playback.

### Workflow
1. User triggers bounce on a loop slot (e.g., push button action)
2. An offline process creates fresh FFT processor instances with current drone
   parameter snapshot
3. The loop is played through the offline drone maker for 2x the loop length
4. A usable region is extracted starting ~10-15 seconds in (skipping FFT startup
   transient), with length equal to the original loop
5. The bounced stereo audio is stored in the slot
6. The system auto-transitions: fades the original loop out of the drone input,
   fades the bounced version in through the dry output path
7. The slot switches to "bounced mode"

### Bounce settings
- **Bypass all automation/modulation/volume/filter adjustments during bounce.**
  The bounce captures the loop as if it were at default settings (volume=1,
  HP=20Hz, LP=20kHz, no automation, no modulation). This prevents situations
  where a bounce captures a nearly-silent or heavily-filtered version that's
  unusable.
- Drone parameters (FFT size, threshold, ratio, spectral settings) ARE captured
  at bounce time — these define the drone character.

### Post-bounce parameter handling
- After bouncing, automation and modulation that were being applied pre-drone
  would now need to be applied to the bounced audio directly.
- This changes where processing sits in the signal chain (pre-drone vs post-drone),
  which may alter the sound.
- Acceptable approach: try applying existing automation/modulation to the bounced
  audio and evaluate sonically. If the difference is too significant, implement a
  transition script that disables automation/modulation and resets parameters to
  defaults when switching to bounced mode.
- This is a known compromise — preferable to destructive parameter freezing.

### Offline processing
- Runs in a background thread, not in the audio callback
- Self-throttles to avoid starving the live audio thread
- Processes samples in batches (e.g., 256-1024 samples per iteration)
- Can run slower than real-time — a 30s loop bounced at half-speed takes ~2 minutes
- Progress indicator on the loop button (fill animation or percentage)
- Cancellable by the user

### Crossfaded dual-voice playback
- Bounced audio is stereo (drone output is stereo)
- Two playback heads offset by half the bounced length
- Long crossfade (several seconds) using Hann or raised-cosine envelope
- Creates seamless, click-free looping of the bounced material

### Transition (live → bounced)
- Auto-triggered when bounce completes
- Transition point is calculated to align the current drone output position with
  the corresponding position in the bounced stereo file — no crossfading between
  unrelated parts of the audio
- The original loop's wet send fades to zero (stops feeding the drone)
- The drone's existing spectral energy from that loop decays naturally
- The bounced audio fades in through the dry output path
- The overlap between natural drone decay and bounced audio playback should sound
  musical and smooth

### Bounced mode per slot
- Slot switches from live mode to bounced mode
- Bounced mode: stereo buffer → crossfaded dual-voice playback → dry output path
  → effects chain → master output
- NOT fed to the drone maker
- Different control set: loop window position, crossfade length, volume envelope,
  trim, pitch (TBD)
- Original raw loop buffer is preserved — un-bouncing reverts to live mode

### Memory
- Bounced stereo buffer: 30s at 44.1kHz = ~5.3MB per slot
- Max 8 slots bounced = ~42MB total
- Acceptable on Pi

### Thread safety
- Offline bounce writes to a temporary buffer
- When complete, pointer swap or atomic flag signals the audio thread
- Audio thread never reads the bounce buffer until the flag is set

---

## Implementation Order

1. **System 1 first** — delayed dry playback, history buffer, wet/dry per loop,
   drone offset parameter, feed-during-recording
2. **System 2 second** — offline bouncer, crossfaded playback, transition logic,
   bounced mode controls

System 1 creates the dry output path that System 2 reuses.

---

## Open Questions

- Exact drone offset default value (needs testing — measure FFT buildup time)
- Bounced mode control set (what controls are available post-bounce)
- Whether post-bounce automation/modulation sounds acceptable or needs reset script
- Bounce progress UI design
- Whether to expose bounce-related controls on encoder push buttons or elsewhere
