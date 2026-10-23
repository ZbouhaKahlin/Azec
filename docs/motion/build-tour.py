"""Build the self-contained tour from the editable canvas source and bundled art."""
from pathlib import Path
import base64, io, json, re, sys, gzip
import xml.etree.ElementTree as ET
from fontTools import subset
from fontTools.ttLib import TTFont
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'.build/motion/python-deps'))
import qrcode
def data(payload,mime):return 'data:'+mime+';base64,'+base64.b64encode(payload).decode()
def font(file,family,weight):
    f=TTFont(ROOT/'assets/fonts'/file)
    options=subset.Options()
    s=subset.Subsetter(options=options);s.populate(unicodes=list(range(32,127))+[0x2014,0x2013,0x00d7,0x2039,0x2026,0x00b7]);s.subset(f)
    f.flavor='woff2';b=io.BytesIO();f.save(b)
    return '@font-face{font-family:"'+family+'";font-weight:'+weight+';src:url("'+data(b.getvalue(),'font/woff2')+'") format("woff2");font-display:block}'
fonts='\n'.join([font('Geist-Regular.ttf','Geist','400'),font('Geist-Medium.ttf','Geist','500'),font('Geist-SemiBold.ttf','Geist','600'),font('Fredoka.ttf','Fredoka','300 700'),font('PressStart2P-Regular.ttf','Pixel','400')])
assets={}
for name in ['logo','ace','alibi','amaru','aruni','ash','azami','bandit']:
    p=ROOT/'assets'/(name+'.svg') if name=='logo' else ROOT/'assets/operators'/(name+'.svg')
    root=ET.fromstring(p.read_text(encoding='utf-8'))
    for parent in root.iter():
        for child in list(parent):
            if 'ns.adobe.com' in child.tag or child.tag.rsplit('}',1)[-1] in ['metadata','foreignObject']:
                parent.remove(child)
    ET.register_namespace('','http://www.w3.org/2000/svg')
    assets[name]=data(ET.tostring(root,encoding='utf-8'),'image/svg+xml')
# Fictional example only. This is not a live app token or personal pairing link.
qr=qrcode.QRCode(border=0,error_correction=qrcode.constants.ERROR_CORRECT_M)
qr.add_data('http://192.168.1.20:18764/?code=123456');qr.make(fit=True)
packed=base64.b64encode(gzip.compress(json.dumps(assets,separators=(',',':')).encode(),mtime=0)).decode()
source=(HERE/'tour.js').read_text(encoding='utf-8').replace('__ASSETS_GZIP__',json.dumps(packed)).replace('__QR__',json.dumps(qr.get_matrix(),separators=(',',':')))
notices=[]
for folder in ['Geist','Fredoka','PressStart2P','r6operators']:
    for p in (ROOT/'licenses'/folder).rglob('*'):
        if p.is_file() and p.suffix.lower() in ['.txt','.md','']:
            notices.append(folder+'/'+p.name+'\n'+p.read_text(encoding='utf-8',errors='replace'))
license_comment='<!--\nTHIRD PARTY NOTICES\n'+'\n\n'.join(notices).replace('--','- -')+'\n-->'
transcript=(HERE/'NARRATION.txt').read_text(encoding='utf-8')
html='''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Azec — Full product tour</title><style>
'''+fonts+'''
*{box-sizing:border-box}html,body{margin:0;background:transparent;color:var(--foreground,#F8EFF2);font-family:Geist,Arial,sans-serif}
canvas{display:block;width:100%;height:auto;aspect-ratio:16/9;background:#222122;border-radius:12px}.controls{display:flex;align-items:center;gap:10px;padding:12px 2px;font-size:12px;color:var(--muted-foreground,#C8B7BF)}button,select,.file-label{font:500 12px Geist,Arial,sans-serif;color:inherit;border:1px solid var(--border,#58414A);border-radius:18px;background:var(--background,#222122);padding:7px 11px;cursor:pointer}select{max-width:220px;min-width:100px}input[type=range]{flex:1;min-width:40px;accent-color:#F2B3C6}output{font-variant-numeric:tabular-nums;white-space:nowrap}button:focus-visible,select:focus-visible,input:focus-visible,.file-label:focus-within{outline:2px solid #F2B3C6;outline-offset:3px}.audio-row{display:flex;align-items:center;gap:12px;padding:0 2px 12px;font-size:12px;color:var(--muted-foreground,#C8B7BF)}input[type=file]{position:absolute;width:1px;height:1px;opacity:0}#voiceStatus{overflow-wrap:anywhere}.sr-only{position:absolute;width:1px;height:1px;overflow:hidden;clip:rect(0,0,0,0)}@media(max-width:600px){.controls{flex-wrap:wrap;gap:7px}select{order:5;max-width:none;width:100%}.audio-row{align-items:flex-start}}
</style></head><body><main><canvas id="film" width="1920" height="1080" role="img" aria-label="Azec full product tour: setup, profiles, recoil, rapid fire, saving, shortcuts, phone controls, appearance, backups and updates."></canvas><div class="controls"><button id="play">Pause</button><button id="replay">Replay</button><input id="seek" aria-label="Tour time" type="range" min="0" max="189" step="0.0166666667"><output id="time">0:00 / 3:09</output><select id="chapter" aria-label="Jump to chapter"></select></div><div class="audio-row"><label class="file-label">Load voice MP3<input id="voiceFile" type="file" accept="audio/mpeg,audio/mp3,audio/*"></label><span id="voiceStatus">Silent preview · scene timing awaits your narration.</span></div><audio id="voice" preload="auto"></audio><p class="sr-only">'''+transcript+'''</p></main><script>'''+source+'''</script>'''+license_comment+'''</body></html>'''
html='\n'.join(line.rstrip() for line in html.splitlines())+'\n'
destination=HERE/'azec-explainer.html';destination.write_text(html,encoding='utf-8')
print(f'{destination} ({destination.stat().st_size:,} bytes)')
