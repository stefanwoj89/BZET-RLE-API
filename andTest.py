from RLE import rle
bset1 = rle([1,100])
bset2 = rle([50,100])
bset3 = bset1.AND(bset2)
print(bset1)
print(bset2)
print(bset3)
