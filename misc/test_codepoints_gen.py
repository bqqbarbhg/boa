import itertools

def grouped(n, iterable):
    it = iter(iterable)
    while True:
       chunk = tuple(itertools.islice(it, n))
       if not chunk:
           return
       yield chunk

def fibo():
    a,b = 0,1
    while True:
        yield a
        a, b = b, a + b

def fibo_points():
    for f in fibo():
        if 0xD800 <= f < 0xE000 or f == 0: continue
        elif f > 0x10FFFF: return
        else: yield f

codepoints = []

# Single bit set
for i in range(0, 21):
    codepoints.append(1 << i)

# Low bits set
for i in range(1, 21):
    codepoints.append((1 << i) - 1)

# High bits set
for i in range(0, 20):
    codepoints.append(~((1 << i) - 1) & 0xFFFFF)

# UTF-16 edges
codepoints += [0xD7FF, 0xE000, 0xFFFF, 0x10000, 0x10FFFF]

# UTF-8 edges
codepoints += [0x7F, 0x80, 0x7FF, 0x800, 0xFFFF, 0x10000, 0x10FFFF]

# AAAAAAAA
codepoints += [0xA, 0xAA, 0xAAA, 0xAAAA, 0xAAAAA]

# Fibonacci codepoints
codepoints += fibo_points()

codepoints = sorted(set(codepoints))

for utf32 in codepoints:
    utf8 = ['0x%02x' % b for b in chr(utf32).encode('utf-8')]
    utf16 = ['0x%04x' % (p[0] << 8 | p[1]) for p in grouped(2, chr(utf32).encode('utf-16be'))]
    utf8len = len(utf8)
    utf16len = len(utf16)
    while len(utf8) < 4: utf8.append('0x00')
    while len(utf16) < 2: utf16.append('0x0000')
    print('\t{ 0x%06x, { %s }, { %s }, %d, %d },' % (utf32, ','.join(utf8), ','.join(utf16), utf8len, utf16len))
