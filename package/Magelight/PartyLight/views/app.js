/* PartyLight: shared HUD and layout editor; no network dependencies. */
(() => {
  'use strict';
  const editing = document.body.classList.contains('editing');
  const defaults = {enabled:true,x:.01,y:.4,scale:1,portraitSize:60,barWidth:88,barHeight:13,gap:12,portraits:true,numbers:true,horizontal:false,maxRows:4,maxColumns:2,showSummons:false,distance:true,overheadDebug:false,showCasualFollowers:false,hideDistantCasualFollowers:true,casualFollowerRange:100,onlySeverCompanions:false,excludeHorses:true,hotkey:66,shift:true,ctrl:false,alt:false,vrButton:7,vrModifier:2,vrHand:1,cards:{}};
  const clone = value => JSON.parse(JSON.stringify(value));
  let config = clone(defaults), actors = [], selected = '', drag = null, saveTimer, sliderActive=false, pendingControls=false, vrRuntime=false;
  const images = new Map(), root = document.getElementById('party');
  const bridge = window.magelight;
  const parse = value => typeof value === 'string' ? JSON.parse(value) : value;
  const clamp = (n,a,b) => Math.min(b,Math.max(a,n));
  const send = (name, value) => bridge?.send(name,value);
  const visibleActors=()=>actors.filter(a=>config.showSummons||!a.summon).slice(0,Math.floor(config.maxRows)*Math.floor(config.maxColumns));
  function save() {
    clearTimeout(saveTimer);
    saveTimer = setTimeout(() => send('partySave',config),180);
  }
  function listen(name, fn) {
    // Classic InteropCall and modern buffered channel delivery are both supported.
    window[name] = value => fn(parse(value));
    bridge?.on(name,value => fn(parse(value)));
  }
  listen('partyConfig', value => {
    if (drag || sliderActive) return;
    config = {...defaults,...value,cards:value.cards || {}}; render(); controls();
  });
  listen('partyTheme',value=>{document.documentElement.dataset.theme=typeof value==='string'?value:'ledger';});
  listen('partyRuntime',value=>{vrRuntime=Boolean(value.vr);controls();});
  listen('partyState', value => {
    const before=visibleActors().map(a=>a.key+':'+Boolean(a.canine)).join('|');
    actors = value;
    if(before!==visibleActors().map(a=>a.key+':'+Boolean(a.canine)).join('|')) {render();controls();return;}
    for(const actor of actors) {
      const card=[...root.querySelectorAll('.card')].find(c=>c.dataset.key===actor.key);
      if(!card) continue;
      card.classList.toggle('unloaded',!actor.loaded);
      card.querySelector('.name').textContent=actor.name||'Unknown';
      updateIndicators(card,actor);
      for(const stat of ['health','stamina','magicka']) {
        const data=actor[stat]||{current:0,max:0},bar=card.querySelector('.'+stat);
        if(!bar) continue;
        bar.querySelector('.fill').style.width=clamp(data.max>0?data.current/data.max*100:0,0,100)+'%';
        const label=bar.querySelector('.value');if(label) label.textContent=actor.loaded?Math.round(data.current)+'/'+Math.round(data.max):'—';
      }
    }
  });
  listen('partyPortrait', value => {
    images.set(value.id,value); render();
  });
  listen('partyReset', () => { actors=[]; images.clear(); selected=''; render(); controls(); });
  const make = (tag, className, text) => {
    const el=document.createElement(tag); if(className) el.className=className;
    if(text!==undefined) el.textContent=text; return el;
  };
  function icon(kind,title) {
    const paths={combat:'M3 21L18 6M14 3L21 3L21 10M3 14L10 21M2 22L6 18',hunger:'M3 11H21C21 20 3 20 3 11ZM6 3V7M12 2V7M18 3V7M7 21H17',fatigue:'M20 15A9 9 0 0 1 9 4A9 9 0 1 0 20 15Z',cold:'M12 2V22M3 7L21 17M3 17L21 7M8 4L12 7L16 4M8 20L12 17L16 20',arrow:'M12 3L5 11H9V21H15V11H19Z',
      paw:'M7 16Q12 9 17 16Q21 23 12 20Q3 23 7 16ZM4 9A2 3 0 1 0 8 9A2 3 0 1 0 4 9ZM9 5A2 3 0 1 0 13 5A2 3 0 1 0 9 5ZM16 9A2 3 0 1 0 20 9A2 3 0 1 0 16 9Z',nap:'M3 8H12L3 19H12M14 3H22L14 12H22',
      hunt:'M12 2V7M12 17V22M2 12H7M17 12H22M5 12A7 7 0 1 0 19 12A7 7 0 1 0 5 12Z',
      sniff:'M3 10Q7 4 14 8L20 11L15 16L10 14M17 4Q23 3 21 7M18 19Q22 22 23 18',
      play:'M3 12A9 9 0 1 0 21 12A9 9 0 1 0 3 12ZM5 6Q12 10 8 20M19 5Q12 12 19 18',
      fish:'M3 12Q9 1 18 12Q9 23 3 12ZM18 12L23 6V18ZM8 10H9',
      bone:'M7 8Q1 9 2 5Q4 1 7 5L17 15Q22 14 22 19Q20 23 17 19L7 9Z',
      greet:'M5 14Q12 8 18 15Q21 23 12 20Q4 23 5 14ZM4 7V4M10 6V2M16 7V3M20 12Q25 7 22 4',
      warm:'M12 2Q14 8 19 12Q25 22 12 22Q0 22 5 12L8 7L10 13Q15 11 12 2Z'};
    const span=make('span','status-icon '+kind);span.title=title;
    const svg=document.createElementNS('http://www.w3.org/2000/svg','svg');svg.setAttribute('viewBox','0 0 24 24');
    const path=document.createElementNS(svg.namespaceURI,'path');path.setAttribute('d',paths[kind]);path.setAttribute('fill','none');path.setAttribute('stroke','currentColor');path.setAttribute('stroke-width','2');path.setAttribute('stroke-linejoin','round');svg.append(path);span.append(svg);return span;
  }
  const dogActivities={0:['paw','Activity unavailable'],1:['paw','Awake'],2:['hunger','Eating'],3:['nap','Napping'],4:['hunt','Hunting'],5:['sniff','Sniffing'],6:['play','Playing'],7:['fish','Fishing'],8:['bone','Scavenging'],9:['greet','Greeting'],10:['warm','Warming'],11:['combat','Fighting']};
  function updateCanine(card,actor) {
    const row=card.querySelector('.canine-status'),dog=actor.canine;
    if(!row||!dog) return;
    for(const [kind,warning,value,tier] of [['dog-hunger',dog.hungerWarning,dog.hunger,dog.hungerTier],['dog-tired',dog.tiredWarning,dog.tiredness,dog.tiredTier]]) {
      const badge=row.querySelector('.'+kind);
      badge.style.visibility=warning?'visible':'hidden';
      badge.dataset.tier=String(tier);
      badge.title='Canine Drives '+(kind==='dog-hunger'?'hunger':'tiredness')+': '+Number(value).toFixed(1)+'/100 · onset '+dog.threshold+' (includes hysteresis)';
    }
    const activity=row.querySelector('.dog-activity');
    if(activity.dataset.activity!==String(dog.activity)) {
      const [kind,label]=dogActivities[dog.activity]||['paw','Other activity'];
      activity.replaceChildren(icon(kind,'Canine Drives: '+label));
      activity.dataset.activity=String(dog.activity);activity.title='Canine Drives: '+label;
      activity.classList.toggle('unknown',dog.activity===0);
    }
  }
  function updateIndicators(card,actor) {
    card.querySelector('.combat').hidden=!actor.combat;
    const needs=actor.needs||[];
    ['hunger','fatigue','cold'].forEach((kind,i)=>{card.querySelector('.'+kind).hidden=!needs[i];});
    updateCanine(card,actor);
    const travel=card.querySelector('.travel');
    if(travel) {
      const known=Number.isFinite(actor.distance), heading=Number.isFinite(actor.heading);
      travel.querySelector('.distance-text').textContent=known?(actor.distance>=1000?(actor.distance/1000).toFixed(1)+' km':Math.round(actor.distance)+' m'):'Elsewhere';
      const arrow=travel.querySelector('.arrow');arrow.hidden=!heading;
      arrow.style.transform=`rotate(${heading?actor.heading:0}deg)`;
      travel.title=known?'Distance and direction relative to player facing':'Follower is in another interior or worldspace';
    }
  }
  function makeCard(actor) {
    const per = config.cards[actor.key] || {};
    const width = config.barWidth;
    const card=make('div','card'+(!actor.loaded?' unloaded':'')+(selected===actor.key?' selected':''));
    card.dataset.key=actor.key;
    card.style.width=width+'px';
    const showPortrait=per.portraits ?? config.portraits;
    if(showPortrait) {
      const picture=images.get(actor.id), portrait=make('div','portrait'+(picture?.kind===2?' cutout':''));
      portrait.style.width=portrait.style.height=config.portraitSize+'px';
      if(picture?.src) {
        const img=make('img'); img.src=picture.src; img.alt='';
        img.onerror=()=>{img.remove();portrait.append(make('div','monogram',actor.name?.slice(0,1)||'?'));};
        portrait.append(img);
      } else portrait.append(make('div','monogram',actor.name?.slice(0,1)||'?'));
      card.append(portrait);
    }
    const nameRow=make('div','name-row');nameRow.append(icon('combat','In combat'),make('span','name',actor.name||'Unknown'));
    for(const [kind,title] of [['hunger','Hunger penalty active'],['fatigue','Fatigue penalty active'],['cold','Cold penalty active']]) nameRow.append(icon(kind,title));
    card.append(nameRow);
    const bars=make('div','bars');
    for (const stat of ['health','stamina','magicka']) {
      if(stat==='magicka' && actor.canine) continue;
      const data=actor[stat] || {current:0,max:0}, bar=make('div','bar '+stat), fill=make('div','fill');
      const current=Number(data.current)||0,max=Number(data.max)||0;
      bar.style.height=bar.style.lineHeight=config.barHeight+'px';
      fill.style.width=clamp(max>0 ? current/max*100:0,0,100)+'%'; bar.append(fill);
      if(config.numbers) bar.append(make('span','value',actor.loaded ? Math.round(current)+'/'+Math.round(max) : '—'));
      bar.title=stat+': '+Math.round(current)+' / '+Math.round(max); bars.append(bar);
    }
    card.append(bars);
    if(actor.canine) {
      const dogRow=make('div','canine-status');
      for(const [kind,symbol] of [['dog-hunger','hunger'],['dog-tired','fatigue']]) {const badge=make('span','dog-badge '+kind);badge.append(icon(symbol,''));dogRow.append(badge);}
      dogRow.append(make('span','dog-badge dog-activity'));card.append(dogRow);
    }
    if(config.distance) {const travel=make('div','travel');travel.append(icon('arrow','Direction relative to player'),make('span','distance-text'));card.append(travel);}
    updateIndicators(card,actor);
    if(editing) card.addEventListener('pointerdown',event=>startDrag(event,actor));
    return card;
  }
  function render() {
    if(drag) return;
    root.replaceChildren();
    const shown=visibleActors();
    if(editing) document.getElementById('empty').hidden=shown.length>0;
    if(!config.enabled && !editing) return;
    const group=make('div','party-group horizontal');
    const scale=config.scale;
    group.style.left=(config.x*innerWidth)+'px'; group.style.top=(config.y*innerHeight)+'px';
    group.style.gap=config.gap+'px'; group.style.transform=`scale(${scale})`;
    if(editing && shown.length) {
      const handle=make('div','group-handle','✥ Move party');
      handle.addEventListener('pointerdown',event=>startDrag(event,null)); group.append(handle);
    }
    root.append(group);
    const columns=[];
    const columnCount=Math.min(config.maxColumns,config.horizontal?shown.length:Math.ceil(shown.length/config.maxRows));
    for(let i=0;i<columnCount;i++){const column=make('div','stack-column');column.style.gap=config.gap+'px';group.append(column);columns.push(column);}
    for(const [index,actor] of shown.entries()) {
      const per=config.cards[actor.key] || {}, card=makeCard(actor);
      if(per.x!==undefined && per.y!==undefined) {
        card.classList.add('free');
        card.style.left=per.x*innerWidth+'px'; card.style.top=per.y*innerHeight+'px';
        card.style.transform=`scale(${scale*(per.scale||1)})`; root.append(card);
      } else {
        // A wrapper reserves the scaled footprint in the party's flow.
        const slot=make('div'); const localScale=per.scale||1;
        card.style.transform=`scale(${localScale})`;
        const column=config.horizontal?index%columnCount:Math.floor(index/config.maxRows);
        slot.append(card); columns[column].append(slot);
        slot.style.width=config.barWidth*localScale+'px';
        slot.style.height=card.offsetHeight*localScale+'px';
      }
    }
    // Keep the stacked HUD reachable after resizing, changing scale or adding followers.
    const rect=group.getBoundingClientRect();
    group.style.left=clamp(config.x*innerWidth,0,Math.max(0,innerWidth-rect.width))+'px';
    group.style.top=clamp(config.y*innerHeight,editing?32:0,Math.max(editing?32:0,innerHeight-rect.height))+'px';
    for(const card of root.querySelectorAll('.free')) {
      const rect=card.getBoundingClientRect();
      card.style.left=clamp(parseFloat(card.style.left),0,Math.max(0,innerWidth-rect.width))+'px';
      card.style.top=clamp(parseFloat(card.style.top),0,Math.max(0,innerHeight-rect.height))+'px';
    }
  }
  function startDrag(event, actor) {
    if(event.button!==0) return;
    event.preventDefault();
    if(actor) selected=actor.key;
    const element=event.currentTarget, rect=element.getBoundingClientRect();
    drag={actor,startX:event.clientX,startY:event.clientY,x:actor?rect.left:config.x*innerWidth,y:actor?rect.top:config.y*innerHeight,width:actor?rect.width:0,height:actor?rect.height:0};
    if(!actor) { const r=element.parentElement.getBoundingClientRect(); drag.x=r.left;drag.y=r.top;drag.width=r.width;drag.height=r.height; }
    controls(); document.body.classList.add('dragging');
  }
  addEventListener('pointermove',event=>{
    if(!drag) return;
    const x=clamp(drag.x+event.clientX-drag.startX,0,Math.max(0,innerWidth-drag.width))/innerWidth;
    const y=clamp(drag.y+event.clientY-drag.startY,0,Math.max(0,innerHeight-drag.height))/innerHeight;
    if(drag.actor) config.cards[drag.actor.key]={...(config.cards[drag.actor.key]||{}),x,y};
    else { const dx=x-config.x,dy=y-config.y; config.x=x;config.y=y;
      for(const per of Object.values(config.cards)) if(per.x!==undefined){per.x=clamp(per.x+dx,0,.98);per.y=clamp(per.y+dy,0,.98);}
    }
    const current=drag; drag=null; render(); drag=current;
  });
  const endDrag=()=>{if(drag){drag=null;document.body.classList.remove('dragging');render();save();}};
  addEventListener('pointerup',endDrag);addEventListener('pointercancel',endDrag);addEventListener('blur',endDrag);
  addEventListener('resize',render);
  function controls() {
    if(!editing) return;
    if(sliderActive){pendingControls=true;return;}
    const parent=document.getElementById('controls');parent.replaceChildren();
    const section=text=>parent.append(make('div','section',text));
    function checkbox(key,label,target=config) {
      const row=make('div','control'),caption=make('label',null,label),input=make('input');
      input.type='checkbox';input.checked=target[key] ?? config[key];
      input.onchange=()=>{target[key]=input.checked;render();save();};caption.append(input);row.append(caption);parent.append(row);
    }
    function range(key,label,min,max,step,target=config) {
      const row=make('div','control'),caption=make('label',null,label),input=make('input'),out=make('output');
      input.type='range';input.min=min;input.max=max;input.step=step;input.value=target[key]??(key==='scale'?1:config[key]);out.value=input.value;
      input.oninput=()=>{target[key]=Number(input.value);out.value=input.value;render();save();};
      const update=event=>{const rect=input.getBoundingClientRect();const fraction=clamp((event.clientX-rect.left-9)/(rect.width-18),0,1);input.value=String(min+Math.round(fraction*(max-min)/step)*step);input.oninput();};
      input.onpointerdown=event=>{if(event.button!==0)return;event.preventDefault();sliderActive=true;input.setPointerCapture(event.pointerId);update(event);};
      input.onpointermove=event=>{if(sliderActive&&input.hasPointerCapture(event.pointerId))update(event);};
      input.onpointerup=input.onpointercancel=event=>{if(!sliderActive)return;sliderActive=false;if(input.hasPointerCapture(event.pointerId))input.releasePointerCapture(event.pointerId);save();if(pendingControls){pendingControls=false;controls();}};
      row.append(caption,input,out);parent.append(row);
    }
    section('Party layout');checkbox('enabled','Show follower HUD');checkbox('portraits','Show portraits');checkbox('numbers','Show current / maximum');checkbox('horizontal','Fill across before down');checkbox('showSummons','Show minions and summons');checkbox('distance','Show distance and direction');
    range('maxRows','Maximum vertical cards',1,12,1);range('maxColumns','Maximum horizontal cards',1,12,1);
    range('scale','Overall size',.5,3,.05);range('portraitSize','Portrait size',24,140,1);range('barWidth','Bar width',55,240,1);range('barHeight','Bar height',8,28,1);range('gap','Card spacing',0,80,1);
    section('Follower selection');
    checkbox('excludeHorses','Exclude horses');
    checkbox('onlySeverCompanions','Only active SeverActions companions');
    checkbox('showCasualFollowers','Show SeverActions casual followers');
    checkbox('hideDistantCasualFollowers','Hide casual followers beyond range');
    range('casualFollowerRange','Casual follower range (metres)',10,500,10);
    parent.append(make('p','hint','Casual followers default off. Their distance limit defaults on at 100 m; unloaded actors and other interiors/worldspaces are outside range. Only active SeverActions companions overrides casual followers, independent vanilla/quest followers and summons. Recruited companions at camp remain eligible. Changes apply on the next follower scan.'));
    if(!vrRuntime) {
      section('Dog debugging');checkbox('overheadDebug','Hjalti overhead debug icons');
      parent.append(make('p','hint','Optional Status Indicator Framework icons above Hjalti: hunger, tiredness and current activity. Off by default.'));
    }
    section('Selected follower');
    const choose=make('select');choose.append(new Option('Choose a follower…',''));
    for(const a of actors) choose.append(new Option(a.name,a.key));choose.value=selected;
    choose.onchange=()=>{selected=choose.value;render();controls();};parent.append(choose);
    if(selected && actors.some(a=>a.key===selected)) {
      const per=config.cards[selected] ||= {};
      checkbox('portraits','Show this portrait',per);range('scale','Follower size',.5,3,.05,per);
      const restore=make('button',null,'Return to party stack');restore.onclick=()=>{delete config.cards[selected];render();controls();save();};parent.append(restore);
    }
    if(vrRuntime) {
      section('VR settings shortcut');
      const dropdown=(key,label,choices)=>{const row=make('div','control'),select=make('select');row.append(make('label',null,label));for(const [text,value] of choices)select.append(new Option(text,String(value)));select.value=String(config[key]);select.onchange=()=>{config[key]=Number(select.value);save();};row.append(select);parent.append(row);};
      dropdown('vrButton','Button',[['A / X',7],['B / Y',1],['Stick click',32],['Trigger',33],['Touchpad',35]]);
      dropdown('vrModifier','Hold modifier',[['Grip',2],['None',0]]);
      dropdown('vrHand','Controller',[['Left',1],['Right',2],['Either',0]]);
      parent.append(make('p','hint','Default: hold left Grip and press X (A/X). The same chord closes settings. Use the Magelight laser to click and drag.'));
    }
    section(vrRuntime?'Keyboard fallback':'Settings hotkey');
    const keys=[...Array.from({length:10},(_,i)=>['F'+(i+1),59+i]),['F11',87],['F12',88],['Home',199],['End',207],['Insert',210],['Delete',211],['Pause',197],...Object.entries({A:30,B:48,C:46,D:32,E:18,F:33,G:34,H:35,I:23,J:36,K:37,L:38,M:50,N:49,O:24,P:25,Q:16,R:19,S:31,T:20,U:22,V:47,W:17,X:45,Y:21,Z:44})];
    const keySelect=make('select');for(const [label,scan] of keys) keySelect.append(new Option(label,String(scan)));
    keySelect.value=String(Math.round(config.hotkey));keySelect.onchange=()=>{config.hotkey=Number(keySelect.value);save();};parent.append(keySelect);
    checkbox('shift','Require Shift');checkbox('ctrl','Require Ctrl');checkbox('alt','Require Alt');
    parent.append(make('p','hint','Both left and right modifier keys work. Unchecked modifiers must be released. The new binding applies immediately.'));
  }
  if(editing) {
    document.getElementById('close').onclick=()=>{clearTimeout(saveTimer);send('partySave',config);send('partyClose','');};
    document.getElementById('reset').onclick=()=>{config={...clone(defaults),hotkey:config.hotkey,shift:config.shift,ctrl:config.ctrl,alt:config.alt};selected='';render();controls();save();};
    addEventListener('keydown',event=>{if(event.key==='Escape'){clearTimeout(saveTimer);send('partySave',config);send('partyClose','');}});
    // Save the final slider/drag value before the host handles Escape or the toggle key.
    bridge?.on('__uimode',value=>{if(String(value)==='0'){endDrag();clearTimeout(saveTimer);send('partySave',config);}});
  }
  render();controls();send('__prepaint','1');
})();
