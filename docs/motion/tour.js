'use strict';
let ASSETS={};
const PACKED_ASSETS=__ASSETS_GZIP__,QR=__QR__;
// Timings are provisional until the ElevenLabs recording arrives.
const SCENES=[
  [0,8,'Open Azec'],[8,27,'Mouse setup'],[27,36,'Choose an operator'],
  [36,51,'Primary and secondary'],[51,66,'Recoil control'],[66,80,'Rapid fire'],
  [80,90,'Save your changes'],[90,109,'Shortcuts'],[109,126,'General'],
  [126,141,'Connect your phone'],[141,156,'Edit from your phone'],
  [156,167,'Appearance and favorites'],[167,175,'Profile backup'],
  [175,186,'Updates and tour'],[186,189,'Azec']
].map(([start,end,title])=>({start,end,title}));
const STORY_SCENES=SCENES.map(s=>({...s}));
const canvas=document.getElementById('film'),ctx=canvas.getContext('2d',{alpha:false}),W=1920,H=1080;
let D=189;
const images={},BG='#222122';
const clamp=(v,a=0,b=1)=>Math.max(a,Math.min(b,v)),mix=(a,b,p)=>a+(b-a)*p;
const phase=(t,a,b)=>clamp((t-a)/(b-a));
const out=p=>1-Math.pow(1-clamp(p),5);
const smooth=p=>(p=clamp(p))<.5?16*p*p*p*p*p:1-Math.pow(-2*p+2,5)/2;
const ease=(t,a,b)=>smooth(phase(t,a,b));
const C={};
const palettes=[
 ['Charcoal & blush','#222122','#323032','#2C2A2C','#F2B3C6','#58414A','#F8EFF2','#C8B7BF','#131113'],
 ['Goofy (original)','#FFFAEE','#FFFFFF','#FFFAEE','#7248D7','#EBDEFF','#31204C','#5D4973','#31204C'],
 ['Pistachio','#EEF5E9','#FFFFFF','#F7FAF2','#D85947','#F9DDD2','#27372D','#596D5D','#27372D'],
 ['Ice blue','#EAF2FF','#FFFFFF','#F5F8FF','#315BDF','#DDE6FF','#182D50','#536987','#182D50'],
 ['Lilac','#EFE8FB','#FFFFFF','#F8F4FF','#7248D7','#E6DAFF','#2C2046','#75608D','#2C2046'],
 ['Butter & ocean','#FFF8D9','#FFFDF4','#FFFAE8','#177B86','#D3EEEA','#263D40','#5F756F','#263D40'],
 ['Rose','#FBECEF','#FFFCFD','#FFF5F7','#AF3B64','#F6D4E1','#442536','#886071','#442536'],
 ['Midnight violet','#171522','#242131','#211F2C','#B89AFF','#47385F','#F2EAFB','#B9ADC8','#100E17'],
 ['Forest & tangerine','#14241E','#21382D','#1D3027','#F7A25B','#3F5743','#F3EED9','#B0C5B4','#0B1711'],
 ['Deep sea & citron','#14242C','#21343E','#1B2E37','#DDE877','#41524A','#F0F4E4','#B0C5CC','#0B171E']
];
function colorMix(a,b,p){let ca=parseInt(a.slice(1),16),cb=parseInt(b.slice(1),16);return '#'+[16,8,0].map(s=>Math.round(mix((ca>>s)&255,(cb>>s)&255,p)).toString(16).padStart(2,'0')).join('')}
function theme(t){let p=ease(t,159.3,160.3);['bg','surface','card','accent','selection','ink','muted','outline'].forEach((k,i)=>C[k]=colorMix(palettes[0][i+1],palettes[4][i+1],p));Object.assign(C,{yellow:'#FFDD59',blue:'#BAE7FF',pink:'#FFA8C9',lime:'#CCF47F'})}
function state(fn){ctx.save();try{fn()}finally{ctx.restore()}}
function at(x,y,fn){state(()=>{ctx.translate(x,y);fn()})}
function alpha(a,fn){if(a<=0)return;state(()=>{ctx.globalAlpha*=clamp(a);fn()})}
function rect(x,y,w,h,fill,r=16,stroke=null,lw=3){ctx.beginPath();ctx.roundRect(x,y,w,h,r);if(fill){ctx.fillStyle=fill;ctx.fill()}if(stroke){ctx.strokeStyle=stroke;ctx.lineWidth=lw;ctx.stroke()}}
function panel(x,y,w,h,fill=C.surface,r=24,shadow=6){if(shadow)rect(x+shadow,y+shadow,w,h,C.outline,r);rect(x,y,w,h,fill,r,C.outline,3)}
function text(s,x,y,size=22,color=C.ink,weight=400,font='Geist',align='left'){ctx.font=`${weight} ${size}px "${font}"`;ctx.fillStyle=color;ctx.textAlign=align;ctx.textBaseline='alphabetic';ctx.fillText(s,x,y)}
function line(x1,y1,x2,y2,color=C.ink,lw=2){ctx.beginPath();ctx.moveTo(x1,y1);ctx.lineTo(x2,y2);ctx.strokeStyle=color;ctx.lineWidth=lw;ctx.lineCap='round';ctx.stroke()}
function circle(x,y,r,fill,stroke=null,lw=2){ctx.beginPath();ctx.arc(x,y,r,0,Math.PI*2);if(fill){ctx.fillStyle=fill;ctx.fill()}if(stroke){ctx.strokeStyle=stroke;ctx.lineWidth=lw;ctx.stroke()}}
function image(name,x,y,w,h=w){if(images[name])ctx.drawImage(images[name],x,y,w,h)}
function logo(x,y,size=23){image('logo',x,y-size*.85,size*1.5);state(()=>{ctx.font=`${size}px Pixel`;ctx.lineJoin='round';ctx.lineWidth=4;ctx.strokeStyle=C.outline;ctx.strokeText('Azec',x+size*1.9+1.5,y+2);ctx.fillStyle=C.ink;ctx.fillText('Azec',x+size*1.9,y)})}
function chip(label,x,y,w,color=C.yellow,size=15){at(x+w/2,y+14,()=>state(()=>{ctx.rotate(-.025);panel(-w/2,-14,w,28,color,10,2);text(label,0,5,size,'#31204C',600,'Geist','center')}))}
function inkOn(hex){const n=parseInt(hex.slice(1),16);return (((n>>16)*299+((n>>8)&255)*587+(n&255)*114)/1000)>150?'#31204C':'#FFFFFF'}
function button(label,x,y,w,h=44,selected=false,size=18){panel(x,y,w,h,selected?C.accent:C.card,12,2);text(label,x+w/2,y+h/2+size*.34,size,selected?inkOn(C.accent):C.ink,500,'Geist','center')}
function number(label,x,y,w=83){rect(x,y,w,36,C.yellow,10,C.outline,2);text(label,x+w/2,y+25,19,'#31204C',600,'Geist','center')}
function toggle(x,y,p,w=62){rect(x,y,w,32,p>.5?C.accent:C.selection,16,C.outline,2.5);let cx=x+16+(w-32)*p;circle(cx,y+16,12,'#FFFFFF',C.outline,2);if(p>.5){circle(cx-3,y+13,1.1,C.outline);circle(cx+3,y+13,1.1,C.outline);ctx.beginPath();ctx.arc(cx,y+16,5,.1,Math.PI-.1);ctx.strokeStyle=C.outline;ctx.lineWidth=1.5;ctx.stroke()}else line(cx-3,y+16,cx+3,y+16,C.outline,1.8)}
function slider(x,y,w,p){rect(x,y-5,w,10,C.selection,5);rect(x,y-5,w*p,10,C.accent,5);circle(x+w*p,y,10,'#FFFFFF',C.outline,2.5)}
function saveGlyph(x,y,color=C.ink){rect(x,y,21,24,null,3,color,2);line(x+5,y,x+5,y+8,color,2);line(x+5,y+8,x+16,y+8,color,2);line(x+16,y+8,x+16,y,color,2);rect(x+5,y+15,12,9,null,1,color,2)}
function gear(x,y){state(()=>{ctx.translate(x,y);ctx.beginPath();for(let i=0;i<32;i++){let a=i*Math.PI/16,r=i%4<2?13:10;let x=Math.cos(a)*r,y=Math.sin(a)*r;i?ctx.lineTo(x,y):ctx.moveTo(x,y)}ctx.closePath();ctx.strokeStyle=C.ink;ctx.lineWidth=2;ctx.stroke();circle(0,0,4,null,C.ink,2)})}
function check(x,y,color=C.ink){line(x-5,y,x-1,y+4,color,2.5);line(x-1,y+4,x+6,y-5,color,2.5)}
function star(x,y,size=10,fill=C.yellow){ctx.beginPath();for(let i=0;i<10;i++){let a=-Math.PI/2+i*Math.PI/5,r=i%2?size*.46:size;let xx=x+Math.cos(a)*r,yy=y+Math.sin(a)*r;i?ctx.lineTo(xx,yy):ctx.moveTo(xx,yy)}ctx.closePath();ctx.fillStyle=fill;ctx.fill();ctx.strokeStyle=C.outline;ctx.lineWidth=1.7;ctx.stroke()}

function model(t){let op=t<32?'ace':'ash';if(t>=84.8&&t<87.2)op='ace';let dual=t>=38.8,secondary=(t>=39.3&&t<47)||(t>=73.2&&t<76)||(t>=144);
 let v=op==='ace'?6:secondary?mix(12,8.4,ease(t,42,43.1)):mix(12,12.8,ease(t,53,54.3))+.1*ease(t,57.2,57.7);
 let h=op==='ace'?0:secondary?mix(1.2,.6,ease(t,43.3,44.2)):mix(1.2,-2.4,ease(t,55,56.2));
 if(secondary&&t>=148)v=mix(8.4,8.8,ease(t,148,149));
 let rapid=op==='ash'&&(secondary?t>=146.2:t>=69),recoil=t>=61.2;
 let dirty=(t>=38.8&&t<82.6)||(t>=146.2&&t<153.2)||(t>=173&&t<174.2);
 return {op,dual,secondary,v,h,rapid,recoil,dirty,delay:Math.round(mix(80,65,ease(t,70.4,71.5))),favorite:t>=165.2};
}
function rapidTween(t){let value=0;for(const [start,target] of [[69,1],[73.2,0],[76,1],[84.8,0],[87.2,1],[144,0],[146.2,1]]){if(t<start)break;value=mix(value,target,ease(t,start,start+.4))}return value}
function section(x,y,w,h,title,color){panel(x,y,w,h,C.card,19,0);chip(title.toUpperCase(),x+18,y+13,Math.max(114,title.length*9+25),color,14)}
function appHeader(m){logo(22,44,23);button(m.recoil?'Recoil on':'Recoil off',465,18,116,36,m.recoil,16);button(m.rapid?'Rapid fire on':'Rapid fire off',590,18,142,36,m.rapid,16);saveGlyph(759,23,m.dirty?C.accent:C.ink);gear(820,35);line(866,35,878,35,C.muted,2);line(913,29,925,41,C.muted,2);line(925,29,913,41,C.muted,2)}
function activePage(t){if(t>=9.2&&t<11)return 'General';if(t>=11&&t<28)return 'Mouse setup';if(t>=91&&t<93)return 'General';if(t>=93&&t<110.3)return 'Shortcuts';if(t>=110.3&&t<127.5)return 'General';if(t>=127.5&&t<141)return 'Phone control';if(t>=156.7&&t<157.7)return 'General';if(t>=157.7&&t<162)return 'Appearance';if(t>=167.4&&t<168.3)return 'General';if(t>=168.3&&t<175.7)return 'Profile backup';if(t>=175.7&&t<186)return 'General';return null}
const roster=[['ace','Ace'],['alibi','Alibi'],['amaru','Amaru'],['aruni','Aruni'],['ash','Ash'],['azami','Azami'],['bandit','Bandit']];
function sidebar(t,m,page){button('Collapse',15,85,187,36,false,16);if(page){['General','Appearance','Shortcuts','Phone control','Profile backup','Mouse setup'].forEach((s,i)=>{let y=146+i*61;if(s===page)rect(13,y-28,191,48,C.selection,13,C.outline,2);text(s,30,y+2,19,C.ink,s===page?600:400)});return}
 let search=t>=29&&t<33.5,typed=t<30?'':t<30.25?'A':t<30.5?'As':'Ash';
 let yy=159;if(m.favorite){chip('FAVORITES',24,yy,133,C.yellow);yy+=47;row('ash','Ash',yy,m.op==='ash',true);yy+=67}
 chip('ALL OPERATORS',24,yy,165,C.blue);yy+=44;
 let list=search&&typed?roster.filter(v=>v[1].toLowerCase().startsWith(typed.toLowerCase())):roster;
 for(const [name,label] of list){if(m.favorite&&name==='ash')continue;if(yy>605)break;row(name,label,yy,m.op===name);yy+=57}
 rect(12,630,192,38,C.surface,10,C.outline,2);text(search?typed:'Search operators...',24,655,15,search?C.ink:C.muted);if(search&&t<32.5)line(29+typed.length*9,639,29+typed.length*9,659,C.accent,2);
 function row(name,label,y,selected,favorite=false){if(selected)rect(12,y-5,192,50,C.selection,14,C.outline,2);image(name,20,y,43);text(label,78,y+29,19,C.ink,500);if(favorite)star(183,y+19,8)}
}
function mainPage(t,m){image(m.op,255,91,46);text(m.op==='ace'?'Ace':'Ash',315,115,30,C.ink,500,'Fredoka');text('Operator settings',315,139,15,C.muted);
 let scroll=118*ease(t,66,67)*(1-ease(t,80,80.8));
 state(()=>{ctx.beginPath();ctx.rect(244,153,692,493);ctx.clip();ctx.translate(0,-scroll);
 section(251,162,670,123,'Weapon profiles',C.yellow);text('Separate secondary profile',270,221,19,C.ink,500);toggle(837,200,ease(t,38.4,38.8));
 if(m.dual){button('1  Primary',271,239,170,34,!m.secondary,17);button('2  Secondary',451,239,180,34,m.secondary,17)}
 section(251,300,670,215,'Recoil control',C.blue);text('Vertical force',271,365,19,C.ink,500);slider(491,359,286,clamp(m.v/25));number(m.v.toFixed(1),817,340);
 text('Horizontal adjust',271,425,19,C.ink,500);slider(491,419,286,clamp((m.h+20)/40));number(m.h.toFixed(1),817,400);text('Humanize movement',271,479,18,C.ink,500);toggle(837,457,0);
 section(251,531,670,167,'Rapid fire',C.pink);text('Rapid fire',271,595,19,C.ink,500);toggle(837,573,rapidTween(t));text('Shot delay',271,649,19,C.ink,500);number(m.delay+' ms',807,625,93);slider(478,648,295,(m.delay-30)/120);
 });
 if(t>=82.6&&t<84.4){panel(643,65,265,43,C.surface,12,3);text('Configuration saved',775,93,18,C.ink,500,'Geist','center')}
 if(t>=163.7&&t<165.2){panel(116,455,168,45,C.surface,12,4);star(139,478,10);text('Favorite',160,484,19,C.ink,500)}
}
function settings(t,m,page){text('‹',253,122,36,C.ink,400);text('Settings',292,120,32,C.ink,500,'Fredoka');state(()=>{ctx.beginPath();ctx.rect(244,149,692,497);ctx.clip();
 if(page==='Mouse setup')mousePage(t);if(page==='Shortcuts')shortcuts(t);if(page==='General')general(t);if(page==='Phone control')pairing(t);if(page==='Appearance')appearance(t);if(page==='Profile backup')backup(t);
})}
function rowField(label,y,value,width=126){text(label,273,y+25,21,C.ink,500);number(value,777,y,width)}
function mousePage(t){section(252,161,669,423,'Mouse setup',C.blue);rowField('Mouse DPI',215,t<16?'—':'800');rowField('Horizontal sensitivity',274,t<14?'—':t<20.2?'12':'14');rowField('Vertical sensitivity',333,t<14?'—':t<20.2?'12':'14');rowField('Sensitivity multiplier',392,t<14?'—':'0.020');button('Read game settings',273,454,254);button('Choose INI file',545,454,228);button('Save starting values',273,520,277,t<18?44:44,t>=18&&t<19.5);button('Rescale profiles',568,520,273);
 if(t>=18.3&&t<20){text('Starting values saved.',273,616,19,C.lime,500)}
 if(t>=22&&t<25){modal('Rescale profiles',()=>{text('Horizontal × 0.86',381,329,22);text('Vertical × 0.86',381,368,22);button('Cancel',381,411,155);button('Rescale',556,411,155,44,true)})}
}
function shortcuts(t){section(252,161,669,467,'Shortcuts',C.yellow);let labels=['Open / hide window','Recoil toggle','Panic / exit key','Primary weapon','Secondary weapon'];let keys=['Insert',t>=100.3?'F8':'F6','End','1','2'];labels.forEach((s,i)=>{let y=218+i*77;text(s,273,y+27,21,C.ink,500);button(keys[i],765,y,129,41,false,20)});
 if((t>=96.8&&t<100.6)||(t>=104&&t<107.8)){let duplicate=t>=104;modal('Recoil toggle',()=>{text(duplicate?'This key is already assigned.':'Press a key now',546,332,21,duplicate?C.pink:C.muted,500,'Geist','center');button(duplicate?'Insert':t>=99?'F8':'…',474,353,142,52,false,27);text(duplicate?'Choose another key.':'Esc cancels',546,445,17,C.muted,400,'Geist','center')})}
}
function general(t){section(252,161,669,254,'General',C.blue);text('Only in Rainbow Six',273,229,21,C.ink,500);toggle(834,204,1);text('Language',273,298,21,C.ink,500);button('English (US)',716,272,177,40,false,18);text('Feedback sound',273,365,21,C.ink,500);toggle(834,341,1);
 button('Quick guide',273,442,187);button('Replay tour',480,442,190);section(252,513,669,119,'Updates',C.yellow);button('Check for updates',273,566,276);
}
function qr(x,y,size){let n=QR.length,q=4,u=size/(n+q*2);rect(x,y,size,size,'#FFFFFF',10,C.outline,2);ctx.fillStyle='#211A29';for(let r=0;r<n;r++)for(let c=0;c<n;c++)if(QR[r][c])ctx.fillRect(x+(c+q)*u,y+(r+q)*u,u+.12,u+.12)}
function pairing(t){section(252,161,669,460,'Phone control',C.blue);text('Enable phone access',273,228,21,C.ink,500);toggle(834,204,1);rect(273,253,619,43,C.surface,10,C.outline,2);text('http://192.168.1.20:18764',292,282,21,C.ink,500);text('Pairing code',273,337,20,C.muted);number('123456',425,310,133);button('Copy code',576,308,143,40,false,17);button('Copy link',731,308,162,40,false,17);
 const reveal=t>=141?0:ease(t,132,132.5);qr(273,376,222);alpha(1-reveal,()=>{rect(273,376,222,222,C.selection,10,C.outline,2);text('Click to',384,473,22,C.ink,500,'Geist','center');text('reveal QR',384,506,22,C.ink,500,'Geist','center')});button('New code',546,391,179,42,false,18);
}
function appearance(t){const yBase=174;palettes.forEach((p,i)=>{let x=263+i%3*216,y=yBase+Math.floor(i/3)*114;rect(x,y,201,102,p[1],15,(t<159.6?i===0:i===4)?C.accent:C.outline,(t<159.6?i===0:i===4)?3:2);rect(x+15,y+15,52,44,p[2],9,p[8],1.5);rect(x+25,y+24,31,6,p[4],3);rect(x+25,y+37,23,5,p[5],3);text(p[0],x+13,y+84,14,p[6],500);if((t<159.6?i===0:i===4)){circle(x+178,y+23,11,p[4]);check(x+178,y+23,inkOn(p[4]))}})}
function backup(t){section(252,161,669,220,'Profile backup',C.pink);button('Export profiles',273,225,294);button('Import profiles',585,225,310);if(t>=169.8&&t<171.3){text('Profiles exported.',273,329,21,C.lime,500)}if(t>=173){text('Profiles imported. Save to keep the changes.',273,329,20,C.ink,500)}if((t>=168.7&&t<169.7)||(t>=171.8&&t<173)){modal(t<170?'Export profiles':'Import profiles',()=>{text('Azec-profiles.json',381,333,24,C.ink,500);button(t<170?'Save':'Open',554,406,171,44,true)})}}
function modal(title,content){rect(235,77,710,591,'#13111399',24);panel(346,237,409,248,C.surface,23,7);text(title,373,281,27,C.ink,500,'Fredoka');content()}
function otherApp(t){let a=ease(t,117,117.6)*(1-ease(t,121.1,121.7));alpha(a,()=>{panel(494,298,411,248,'#F4F2F3',18,8);text('Notepad',518,336,23,'#282228',500);line(510,353,887,353,'#D2CBD0',2);line(820,323,831,323,'#625662',1.5);line(867,317,879,329,'#625662',1.5);line(879,317,867,329,'#625662',1.5);line(520,380,520,403,'#625662',1.5)})}

function framing(t){let p=ease(t,126,127.2)*(1-ease(t,155.9,157.1));return {x:mix(408,90,p),y:mix(205,252,p),s:mix(1.15,1.04,p),phone:p}}
const PAGE_CUTS=[9.2,11,28,91,93,110.3,127.5,141,156.7,157.7,162,167.4,168.3,175.7];
function app(t,m){let f=framing(t);alpha(ease(t,1.8,2.8)*(1-ease(t,185.6,186.4)),()=>at(f.x,f.y,()=>state(()=>{ctx.scale(f.s,f.s);panel(0,0,960,688,C.bg,26,9);appHeader(m);let page=activePage(t),cut=PAGE_CUTS.find(v=>t>=v&&t<v+.4);panel(228,76,716,596,C.surface,25,7);const content=p=>{sidebar(t,m,p);if(p)settings(t,m,p);else mainPage(t,m)};if(cut!==undefined){let blend=phase(t,cut,cut+.4);alpha(1-blend,()=>content(activePage(cut-.001)));alpha(blend,()=>content(page))}else content(page);if(t>=109&&t<126)otherApp(t);appPointer(t,page,m)})))}
function phone(t,m){let a=framing(t).phone;if(a<=0)return;alpha(a,()=>at(1325+60*(1-a),198+20*(1-a),()=>{
 panel(0,0,390,782,C.outline,43,8);rect(8,8,374,766,C.bg,36,C.outline,2);rect(137,18,116,24,C.outline,13);logo(24,88,23);gear(347,76);line(16,112,374,112,C.outline,2);
 if(t<139.4){text('Pair with your PC',195,216,31,C.ink,500,'Fredoka','center');text('Pairing code',30,280,22,C.ink,500);rect(27,301,336,64,C.surface,14,C.outline,3);text(t>=137.6?'123456':'',195,345,34,C.ink,500,'Geist','center');button('Connect',27,390,336,51,true,23);return}
 section(21,132,348,163,'Output',C.lime);text('Recoil active',39,204,20,C.ink,500);toggle(285,179,m.recoil?1:0);text('Rapid fire active',39,263,20,C.ink,500);toggle(285,238,rapidTween(t));
 image(m.op,27,316,48);text(m.op==='ash'?'Ash':'Ace',91,347,31,C.ink,500,'Fredoka');button('Primary',24,382,164,40,!m.secondary,19);button('Secondary',200,382,166,40,m.secondary,19);
 section(21,445,348,207,'Recoil control',C.blue);text('Vertical force',39,509,20,C.ink,500);number(m.v.toFixed(1),278,483,73);slider(43,544,302,clamp(m.v/25));text('Horizontal adjust',39,593,19,C.ink,500);number(m.h.toFixed(1),278,567,73);slider(43,628,302,clamp((m.h+20)/40));
 button('Save changes',24,686,342,53,m.dirty,22);rect(146,757,99,5,C.muted,3);
 phonePointer(t);
 }))}

// A pointer drawn inside the artwork; it never controls the system cursor.
function pointer(x,y,pressed=0){state(()=>{ctx.translate(x,y);ctx.scale(1-.07*pressed,1-.07*pressed);ctx.beginPath();ctx.moveTo(0,0);ctx.lineTo(0,27);ctx.lineTo(7,20);ctx.lineTo(13,33);ctx.lineTo(20,29);ctx.lineTo(14,17);ctx.lineTo(24,17);ctx.closePath();ctx.fillStyle='#FFFFFF';ctx.fill();ctx.strokeStyle='#131113';ctx.lineWidth=2;ctx.lineJoin='round';ctx.stroke()})}
function track(t,points){if(!points.length||t<points[0][0]-.8||t>points.at(-1)[0]+1)return;let last=points[0],x=last[1]+30,y=last[2]+70,press=0;for(let i=0;i<points.length;i++){let p=points[i],prev=i?points[i-1]:[p[0]-1,p[1]+30,p[2]+70];if(t>=prev[0]){let k=ease(t,Math.max(prev[0]+.28,p[0]-.9),p[0]-.08);x=mix(prev[1],p[1],k);y=mix(prev[2],p[2],k)}if(p[3]!==false&&t>=p[0]-.08&&t<=p[0]+.16)press=1-Math.abs((t-p[0]-.04)/.12);if(t<p[0])break;last=p}let a=ease(t,points[0][0]-.8,points[0][0]-.5)*(1-ease(t,points.at(-1)[0]+.4,points.at(-1)[0]+1));alpha(a,()=>pointer(x,y,Math.max(0,press)))}
function appPointer(t,page,m){let p=[];
 if(t>=8&&t<27)p=[[9.2,820,35],[11,106,453],[14,405,476],[16,835,231],[18,399,543],[20.2,838,290],[22,697,541],[24.8,448,434]];
 if(t>=27&&t<36)p=[[28,261,110],[29.1,91,650],[32,99,223]];
 if(t>=36&&t<51)p=[[38.8,869,215],[40,540,254],[43.1,590,359],[44.2,634,419],[47,350,254]];
 if(t>=51&&t<66)p=[[53,626,359],[54.3,637,359],[56.2,616,419],[57.4,857,358],[61.2,525,36]];
 if(t>=66&&t<80)p=[[69,865,473],[70.4,601,530],[71.5,567,530],[73.2,540,136],[76,350,136]];
 if(t>=80&&t<90)p=[[82.6,771,35],[84.8,97,218],[87.2,97,445]];
 if(t>=90&&t<109)p=[[91,820,35],[93,108,271],[96.8,826,316],[104,826,316]];
 if(t>=109&&t<126)p=[[110.3,110,148],[113,864,218,false],[123,804,292,false]];
 if(t>=126&&t<141)p=[[127.5,117,331],[132,385,485],[135.5,661,329]];
 if(t>=156&&t<167)p=[[156.7,820,35],[157.7,106,209],[159.5,580,339],[162,260,110],[163.7,103,445],[165.2,196,478]];
 if(t>=167&&t<175)p=[[167.4,820,35],[168.3,114,394],[169,414,246],[171.8,739,246],[173,639,429],[174.2,770,35]];
 if(t>=175&&t<186)p=[[175.7,109,149],[179,419,586,false],[183,578,463,false]];
 track(t,p);
}
function phonePointer(t){track(t,t<141?[[137.6,188,333],[139.4,184,415]]:[[144,278,400],[146.2,318,254],[148,154,544],[149,161,544],[153.2,193,713]])}

function chapter(t){let scene=STORY_SCENES.find(s=>t>=s.start&&t<s.end)||STORY_SCENES.at(-1);if(t>=186)return;let a=(scene.start===0?1:ease(t,scene.start,scene.start+.4))*(1-ease(t,scene.end-.25,scene.end));alpha(a,()=>text(scene.title,120,125,60,'#F8EFF2',500,'Fredoka'))}
function opening(t){alpha(1-ease(t,1.6,2.2),()=>{panel(831,380,258,250,C.surface,24,8);image('logo',910,413,100);text('Azec.exe',960,575,28,'#F8EFF2',500,'Geist','center');track(t,[[1.1,966,484]])})}
function endCard(t){let p=ease(t,186,186.8);alpha(p,()=>{image('logo',883,321+12*(1-p),154);state(()=>{ctx.font='88px Pixel';ctx.textAlign='center';ctx.lineJoin='round';ctx.strokeStyle='#131113';ctx.lineWidth=10;ctx.strokeText('Azec',966,634);ctx.fillStyle='#F8EFF2';ctx.fillText('Azec',960,628)})})}
function storyTime(t){const index=SCENES.findIndex(s=>t>=s.start&&t<s.end);if(index<0)return t<=0?0:189;const a=SCENES[index],b=STORY_SCENES[index];return b.start+(t-a.start)/(a.end-a.start)*(b.end-b.start)}
function draw(t){t=storyTime(clamp(Number(t)||0,0,D));ctx.setTransform(1,0,0,1,0,0);ctx.globalAlpha=1;ctx.fillStyle=BG;ctx.fillRect(0,0,W,H);theme(t);chapter(t);if(t<2.2)opening(t);const m=model(t);app(t,m);phone(t,m);if(t>=186)endCard(t)}

let playing=false,lastT=0,epoch=0,raf=0,readyDone=false,voiceUrl=null;
const play=document.getElementById('play'),seek=document.getElementById('seek'),time=document.getElementById('time'),audio=document.getElementById('voice'),chapterSelect=document.getElementById('chapter');
const clock=t=>Math.floor(t/60)+':'+String(Math.floor(t%60)).padStart(2,'0');
SCENES.forEach((s,i)=>{const o=document.createElement('option');o.value=i;o.textContent=clock(s.start)+'  '+s.title;chapterSelect.append(o)});
function controls(t){seek.value=t;time.textContent=clock(t)+' / '+clock(D);play.textContent=playing?'Pause':t>=D?'Play again':'Play';const i=SCENES.findIndex(s=>t>=s.start&&t<s.end);chapterSelect.value=i<0?SCENES.length-1:i;play.setAttribute('aria-label',playing?'Pause animation':'Play animation')}
function stop(){playing=false;cancelAnimationFrame(raf);audio.pause();controls(lastT)}
function frame(now){if(!playing)return;lastT=Math.min(D,voiceUrl&&!audio.paused?audio.currentTime:(now-epoch)/1000);draw(lastT);if(lastT>=D){playing=false;audio.pause();controls(lastT);return}controls(lastT);raf=requestAnimationFrame(frame)}
function start(t=lastT){if(!readyDone)return;cancelAnimationFrame(raf);lastT=t>=D?0:t;epoch=performance.now()-lastT*1000;playing=true;if(voiceUrl){audio.currentTime=lastT;audio.play().catch(()=>{document.getElementById('voiceStatus').textContent='Press Play to enable narration.'})}controls(lastT);raf=requestAnimationFrame(frame)}
function setTimings(boundaries){if(!Array.isArray(boundaries)||boundaries.length!==SCENES.length+1||boundaries[0]!==0||boundaries.some((v,i)=>!Number.isFinite(v)||(i&&v<=boundaries[i-1])))throw new Error('Provide 16 increasing chapter boundaries, starting at 0.');stop();SCENES.forEach((s,i)=>{s.start=boundaries[i];s.end=boundaries[i+1];chapterSelect.options[i].textContent=clock(s.start)+'  '+s.title});D=boundaries.at(-1);motion.duration=D;seek.max=D;motion.render(0);return SCENES}
window.motion={duration:D,fps:60,width:W,height:H,scenes:SCENES,setTimings,render(t){stop();lastT=clamp(Number(t)||0,0,D);if(readyDone)draw(lastT);if(voiceUrl)audio.currentTime=lastT;controls(lastT);return canvas},ready:null};
play.addEventListener('click',()=>playing?stop():start());document.getElementById('replay').addEventListener('click',()=>start(0));seek.addEventListener('input',()=>motion.render(Number(seek.value)));chapterSelect.addEventListener('change',()=>motion.render(SCENES[Number(chapterSelect.value)].start));
document.getElementById('voiceFile').addEventListener('change',event=>{let file=event.target.files[0];if(!file)return;stop();if(voiceUrl)URL.revokeObjectURL(voiceUrl);voiceUrl=URL.createObjectURL(file);audio.src=voiceUrl;document.getElementById('voiceStatus').textContent=file.name+' — scene timing is provisional.'});
window.motion.ready=(async()=>{const bytes=Uint8Array.from(atob(PACKED_ASSETS),c=>c.charCodeAt(0));const stream=new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip'));ASSETS=JSON.parse(await new Response(stream).text());await Promise.all([document.fonts.load('400 24px Geist'),document.fonts.load('500 24px Geist'),document.fonts.load('600 24px Geist'),document.fonts.load('500 48px Fredoka'),document.fonts.load('400 24px Pixel'),...Object.entries(ASSETS).map(([name,src])=>new Promise((resolve,reject)=>{const im=new Image();im.onload=()=>{images[name]=im;resolve()};im.onerror=()=>reject(new Error('Embedded asset failed: '+name));im.src=src}))]);readyDone=true;let q=new URLSearchParams(location.search);if(q.has('still'))motion.render(Number(q.get('still'))||0);else start(0);return motion})();
