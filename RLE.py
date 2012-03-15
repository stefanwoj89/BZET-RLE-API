#!/usr/bin/env python3



from struct import *
from ctypes import *
import sys
import struct
from   os.path import dirname

wordSize = 32# alter this with 8,16,32,64 to run this with the appropriate word-size
isBigEndian = 0;
if sys.byteorder == 'big':
    isBigEndian = 1

rle_ver = "v0.2"     # Version number
vdate   = "12-03-08" # Date of the version


dlldir = dirname(__file__)
# print( "DLL Directory:", dlldir )
dlldir.replace( '\\', '\\\\' )
# print( "DLL File:", dlldir+'\\_RLE.dll' )
#cdll[find_msvcrt()]
if wordSize == 8:
    #lib = cdll.LoadLibrary(dlldir+'\\gwah8-1.dll')
    lib = cdll.LoadLibrary('./gwah8-1.so') #uncomment this to run in linux

elif wordSize == 16:
    #lib = cdll.LoadLibrary(dlldir+'\\gwah16-2.dll')
    lib = cdll.LoadLibrary('./gwah16-2.so') #uncomment this to run in linux
elif wordSize == 32:
    #lib = cdll.LoadLibrary(dlldir+'\\gwah32-4.dll')
    lib = cdll.LoadLibrary('./gwah32-4.so') #uncomment this to run in linux
elif wordSize == 64:
    #lib = cdll.LoadLibrary(dlldir+'\\gwah64-8.dll')
    lib = cdll.LoadLibrary('./gwah64-8.so') #uncomment this to run in linux

PBZ = c_void_p
ULL = c_uint
LL  = c_longlong

###### Utility Operations that Allocate/Free RLEs 

# RLE Create a bitset
# ...............t
# and set a single bit to True
RLEcreate = lib.create
RLEcreate.argtypes = []
RLEcreate.restype  = PBZ


# RLE Replace a bitset
# This replaces the first argument with all
# the data from the second argument
# and frees the header of the second argument.
RLEreplace = lib.replace
RLEreplace.argtypes = [ PBZ, PBZ ]
RLEreplace.restype  = None


# RLE Create a rle bitset with bit at position x
# fff...........t
RLEint = lib.setInt # beg, end, stride
RLEint.argtypes = [ ULL ]
RLEint.restype  = PBZ

# RLE Create a rle bitset with a pattern of bits
# .............tfff...tfff...tfff...
# where the stride is 1 less than the number of fs
RLErange = lib.range # beg, end, stride
RLErange.argtypes = [ ULL, ULL]
RLErange.restype  = PBZ

# RLE Free an RLE
# Releases both the header and data
RLEfree = lib.RLEfree
RLEfree.argtypes = [ PBZ ]
RLEfree.restype  = None



###### Utility Operations on the RLE buffer
###### These get or put entire RLE buffers

# RLE Make a bitset from a bitarray with an rle in it.
RLEmake = lib.Buf2RLE
RLEmake.argtypes = [ c_char_p, ULL ]
RLEmake.restype  = PBZ

# RLE Get a bytearray with an rle in it from a bitset.
RLErepr = lib.RLE2Buf
RLErepr.argtypes = [ PBZ, c_char_p, ULL ]
RLErepr.restype  = PBZ



###### Utility Operations on RLEs
###### That operate on RLEs in place

# Bzet Set a bit Operation
RLEset = lib.set
RLEset.argtypes = [ PBZ, ULL ]
RLEset.restype  = None

# RLE Unset a bit Operation
RLEunset = lib.unset
RLEunset.argtypes = [ PBZ, ULL ]
RLEunset.restype  = None

# RLE Flip a bit Operation
RLEflip = lib.flip
RLEflip.argtypes = [ PBZ, ULL ]
RLEflip.restype  = None

# RLE Invert Bitset Operation (In place)
RLEinvert = lib.invert
RLEinvert.argtypes = [ PBZ ]
RLEinvert.restype  = None



###### Access to the RLE Header
RLEnwords = lib.nWords
RLEnwords.argtypes = [ PBZ ]
RLEnwords.restype = ULL

RLEsize = lib.RLEsize
RLEsize.argtypes = [PBZ]
RLEsize.restype = ULL




######## Utility Operations on RLEs
######## That return a Bool or Int
##
### RLE Find First Bit Operation
RLEfirst = lib.first
RLEfirst.argtypes = [ PBZ ]
RLEfirst.restype  = ULL
##
### RLE Find Last Bit Operation
RLElast = lib.last
RLElast.argtypes = [ PBZ ]
RLElast.restype  = ULL
##
### RLE Count Number of True Bits Operation
RLEtcnt = lib.count
RLEtcnt.argtypes = [ PBZ ]
RLEtcnt.restype  = ULL
##
### RLE Examine a bit
RLEtest = lib.test
RLEtest.argtypes = [ PBZ, ULL]
RLEtest.restype  = c_bool
##
### RLE EQ Operations
### Test if two bitsets are equal
RLEseteq = lib.eq
RLEseteq.argtypes = [ PBZ, PBZ ]
RLEseteq.restype  = c_bool
##
##
##
######## Utility Operations on RLEs
######## That return a new RLE
##
### RLE Not Bitset Operation (returns new bitset)
RLEnot = lib.RLEnot
RLEnot.argtypes = [ PBZ ]
RLEnot.restype  = PBZ

RLEand_op = lib.and_op_wrapper
RLEand_op.argtypes = [ PBZ, PBZ ]
RLEand_op.restype = PBZ

RLEor_op = lib.or_op_wrapper
RLEor_op.argtypes = [ PBZ, PBZ ]
RLEor_op.restype = PBZ

RLExor_op = lib.xor_op_wrapper
RLExor_op.argtypes = [ PBZ, PBZ ]
RLExor_op.restype = PBZ

RLEalign = lib.align
RLEalign.argtypes = [ PBZ, PBZ, ULL]
RLEalign.restype = None



# This magic gets 2 or 3 for Python2.xx or Python3.xx
python_v = sys.version_info.major
# This magic tells if it is a 32 or 64 bit implementation
python_64bit = 8 * struct.calcsize("P") == 64

class rle:
    @classmethod
    def BLevel(mymethod):
        return 0
    
    @classmethod
    def NewBLevel(mymethod,n):
        return
    
    @classmethod
    def Version(mymethod):
        if wordSize == 8:
            bv = "8"
        elif wordSize == 16:
            bv = "16"
        elif wordSize == 32:
            bv = "32"
        if python_64bit:
            bv = "64"
        return "RLE-" + bv + " " + rle_ver + " RLE" + " " + vdate
    
    # Type constants
    tbytes = type(bytes([]))
    tint   = type(3)
    tbarray= type(bytearray([]))
    tbool  = type(True)
    ttuple = type((0,))

    def __init__(self, x):
        if x == None:                 self.v = RLEcreate()        
        elif type(x) == self.tbytes:  self.v = RLEmake(x,len(x)>>2)
        elif type(x) == self.tbarray: self.v = RLEmake(bytes(x[:]),len(x)>>2)
        elif type(x) == self.tint:    self.v = RLEint(x)
        elif type(x) == type([]):
            r = RLEcreate()
            for ix in x:
                t = "temporary set to integrate into r"
                if type(ix) == self.tint:    t = RLEint(ix)
                elif type(ix) == self.ttuple and len(ix) >= 2:
                    b = ix[0] if ix[0] < ix[1] else ix[1]
                    e = ix[1] if ix[0] < ix[1] else ix[0]
                    n = len(ix)
                    if n <= 2:
                        if n == 1 or b == e: t = RLEint(b)
                        else: t = RLErange(b,e)
                    elif len(ix) == 3:       t = RLErange(b,e,ix[2])                    
                else: raise error
                RLEalign(r,t,0)
                r = RLEor_op( r, t )
                RLEfree(t)
                self.v = r
        else: raise error
        return;
    def LEV(self):
        return 0
        
    def HEX(self):      # This is for the syntax x.HEX()
        return __repr__(self.v)
    
    def __repr__(self):
        size = RLEsize(self.v)
        buffer = create_string_buffer(size)
        RLErepr(self.v, buffer, size)
        junk = buffer.raw[:]
        r = 'L0 len=' + str(len(junk))
        if wordSize == 8:
            for i in range(size):
                value = 0
                for j in range(1):
                    value = (value << 8)
                    if isBigEndian: value += junk[i + j]
                    else: value += junk[i+j]
                r += ' 0x%02X' % value
                if (i+1)%4 == 0: r += '\n        '
            if r[-1] == '\n': r = r[:-1]
        elif wordSize == 16:
            for i in range((size>>1)):
                value = 0
                for j in range(2):
                    value = (value << 8)
                    if isBigEndian: value += junk[2*i + j]
                    else: value += junk[2*i+1-j]
                r += ' 0x%04X' % value
                if (i+1)%4 == 0: r += '\n        '
            if r[-1] == '\n': r = r[:-1]
        elif wordSize == 32:
            for i in range(size>>2):
                value = 0
                for j in range(4):
                    value = (value << 8)
                    if isBigEndian: value += junk[4*i + j]
                    else: value += junk[4*i + 3-j]
                r += ' 0x%08X' % value
                if (i+1)%4 == 0: r += '\n        '      
            if r[-1] == '\n': r = r[:-1]
        elif wordSize == 64:
            for i in range(size>>3):
                value = 0
                for j in range(8):
                    value = (value << 8)
                    if isBigEndian: value += junk[4*i + j]
                    else: value += junk[8*i + 7-j]
                r += ' 0x%016X' % value
                if (i+1)%4 == 0: r += '\n        '      
            if r[-1] == '\n': r = r[:-1]
      
        return r


    @staticmethod
    def HEX( pbs ): return __repr__(pbs) # This is for the syntax HEX(x)

##    @staticmethod
##    def INT(x):                                                                 #stefan: not implemented
##        t = RLEcreate()
##        RLEadd_bits(t,x,False)
##        RLEadd_bits(t,1,True)
##        # Now put result into a Python RLE
##        rv = rle(None)
##        RLEreplace(rv,t)
##        return rv

    def __del__(self):
        # print( "Free called with ", self )
        RLEfree(self.v)
        return

    def __len__(self):  return RLEsize(self.v)*32 # number of bits in buffer, this should be changed in c.

    def size(self):     return RLEsize(self.v) # size of rle buffer used       #stefan: implemented...size in bytes of RLE

    def __bool__(self): return bool( RLEmt(self.v) )                            #stefan: not implemented

    def __getitem__(self,ix):
        if ix < 0: raise error
        return self.TEST(ix)

    def __setitem__(self, ix, val):
        if ix < 0: raise error
        bval = bool(val)
        if bval:
            RLEset( self.v, ix)
        else:
            RLEunset( self.v, ix )
        return self

    def __eq__(self,other):
        return self.EQ(other)

    def __ne__(self,other):
        return not self.EQ(other)
        
    def __or__    (self,other): return self.OR(other)
    def __and__   (self,other): return self.AND(other)
    def __xor__   (self,other): return self.XOR(other)
    def __invert__(self):       return self.INVERT()
    def __ior__   (self,other): return self.OR(other)
    def __iand__  (self,other): return self.AND(other)
    def __ixor__  (self,other): return self.XOR(other)
    
    def SET(self,x):
        RLEset( self.v, x)
        return self
    
    def UNSET(self,x):
        RLEunset( self.v, x )
        return self
    
    def FLIP(self,x):
        RLEflip( self.v, x )
        return self
    
    def INVERT(self):
        RLEinvert( self.v )
        return self
        
    def EQ(self,other):                                         
        return bool( RLEseteq( self.v, other.v ) )
    
    def CLEAR(self):                                            #Stefan: not implemented
        self.v = RLEclear(self.v)
        return self
    
    def COUNT(self):           # Number of One bits
        return RLEtcnt(self.v )
    
    def FIRST(self):
        return RLEfirst(self.v )
            
    def LAST(self):
        return RLElast(self.v )
    
    def LIST_T(self,dstart=0,limit=None):                       
        top = limit
        if limit == None: top = len(self)
        c = 0
        for x in range(dstart,len(self)):
            if self.TEST(x):
                c += 1
                yield x
                if c >= top: return
        return
    
    @staticmethod
    def _align_(bset1, bset2 ):
        RLEalign(bset1,bset2) # Does its work inplace.
        return

    def _normalize_( bset ):                                    # Stefan: not implemented
        # Remove unneccessary zeros at the end of the bitset        
        # Check for length 0 empty or nothing to do.
       #RLEtrim(bset)   # Does it's work inplace.     
        return;
            
    def TEST(self,x):                                           
        return bool( RLEtest( self.v, x) )
    
    def RANGE(b,e,stride=0):                                    # Stefan: stride not implemented
        # b is index of first bit
        # n is number of bits to find the end of the series
        # stride is how many bits between bits less 1.       
        # Create a pattern of bits in a temp RLE
        # print( "Invoke c range with:", b, e, stride )
        #t = RLErange(b,e,stride)
        t = RLErange(b,e+1)   
        # Create an Empty Python rle
        rv = rle(None)
        RLEreplace(rv.v,t)
        return rv



    def NOT(self):                                              # Stefan: implemented <-- caveat with the extra bits at the end of a word.
        t = RLEnot(self.v)
        r = rle(None)
        RLEreplace(r.v,t)
        return r
    
    def AND(self,other):
        t = RLEand_op( self.v, other.v )        # Do BinOP get tmp result
        r = rle(None)                           # Create an Empty Python rle
        RLEreplace(r.v,t)                       # replace r.v with t
        return r


    def OR(self,other):
        t = RLEor_op( self.v, other.v )
        r = rle(None)
        RLEreplace(r.v,t)
        return r

    
    def XOR(self,other):
        r = rle(None)
        t = RLExor_op( self.v, other.v )
        RLEreplace(r.v,t)
        return r



rle.tbits = type(rle(None))
