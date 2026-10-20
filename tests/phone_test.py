import argparse, json, urllib.request, urllib.error, pathlib, time, http.client
parser=argparse.ArgumentParser(description='Isolated HTTP checks; real-time expiry is opt-in.')
parser.add_argument('folder', type=pathlib.Path)
parser.add_argument('--slow', action='store_true', help='Include the real 61-second pairing expiry wait')
args=parser.parse_args()
folder=args.folder
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
page_html=urllib.request.urlopen(base+'/',timeout=5).read()
checks['page_removed_controls_and_has_quick_strip']=b'id="recoilMaster"' not in page_html and b'id="rapidMaster"' not in page_html and b'id="rapid"' not in page_html and b'id="radius"' not in page_html and b'id="accent"' not in page_html and b'id="recoilActive"' in page_html and b'id="rapidActive"' in page_html and b'id="quickStrip"' in page_html
checks['fresh_default_language_is_english']=json.loads(call('/api/state')[1])['globals']['language']==1
initial=json.loads(call('/api/state')[1])
checks['all_ten_phone_themes_available']=len(initial['appearance']['themes'])==10 and initial['appearance']['id']=='charcoal'
checks['theme_updates_require_pairing']=call('/api/update',{'action':'setTheme','theme':'midnight'},auth=False)[0]==401
checks['invalid_theme_is_rejected']=all(call('/api/update',{'action':'setTheme','theme':value})[0]==400 for value in ['unknown','',12,None])
night=json.loads(call('/api/update',{'action':'setTheme','theme':'midnight'})[1])
checks['phone_theme_applies_matching_materials']=night['appearance']['colors']['paper']=='#171522' and night['appearance']['colors']['ink']=='#f2eafb' and night['appearance']['scheme']=='dark'
checks['theme_change_preserves_profiles_and_dirty_state']=night['profile']==initial['profile'] and night['operatorId']==initial['operatorId'] and night['dirty']==initial['dirty']
checks['theme_is_saved_without_profile_save']=json.loads((folder/'azec_ui.json').read_text())['colorTheme']=='midnight'
call('/api/update',{'action':'setTheme','theme':'goofy'})
with urllib.request.urlopen(info['address']+'/',timeout=5) as response:
 checks['advertised_lan_page_available']=response.status==200 and b'pairForm' in response.read()
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
checks['primary_unchanged']=(lambda p:p['vertical']==10 and p['rapid']==False)(json.loads(call('/api/update',{'action':'selectWeapon','slot':0})[1])['profile'])
checks['secondary_preserved']=abs(json.loads(call('/api/update',{'action':'selectWeapon','slot':1})[1])['profile']['vertical']-23.7)<.001
checks['favorite_saves']=next(o for o in json.loads(call('/api/update',{'action':'setFavorite','operator':4,'enabled':True})[1])['operators'] if o['id']==4)['favorite']
state=json.loads(call('/api/state')[1])
checks['extension_operator_listed']=any(o['id']==71 and o['name']=='Noor' for o in state['operators'])
checks['selects_extension_operator']=json.loads(call('/api/update',{'action':'selectOperator','operator':71})[1])['operatorId']==71
checks['extension_profile_edits']=abs(json.loads(call('/api/update',{'action':'setProfile','operator':71,'slot':0,'values':{'vertical':31.5,'horizontal':-4.25,'delay':90,'rapid':True,'humanize':False}})[1])['profile']['vertical']-31.5)<.001
checks['selects_new_extension_operator']=json.loads(call('/api/update',{'action':'selectOperator','operator':77})[1])['operatorId']==77
checks['new_extension_profile_edits']=abs(json.loads(call('/api/update',{'action':'setProfile','operator':77,'slot':0,'values':{'vertical':22.5,'horizontal':1.5,'delay':70}})[1])['profile']['vertical']-22.5)<.001
call('/api/update',{'action':'selectOperator','operator':71})
checks['rejects_extension_boundary']=call('/api/update',{'action':'selectOperator','operator':78})[0]==400
checks['rapid_active_roundtrip']=json.loads(call('/api/update',{'action':'setGlobal','values':{'rapidActive':True}})[1])['globals']['rapidActive']==True
checks['save_succeeds']=call('/api/update',{'action':'save'})[0]==200
checks['legacy_layout_unchanged']=(folder/'r6_config.bin').stat().st_size==3734
saved=json.loads((folder/'azec_weapons.json').read_text())
checks['secondary_persists']=saved['operators']['Ash']['enabled'] and abs(saved['operators']['Ash']['vertical']-23.7)<.001
checks['extension_primary_persists']=saved['selected']==71 and abs(saved['primaryProfiles']['Noor']['vertical']-31.5)<.001 and saved['primaryProfiles']['Noor']['delay']==90
checks['new_extension_primary_persists']=abs(saved['primaryProfiles']['Denari']['vertical']-22.5)<.001 and saved['primaryProfiles']['Denari']['delay']==70
checks['rapid_active_persists']=saved['perProfileRapid']==True and saved['primaryProfiles']['Noor']['rapid']==True and saved['operators']['Ash']['rapid']==True
checks['operator_icon_available']=call('/icons/ash.svg',auth=False)[0]==200
checks['extension_icon_available']=call('/icons/noor.svg',auth=False)[0]==200
checks['current_roster_complete']=all(any(o['name']==n for o in state['operators']) for n in ('Nomad','Zofia','Zero','Rauora','Solid Snake','Denari'))
checks['solid_snake_icon_available']=call('/icons/solidsnake.svg',auth=False)[0]==200
checks['rejects_icon_traversal']=call('/icons/../phone_server.hpp',auth=False)[0]==404
checks['rejects_wrong_host']=call('/api/state',extra={'Host':'unrelated.example:'+str(info['port'])})[0]==403
checks['unknown_authenticated_route']=call('/missing')[0]==404
checks['manifest_available']=call('/manifest.webmanifest',auth=False)[0]==200
midnight=json.loads(call('/api/update',{'action':'setTheme','theme':'midnight'})[1])['appearance']['colors']
manifest=json.loads(call('/manifest.webmanifest',auth=False)[1])
checks['manifest_colors_follow_theme']=manifest['theme_color']==midnight['accent'] and manifest['background_color']==midnight['paper']
call('/api/update',{'action':'setTheme','theme':'goofy'})
checks['home_screen_icon_available']=call('/app-icon.svg',auth=False)[0]==200
checks['rejects_wrong_value_type']=call('/api/update',{'action':'setProfile','operator':4,'slot':0,'values':{'vertical':'12'}})[0]==400
checks['rejects_fractional_delay']=call('/api/update',{'action':'setProfile','operator':4,'slot':0,'values':{'delay':50.5}})[0]==400
checks['rejects_duplicate_hotkeys']=call('/api/update',{'action':'setGlobal','values':{'key2':45}})[0]==400
checks['rejects_mouse_button_binding']=call('/api/update',{'action':'setGlobal','values':{'key1':1}})[0]==400
checks['rejects_modifier_binding']=call('/api/update',{'action':'setGlobal','values':{'key1':17}})[0]==400
checks['rejects_removed_settings']=call('/api/update',{'action':'setGlobal','values':{'radius':200}})[0]==400 and call('/api/update',{'action':'setGlobal','values':{'rapidMaster':True}})[0]==400 and call('/api/update',{'action':'setGlobal','values':{'accent':'#123456'}})[0]==400
global_values={'key0':45,'key1':119,'key2':120,'key3':51,'key4':52,'language':0}
status,body=call('/api/update',{'action':'setGlobal','values':global_values})
state=json.loads(body)
checks['global_settings_roundtrip']=status==200 and state['globals']['language']==0 and state['globals']['accent']=='#7248d7' and 'radius' not in state['globals'] and 'rapidMaster' not in state['globals'] and 'recoilMaster' not in state['globals'] and 'rapidActive' in state['globals'] and len(state['keys'])==5 and state['keys'][1]['value']==119 and state['keys'][4]['value']==52
checks['rejects_wrong_global_type']=call('/api/update',{'action':'setGlobal','values':{'sound':'yes'}})[0]==400
checks['rejects_fractional_key']=call('/api/update',{'action':'setGlobal','values':{'key3':51.5}})[0]==400
checks['invalid_global_update_is_atomic']=json.loads(call('/api/state')[1])['keys'][2]['value']==120 and call('/api/update',{'action':'setGlobal','values':{'key2':121,'sound':'yes'}})[0]==400 and json.loads(call('/api/state')[1])['keys'][2]['value']==120
def raw_post(body,headers):
 connection=http.client.HTTPConnection('127.0.0.1',info['port'],timeout=5)
 try:
  connection.request('POST','/api/update',body,headers)
  response=connection.getresponse();status=response.status;response.read();return status
 finally:connection.close()
headers={'Authorization':'Bearer '+info['token'],'Content-Type':'application/x-www-form-urlencoded'}
checks['rejects_form_without_origin']=raw_post(b'action=save',headers)==400
checks['rejects_oversized_body']=raw_post(b' '*16385,{'Authorization':'Bearer '+info['token']})==400
checks['authenticated_json_without_origin']=call('/api/update',{'action':'save'})[0]==200
bad=[call('/api/pair',{'code':'000000'},False)[0] for _ in range(6)]
checks['sixth_failed_pairing_is_rate_limited']=bad==[401]*5+[429]
checks['pairing_lockout_keeps_authenticated_access']=call('/api/state')[0]==200
skipped=[]
if args.slow:
 print('Waiting for the 60-second pairing window to expire...',flush=True)
 time.sleep(61)
 checks['pairing_recovers_after_window']=call('/api/pair',{'code':info['code']},False)[0]==200
 checks['authenticated_access_after_window']=call('/api/state')[0]==200
else:
 skipped=['pairing_recovers_after_window','authenticated_access_after_window']
report={'checks':checks,'skipped':skipped,'passed':all(checks.values())}
(folder/'http-verification.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
raise SystemExit(0 if report['passed'] else 1)
