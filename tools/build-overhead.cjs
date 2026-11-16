const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const {chromium}=require(process.env.PARTYLIGHT_PLAYWRIGHT||'playwright');
const root=path.resolve(__dirname,'..'),out=path.join(root,'verification/overhead');
fs.mkdirSync(out,{recursive:true});
const source=fs.readFileSync(path.join(root,'package/Magelight/PartyLight/views/app.js'),'utf8');
const paths=vm.runInNewContext('('+source.match(/const paths=(\{[\s\S]*?\});/)[1]+')');
const specs=[['PartyLightDogHungry','partyLightDogHungry','hunger'],['PartyLightDogTired','partyLightDogTired','fatigue'],
 ...['paw','paw','hunger','nap','hunt','sniff','play','fish','bone','greet','warm','combat'].map((kind,i)=>['PartyLightDogActivity'+i,'partyLightDogActivity'+i,kind]),
 ['PartyLightDogActivityOther','partyLightDogActivityOther','paw']];
(async()=>{
 const browser=await chromium.launch({headless:true});const page=await browser.newPage({viewport:{width:72,height:72}});
 const images=[];
 for(const [label,condition,kind] of specs){
  const color=condition.includes('Hungry')||condition.includes('Tired')?'#f6d88e':'#fff0cf';
  await page.setContent(`<style>html,body{margin:0;background:transparent}</style><svg xmlns="http://www.w3.org/2000/svg" width="72" height="72"><rect x="2" y="2" width="68" height="68" rx="13" fill="#12100d" fill-opacity=".85" stroke="#bca370" stroke-width="2"/><svg x="11" y="11" width="50" height="50" viewBox="0 0 24 24" opacity="${label.endsWith('Activity0')?.55:1}"><path d="${paths[kind]}" fill="none" stroke="${color}" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/></svg></svg>`);
  const png=await page.screenshot({omitBackground:true});fs.writeFileSync(path.join(out,label+'.png'),png);images.push({label,png:png.toString('base64')});
 }
 await browser.close();fs.writeFileSync(path.join(out,'images.json'),JSON.stringify(images));
 const rules=specs.map(([label,condition])=>({icon:{source:'PartyLight/canine-debug.swf',label,fadeStartDistance:1200,fadeMaxDistance:2200,maxInstances:1},match:{anyOf:[{[condition]:{}},{isDead:true,not:{isDead:true}}]}}));
 // The contradictory fallback keeps anyOf false if PartyLight's custom condition is unavailable.
 fs.writeFileSync(path.join(root,'package/SKSE/Plugins/SIF/PartyLight-CanineDebug.json'),JSON.stringify({rules},null,2)+'\n');
 console.log('Rendered 15 overhead symbols and fail-closed SIF rules.');
})().catch(e=>{console.error(e);process.exit(1);});
