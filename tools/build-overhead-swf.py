"""Build AS2-compatible exported icon sprites; no scripts or hooks in the movie."""
import base64, io, json, math, pathlib, struct, zlib
from PIL import Image
root=pathlib.Path(__file__).resolve().parents[1]
class Bits:
    def __init__(self): self.bits=[]
    def put(self,value,count): self.bits.extend((value>>i)&1 for i in range(count-1,-1,-1))
    def data(self):
        self.bits.extend([0]*(-len(self.bits)%8))
        return bytes(sum(self.bits[j+i]<<(7-i) for i in range(8)) for j in range(0,len(self.bits),8))
def signed_bits(*values): return max(1,max(abs(v).bit_length()+1 for v in values))
def rect(lo=-240,hi=240):
    b=Bits();n=signed_bits(lo,hi);b.put(n,5)
    for v in [lo,hi,lo,hi]:b.put(v,n)
    return b.data()
def matrix(bitmap=False):
    b=Bits();b.put(int(bitmap),1)
    if bitmap:
        scale=round(480/72*65536);n=signed_bits(scale);b.put(n,5);b.put(scale,n);b.put(scale,n)
    b.put(0,1)
    if bitmap:
        n=signed_bits(-240);b.put(n,5);b.put(-240,n);b.put(-240,n)
    else:b.put(0,5)
    return b.data()
def tag(code,data=b''):
    length=len(data)
    return struct.pack('<H',(code<<6)|min(length,63))+(struct.pack('<I',length) if length>=63 else b'')+data
def shape(shape_id,bitmap_id):
    b=Bits();b.put(1,4);b.put(0,4)
    b.put(0,1);b.put(5,5);b.put(9,5);b.put(-240,9);b.put(-240,9);b.put(1,1)
    for delta,vertical in [(480,0),(480,1),(-480,0),(-480,1)]:
        n=signed_bits(delta);b.put(1,1);b.put(1,1);b.put(n-2,4);b.put(0,1);b.put(vertical,1);b.put(delta,n)
    b.put(0,6)
    return tag(32,struct.pack('<H',shape_id)+rect()+b'\x01\x41'+struct.pack('<H',bitmap_id)+matrix(True)+b'\x00'+b.data())
body=rect()+struct.pack('<HH',24*256,1)+tag(69,b'\0'*4)
exports=[]
for i,item in enumerate(json.loads((root/'verification/overhead/images.json').read_text())):
    bitmap_id,shape_id,sprite_id=i*3+1,i*3+2,i*3+3
    image=Image.open(io.BytesIO(base64.b64decode(item['png']))).convert('RGBA')
    pixels=bytearray()
    for r,g,b,a in image.getdata():pixels.extend([a,(r*a+127)//255,(g*a+127)//255,(b*a+127)//255])
    body+=tag(36,struct.pack('<HBHH',bitmap_id,5,image.width,image.height)+zlib.compress(pixels))
    body+=shape(shape_id,bitmap_id)
    sprite=struct.pack('<HH',sprite_id,1)+tag(26,b'\x06'+struct.pack('<HH',1,shape_id)+matrix())+tag(1)+tag(0)
    body+=tag(39,sprite);exports.append((sprite_id,item['label']))
body+=tag(56,struct.pack('<H',len(exports))+b''.join(struct.pack('<H',sid)+name.encode()+b'\0' for sid,name in exports))+tag(1)+tag(0)
output=root/'package/Interface/PartyLight/canine-debug.swf'
output.write_bytes(b'FWS\x08'+struct.pack('<I',len(body)+8)+body)
print(f'Created {output}: {len(body)+8} bytes, {len(exports)} exported sprites.')
