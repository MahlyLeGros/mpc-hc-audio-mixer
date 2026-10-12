"""Generate MOV, AVI and WebM multitrack cases from the existing baseline fixtures."""
from pathlib import Path
import subprocess,sys
ffmpeg=sys.argv[1]
out=Path(sys.argv[2])
def run(name,args):
 subprocess.run([ffmpeg,'-hide_banner','-loglevel','error','-y',*args,str(out/name)],check=True)
run('three-tracks.mov',['-i',str(out/'three-tracks.mp4'),'-map','0','-c','copy'])
run('three-tracks.webm',['-i',str(out/'three-tracks.mkv'),'-map','0','-c:v','libvpx-vp9','-deadline','realtime','-cpu-used','8','-c:a','libopus'])
# AVI does not represent a late audio start reliably: encode leading silence into PCM.
run('three-tracks.avi',['-i',str(out/'three-tracks.mkv'),'-map','0','-c:v','mpeg4','-q:v','5','-c:a','pcm_s16le','-filter:a:1','asetpts=PTS-STARTPTS,adelay=1250:all=1'])
for suffix in ('mov','avi','webm'):
 run('one-track.'+suffix,['-i',str(out/('three-tracks.'+suffix)),'-map','0:v','-map','0:a:0','-c','copy'])
print('Created MOV/AVI/WebM with three audio tracks and single-track controls')
