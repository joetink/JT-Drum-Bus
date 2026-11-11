# JT-Drum-Bus

Transient Shaper (Attack / Sustain / Speed / Separation / Mix) -> G-Series Bus Comp (feedback VCA, dominant peak, ratio-dependent knee, 0.1/0.3/1/3/10/30ms attack, 0.1/0.3/0.6/1.2s + AUTO release, 2/4/10 ratio) -> D-Saturation (Drive + Mix only, gentle diode) -> Clipper (SOFT/HARD/ASYMM + CLIP METER + waveform scope)

Order: Shaper -> Comp -> Sat -> Clip (Clip last). 4x oversample on RENDER ONLY for D-Sat + Clip. Playback 1x.
