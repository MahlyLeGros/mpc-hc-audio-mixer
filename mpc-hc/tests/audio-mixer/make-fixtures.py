"""Generate small synthetic media; requires ffmpeg as the first argument."""
import json
import subprocess
import sys
from pathlib import Path

ffmpeg = sys.argv[1]
out = Path(sys.argv[2] if len(sys.argv) > 2 else 'bin/mixer-tests/media').resolve()
out.mkdir(parents=True, exist_ok=True)

def run(name, args):
    subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-y', *args, str(out / name)], check=True)

for suffix in ['mkv', 'mp4']:
    args = ['-f', 'lavfi', '-i', 'testsrc2=size=160x90:rate=25:duration=8',
            '-f', 'lavfi', '-i', 'sine=frequency=440:sample_rate=48000:duration=8',
            '-itsoffset', '1.25', '-f', 'lavfi', '-i', 'sine=frequency=880:sample_rate=44100:duration=3',
            '-f', 'lavfi', '-i', 'sine=frequency=220:sample_rate=48000:duration=8',
            '-map', '0:v', '-map', '1:a', '-map', '2:a', '-map', '3:a',
            '-c:v', 'libx264', '-preset', 'ultrafast', '-pix_fmt', 'yuv420p',
            '-metadata:s:a:0', 'title=Jeu', '-metadata:s:a:0', 'language=eng',
            '-metadata:s:a:1', 'title=Micro', '-metadata:s:a:1', 'language=fra',
            '-metadata:s:a:2', 'title=Musique', '-ac:a:0', '2', '-ac:a:1', '2', '-ac:a:2', '1']
    args += ['-c:a:0', 'pcm_f32le', '-c:a:1', 'pcm_s16le', '-c:a:2', 'libopus'] if suffix == 'mkv' else ['-c:a', 'aac']
    run('three-tracks.' + suffix, args)

run('one-track.mp4', ['-i', str(out / 'three-tracks.mp4'), '-map', '0:v', '-map', '0:a:0', '-c', 'copy'])

run('two-tracks.mkv', ['-i', str(out / 'three-tracks.mkv'), '-map', '0:v', '-map', '0:a:0', '-map', '0:a:1', '-c', 'copy'])
run('long-30min.mp4', ['-f', 'lavfi', '-i', 'color=c=black:size=64x64:rate=25:duration=1800',
    '-f', 'lavfi', '-i', r'aevalsrc=if(lt(mod(t\,1)\,0.02)\,0.4*sin(2*PI*440*t)\,0):s=48000:d=1800',
    '-f', 'lavfi', '-i', r'aevalsrc=if(lt(mod(t\,1)\,0.02)\,0.4*sin(2*PI*880*t)\,0):s=44100:d=1800',
    '-vf', r'drawbox=x=0:y=0:w=64:h=64:color=white:t=fill:enable=eq(mod(n\,25)\,0)',
    '-map', '0:v', '-map', '1:a', '-map', '2:a', '-c:v', 'libx264', '-preset', 'ultrafast', '-c:a', 'aac', '-b:a', '32k'])
(out / 'manifest.json').write_text(json.dumps({'duration_seconds': 8, 'delayed_track_start_seconds': 1.25,
    'delayed_track_end_seconds': 4.25, 'long_duration_seconds': 1800,
    'long_markers': 'white video flash and a 20ms audio pulse every second'}, indent=2))
print(out)
