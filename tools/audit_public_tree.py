#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
# Public C repo policy: no known ROM/media/resource payload extensions.
bad_ext={'.jag','.rom','.bin','.aaf','.wav','.mp3','.flac','.j4','.lbm','.iff','.png','.jpg','.jpeg','.bmp'}
# Explicitly permit no binary assets at all in active public tree; research docs only.
viol=[]
for p in ROOT.rglob('*'):
    if not p.is_file(): continue
    rel=p.relative_to(ROOT)
    if any(x == '.git' or x == 'build' or x.startswith('build-') or x.startswith('build_') for x in rel.parts): continue
    if p.suffix.lower() in bad_ext:
        viol.append(str(rel))
if viol:
    print('FAIL: prohibited ROM/media/resource-looking files:')
    for v in viol: print(' ',v)
    sys.exit(1)
print('PASS: no prohibited ROM/media/resource payloads found')
