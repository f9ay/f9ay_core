import zlib
data = bytes.fromhex("78 01 fd e0 01 98 18 0c 03 05 01 01 36 6a 71 e7 00 00 00 00")
print(zlib.decompress(data))