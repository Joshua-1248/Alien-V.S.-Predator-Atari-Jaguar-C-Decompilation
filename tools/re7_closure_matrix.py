#!/usr/bin/env python3
"""RE #7 ordinary-68000 source-block closure matrix helper.

Consumes an external/private surviving source tree and the public readable-C tree.
It never copies source text; output contains only module/label/line/status metadata.
A block is never auto-certified as equivalent. Automation may only mark candidates;
manual proof in the ledger is required for closure.
"""
from __future__ import annotations
import argparse, json, re
from pathlib import Path

LABEL_RE = re.compile(r'^\s*([A-Za-z_.$][A-Za-z0-9_.$]*)(::?|:)\s*(.*)$')
BRANCH_RE = re.compile(r'\b(?:b(?:ra|sr|cc|cs|eq|ge|gt|hi|le|ls|lt|mi|ne|pl|vc|vs)|db(?:ra|f|t|cc|cs|eq|ge|gt|hi|le|ls|lt|mi|ne|pl|vc|vs)|jmp|jsr)\.?[bwl]?\s+([^,;\s]+)', re.I)
DATA_OPS = {'dc.b','dc.w','dc.l','ds.b','ds.w','ds.l','equ','set','reg','macro','endm'}

MODULE_MAP = {
 'MAIN/MAIN.S':['src/game/main_game.c'],
 'MAZE/MAZE.S':['src/game/maze.c'],
 'MAZE/MAZESCRN.S':['src/game/mazescrn.c','src/game/weapons.c','src/game/music.c'],
 'MAZE/LEVELS.S':['src/game/levels.c','src/game/doors.c'],
 'MAZE/PLAYER.S':['src/game/player.c','src/game/collectables.c','src/game/weapons.c'],
 'MAZE/DOORS.S':['src/game/doors.c'],
 'MAZE/COLLIDE.S':['src/game/collision.c'],
 'MAZE/HUD.S':['src/game/hud.c','src/game/hud_score.c'],
 'MAZE/HUD_MSG.S':['src/game/hud_message.c'],
 'MAZE/COMPUTER.S':['src/game/computer.c'],
 'MAZE/AVPCART.S':['src/game/eeprom.c','src/game/savegame.c'],
 'AMP/AMP.S':['src/game/amp.c'],
 'AMP/FONT.S':['src/game/font.c'],
}

def norm_module(p: Path, root: Path) -> str:
    rel = p.relative_to(root).as_posix()
    # surviving archive has "Source Code/" prefix
    if rel.lower().startswith('source code/'):
        rel = rel[len('Source Code/'):]
    return rel

def strip_comment(line: str) -> str:
    return line.split(';',1)[0].rstrip()

def code_hint(croot: Path, files, symbol: str) -> bool:
    if symbol.startswith('.'):
        return False
    pat = re.compile(r'(?<![A-Za-z0-9_])'+re.escape(symbol.strip('.'))+r'(?![A-Za-z0-9_])', re.I)
    for rel in files:
        p=croot/rel
        if p.exists() and pat.search(p.read_text(errors='ignore')):
            return True
    return False

def parse_source(path: Path, module: str):
    rows=[]; current_global=None
    lines=path.read_text(errors='ignore').splitlines()
    labels=[]
    for i,line in enumerate(lines,1):
        bare=strip_comment(line); m=LABEL_RE.match(bare)
        if not m: continue
        name,colons,tail=m.groups()
        if name.startswith('$') or name[0].isdigit(): continue
        is_global=colons=='::'
        if is_global: current_global=name
        elif not name.startswith('.') and current_global is None: current_global=name
        labels.append((i,name,is_global,current_global or '',tail))
    # Classify a label by the body until the next label.  This avoids treating
    # named data tables as executable blocks merely because the label is alone.
    directive_re=re.compile(r'^\s*(?:dc\.[bwl]|ds\.[bwl]|equ|set|reg|include|incbin|even|align|section|if|ifdef|ifndef|else|endif|macro|endm)\b',re.I)
    instr_re=re.compile(r'^\s*(?:move|moveq|movem|lea|pea|clr|tst|cmp|cmpi|cmpa|add|adda|addi|addq|sub|suba|subi|subq|mulu|muls|divu|divs|and|andi|or|ori|eor|eori|not|neg|negx|ext|swap|asl|asr|lsl|lsr|rol|ror|roxl|roxr|btst|bchg|bclr|bset|bra|bsr|bcc|bcs|beq|bge|bgt|bhi|ble|bls|blt|bmi|bne|bpl|bvc|bvs|dbra|dbf|dbt|dbcc|dbcs|dbeq|dbge|dbgt|dbhi|dble|dbls|dblt|dbmi|dbne|dbpl|dbvc|dbvs|jmp|jsr|rts|rte|rtr|link|unlk|trap|stop|nop)\.?[bwl]?\b',re.I)
    for idx,(i,name,is_global,scope,tail) in enumerate(labels):
        end=(labels[idx+1][0]-1) if idx+1<len(labels) else len(lines)
        body=[]
        if tail.strip(): body.append(tail)
        body += lines[i:end]
        has_instr=False; has_data=False
        for raw in body:
            b=strip_comment(raw).strip()
            if not b: continue
            # macro invocations such as sfx/test_key/soa_* are executable too.
            if instr_re.match(b) or re.match(r'^(?:sfx|wep_sfx|un_loop|loop_sfx|test_key|soa_[A-Za-z0-9_]+|soq_[A-Za-z0-9_]+)\b',b,re.I): has_instr=True
            if directive_re.match(b): has_data=True
        kind=('global' if is_global else 'local') if has_instr else 'data'
        rows.append({'module':module,'label':name,'line':i,'scope':scope,'kind':kind})
    return rows

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--asm-root',required=True,type=Path)
    ap.add_argument('--c-root',required=True,type=Path)
    ap.add_argument('--ledger',type=Path)
    ap.add_argument('--json-out',required=True,type=Path)
    ap.add_argument('--md-out',required=True,type=Path)
    a=ap.parse_args()
    ledger={}
    if a.ledger and a.ledger.exists():
        obj=json.loads(a.ledger.read_text())
        ledger=obj.get('blocks',{})
    rows=[]
    for module,cfiles in MODULE_MAP.items():
        candidates=[a.asm_root/module, a.asm_root/'Source Code'/module]
        sp=next((p for p in candidates if p.exists()),None)
        if not sp: continue
        for r in parse_source(sp,module):
            key=f"{module}:{r['line']}:{r['label']}"
            manual=ledger.get(key,{})
            hint=code_hint(a.c_root,cfiles,r['label'])
            r['key']=key
            r['c_files']=cfiles
            r['symbol_hint']='present' if hint else 'absent'
            r['status']=manual.get('status','UNRESOLVED')
            r['proof']=manual.get('proof','')
            # Data labels don't need code translation unless ledger overrides.
            if r['status']=='UNRESOLVED' and r['kind']=='data': r['status']='DATA_CANDIDATE'
            rows.append(r)
    a.json_out.write_text(json.dumps({'rows':rows},indent=2)+"\n")
    counts={}
    for r in rows: counts[r['status']]=counts.get(r['status'],0)+1
    out=['# RE #7 ordinary-68000 source-block closure matrix','',
         '> Generated metadata only. `symbol_hint=present` is **not proof** of semantic equivalence.','',
         '## Counts','']
    for k in sorted(counts): out.append(f'- **{k}**: {counts[k]}')
    out += ['', '| Module | Line | Label | Kind | C symbol hint | Status | Proof |',
            '|---|---:|---|---|---|---|---|']
    for r in rows:
        proof=r['proof'].replace('|','\\|')
        out.append(f"| `{r['module']}` | {r['line']} | `{r['label']}` | {r['kind']} | {r['symbol_hint']} | **{r['status']}** | {proof} |")
    a.md_out.write_text('\n'.join(out)+'\n')
    print(json.dumps(counts,sort_keys=True))

if __name__=='__main__': main()
