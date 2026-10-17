import json, urllib.request, urllib.error, pathlib, sys
folder=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else ".build/phone-check")
info=json.loads((folder/'phone-info.json').read_text())
if not info.get('dryRun'):
 raise SystemExit('Run HTTP checks against an isolated --dry-run instance only.')
base='http://127.0.0.1:'+str(info['port'])
def call(path,data=None,auth=True,extra=None):
 headers={} if not auth else {'Authorization':'Bearer '+info['token']}
 if data is not None: headers['Content-Type']='application/json'
 if extra: headers.update(extra)
 req=urllib.request.Request(base+path,data=json.dumps(data).encode() if data is not None else None,headers=headers)
 try:
  with urllib.request.urlopen(req,timeout=5) as r:return r.status, r.read()
 except urllib.error.HTTPError as e:return e.code,e.read()
checks={}
checks['page_available']=call('/',auth=False)[0]==200
checks['requires_pairing']=call('/api/state',auth=False)[0]==401
checks['bad_pairing_code']=call('/api/pair',{'code':'000000'},False)[0]==401
checks['pairing_returns_token']=json.loads(call('/api/pair',{'code':info['code']},False)[1])['token']==info['token']
checks['rejects_cross_origin']=call('/api/update',{'action':'save'},extra={'Origin':'https://unrelated.example'})[0]==403
checks['rejects_invalid_operator']=call('/api/update',{'action':'selectOperator','operator':999})[0]==400
checks['rejects_invalid_profile']=call('/api/update',{'action':'setProfile','operator':4,'slot':0,'values':{'vertical':-1}})[0]==400
checks['rejects_invalid_slot']=call('/api/update',{'action':'setProfile','operator':4,'slot':1,'values':{'vertical':22}})[0]==400
checks['selects_operator']=json.loads(call('/api/update',{'action':'selectOperator','operator':4})[1])['operatorId']==4
checks['enables_secondary']=json.loads(call('/api/update',{'action':'setDual','operator':4,'enabled':True})[1])['weaponSlot']==1
checks['edits_secondary']=abs(json.loads(call('/api/update',{'action':'setProfile','operator':4,'slot':1,'values':{'vertical':23.7,'horizontal':-3.4,'humanize':False,'rapid':True,'delay':85}})[1])['profile']['vertical']-23.7)<.001
checks['primary_unchanged']=json.loads(call('/api/update',{'action':'selectWeapon','slot':0})[1])['profile']['vertical']==10
checks['secondary_preserved']=abs(json.loads(call('/api/update',{'action':'selectWeapon','slot':1})[1])['profile']['vertical']-23.7)<.001
checks['favorite_saves']=next(o for o in json.loads(call('/api/update',{'action':'setFavorite','operator':4,'enabled':True})[1])['operators'] if o['id']==4)['favorite']
checks['save_succeeds']=call('/api/update',{'action':'save'})[0]==200
checks['legacy_layout_unchanged']=(folder/'r6_config.bin').stat().st_size==3734
saved=json.loads((folder/'kingboss_weapons.json').read_text())
checks['secondary_persists']=saved['operators']['Ash']['enabled'] and abs(saved['operators']['Ash']['vertical']-23.7)<.001
checks['operator_icon_available']=call('/icons/ash.svg',auth=False)[0]==200
checks['rejects_icon_traversal']=call('/icons/../phone_server.hpp',auth=False)[0]==404
report={'checks':checks,'passed':all(checks.values())}
(folder/'http-verification.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
