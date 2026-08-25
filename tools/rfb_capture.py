import socket, struct, sys, time

def capture(port, outfile):
    s = socket.create_connection(('127.0.0.1', port), timeout=5)
    s.settimeout(15)
    def recvn(n):
        buf = b''
        while len(buf) < n:
            c = s.recv(n - len(buf))
            if not c: raise EOFError('closed')
            buf += c
        return buf
    banner = recvn(12)
    s.sendall(b'RFB 003.003\n')
    sec = struct.unpack('>I', recvn(4))[0]
    print('security type:', sec)
    s.sendall(b'\x00')
    fw, fh = struct.unpack('>HH', recvn(4))
    fmt = recvn(16)
    nlen = struct.unpack('>I', recvn(4))[0]
    recvn(nlen)
    print(f'framebuffer {fw}x{fh}')

    # SetPixelFormat: 24bpp RGB (type 0, 3 pad, 16 fmt)
    fmt24 = struct.pack('>BBBBHHHBBBxxx', 24, 24, 0, 1, 255, 255, 255, 16, 8, 0)
    s.sendall(b'\x00\x00\x00\x00' + fmt24)
    # SetEncodings: Raw (type 2, 1 pad, count 1, encoding 0)
    s.sendall(b'\x02\x00\x00\x01' + struct.pack('>i', 0))
    # FramebufferUpdateRequest non-incremental
    s.sendall(b'\x03\x00' + struct.pack('>HHHH', 0, 0, fw, fh))

    mtype = recvn(1)[0]
    if mtype != 0:
        print('unexpected msg type', mtype); return
    recvn(1)
    nrects = struct.unpack('>H', recvn(2))[0]
    print('rects:', nrects)
    data = bytearray()
    for i in range(nrects):
        x, y, w, h, enc = struct.unpack('>HHHHI', recvn(12))
        print(f'  rect {x},{y} {w}x{h} enc={enc}')
        if enc == 0:
            data.extend(recvn(w * h * 3))
        else:
            print('  enc', enc); s.close(); return
    s.close()
    from PIL import Image
    img = Image.new('RGB', (fw, fh))
    px = img.load()
    i = 0
    for yy in range(fh):
        for xx in range(fw):
            px[xx, yy] = (data[i], data[i+1], data[i+2]); i += 3
    img.save(outfile)
    print('saved', outfile)

capture(int(sys.argv[1]), sys.argv[2])
