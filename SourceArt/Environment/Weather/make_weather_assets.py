"""Generate the storm source assets: rain ambience loop, thunder claps and lightning-bolt sprites.

Run with the system Python (numpy + Pillow). Outputs sit next to this script and are
imported into /Game/Environment/Weather.
"""
import os
import wave

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.dirname(os.path.abspath(__file__))
SR = 44100
rng = np.random.default_rng(1947)


def write_wav(name, data):
    data = np.clip(data, -1.0, 1.0)
    pcm = (data * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name), 'wb') as w:
        w.setnchannels(1 if pcm.ndim == 1 else pcm.shape[1])
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def onepole_lp(x, cutoff):
    a = np.exp(-2 * np.pi * cutoff / SR)
    y = np.empty_like(x)
    acc = 0.0
    for i in range(len(x)):
        acc = (1 - a) * x[i] + a * acc
        y[i] = acc
    return y


def lp_fft(x, cutoff, slope=2):
    """Smooth low-pass in the frequency domain (fast for long buffers)."""
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    X *= 1 / (1 + (f / cutoff) ** (2 * slope)) ** 0.5
    return np.fft.irfft(X, len(x))


def hp_fft(x, cutoff, slope=2):
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    X *= 1 - 1 / (1 + (f / cutoff) ** (2 * slope)) ** 0.5
    return np.fft.irfft(X, len(x))


def rain_loop(seconds=12.0):
    n = int(SR * seconds)
    hiss = hp_fft(lp_fft(rng.standard_normal(n), 7000), 400) * 0.18
    body = lp_fft(rng.standard_normal(n), 900) * 0.25
    # droplet ticks on hard surfaces
    drops = np.zeros(n)
    for t in rng.integers(0, n - 2000, size=int(seconds * 260)):
        L = rng.integers(200, 900)
        env = np.exp(-np.linspace(0, 9, L))
        drops[t:t + L] += rng.standard_normal(L) * env * rng.uniform(0.05, 0.25)
    drops = hp_fft(drops, 1500)
    mix = hiss + body + drops * 0.6
    # seamless loop: crossfade the tail into the head
    xf = int(SR * 1.0)
    fade = np.linspace(0, 1, xf)
    mix[:xf] = mix[:xf] * fade + mix[-xf:] * (1 - fade)
    mix = mix[:-xf]
    mix /= np.max(np.abs(mix)) + 1e-9
    return np.stack([mix * 0.7, np.roll(mix, 2111) * 0.7], axis=1)


def thunder(seed, seconds=7.0, crack=0.6):
    r = np.random.default_rng(seed)
    n = int(SR * seconds)
    t = np.linspace(0, seconds, n)
    # rolling rumble: several low bursts with long decays
    rumble = np.zeros(n)
    for _ in range(r.integers(4, 8)):
        start = r.uniform(0.0, seconds * 0.45)
        dec = r.uniform(0.6, 2.2)
        env = np.where(t >= start, np.exp(-(t - start) / dec), 0.0)
        env *= np.clip((t - start) / 0.08, 0, 1)
        rumble += env * r.uniform(0.4, 1.0)
    noise = lp_fft(r.standard_normal(n), 140, slope=3)
    noise /= np.max(np.abs(noise)) + 1e-9
    body = noise * rumble
    # initial crack
    ck = np.zeros(n)
    L = int(SR * 0.35)
    ck[:L] = hp_fft(r.standard_normal(L), 900)[:L] * np.exp(-np.linspace(0, 10, L))
    out = body + ck * crack
    out *= np.clip(t / 0.01, 0, 1)
    out /= np.max(np.abs(out)) + 1e-9
    return out * 0.95


def bolt(seed, w=512, h=1024):
    r = np.random.default_rng(seed)
    img = Image.new('L', (w, h), 0)
    d = ImageDraw.Draw(img)

    def branch(x, y, ang, length, width, depth):
        pts = [(x, y)]
        while length > 0 and 0 <= y < h:
            seg = r.uniform(10, 28)
            ang += r.normal(0, 0.45)
            # steer back toward the middle so the bolt stays inside the sprite
            ang += (x - w / 2) / w * 0.6
            ang = np.clip(ang, np.pi * 0.2, np.pi * 0.8)
            x += np.cos(ang) * seg
            y += np.sin(ang) * seg
            pts.append((x, y))
            length -= seg
            if depth < 3 and r.random() < 0.07:
                branch(x, y, ang + r.choice([-1, 1]) * r.uniform(0.4, 0.9), length * r.uniform(0.25, 0.5),
                       max(1, width - 2), depth + 1)
        d.line(pts, fill=255, width=int(width))

    branch(w * r.uniform(0.4, 0.6), 0, np.pi / 2, h * 1.6, 5, 0)
    core = img
    glow = img.filter(ImageFilter.GaussianBlur(10))
    halo = img.filter(ImageFilter.GaussianBlur(28))
    a = np.asarray(core, float) + np.asarray(glow, float) * 1.6 + np.asarray(halo, float) * 1.2
    a = np.clip(a, 0, 255).astype(np.uint8)
    return Image.fromarray(a)


if __name__ == '__main__':
    write_wav('S_RainLoop.wav', rain_loop())
    for i, (seed, sec, crack) in enumerate([(11, 7.0, 0.8), (23, 8.5, 0.4), (37, 6.0, 1.0)]):
        write_wav(f'S_Thunder_{i + 1:02d}.wav', thunder(seed, sec, crack))
    for i, seed in enumerate((5, 17, 29)):
        bolt(seed).save(os.path.join(OUT, f'T_LightningBolt_{i + 1:02d}.png'))
    print('done')
